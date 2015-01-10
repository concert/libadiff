#!/usr/bin/env python
import os
import sys
import argh
import asyncio
import subprocess
from functools import wraps

import pysndfile

from terminal import LinePrintingTerminal, Keyboard

bdiff_frames_path = os.path.join(
    os.path.dirname(__file__), os.pardir, 'diff_frames')


class Hunk:
    __slots__ = 'start_a', 'end_a', 'start_b', 'end_b'

    def __init__(self, start_a, end_a, start_b, end_b):
        self.start_a = start_a
        self.end_a = end_a
        self.start_b = start_b
        self.end_b = end_b

    def __eq__(self, other):
        return all(
            getattr(self, name) == getattr(other, name) for name in
            self.__slots__)


class NormalisedHunk(Hunk):
    @property
    def start(self):
        return self.start_a

    @property
    def end(self):
        return max(self.end_a, self.end_b)

    def __len__(self):
        return self.end - self.start


def duration(sndfile):
    '''
    :parameter sndfile: The :class:`PySndFile` instance from which to calculate
        the duration.
    :returns float: the duration of the given audio file, in seconds.
    '''
    return sndfile.frames() / sndfile.samplerate()


class Time:
    '''Formats the given time in seconds to an appropriate prescision exlcuding
    leading zero components. Edge cases around overflowing make this
    non-trivial.
    '''

    __slots__ = 'hours', 'minutes', 'seconds'

    def __init__(self, seconds):
        self.hours, self.minutes, self.seconds = self._round_hms(
            *self._split(seconds))

    @staticmethod
    def _split(seconds):
        hours, remainder = divmod(seconds, 3600)
        minutes, seconds = divmod(remainder, 60)
        return hours, minutes, seconds

    @staticmethod
    def _round_pair(big, small, precision):
        if round(small, precision) == 60:
            big += 1
            small = 0.0
        return big, small

    @staticmethod
    def _select(hours, minutes, seconds, a, b, c):
        if not hours:
            if not minutes:
                return c
            return b
        return a

    def _round_hms(self, hours, minutes, seconds):
        second_prec = self._select(hours, minutes, seconds, 0, 1, 2)
        minutes, seconds = self._round_pair(minutes, seconds, second_prec)
        hours, minutes = self._round_pair(hours, minutes, 0)
        return hours, minutes, seconds

    def __str__(self):
        fmt_str = self._select(
            self.hours, self.minutes, self.seconds,
            '{hours:.0f}:{minutes:02.0f}:{seconds:02.0f}',
            '{minutes:.0f}:{seconds:04.1f}',
            '{seconds:.2f}')
        return fmt_str.format(
            hours=self.hours, minutes=self.minutes, seconds=self.seconds)


def fmt_seconds(seconds):
    return str(Time(seconds))


def overlay_lists(base, new, offset):
    '''Mutates the given base list by overlaying the new list on top at a given
    offset.
    '''
    if offset + len(new) >= 0:
        for i in range(len(new)):
            base_idx = i + offset
            if 0 <= base_idx < len(base):
                base[base_idx] = new[i]
            elif base_idx >= len(base):
                break


def caching_property(method):
    attr_name = '_' + method.__name__
    @property
    @wraps(method)
    def new_property(self):
        if not getattr(self, attr_name):
            setattr(self, attr_name, method(self))
        return getattr(self, attr_name)
    return new_property


class _DrawState:
    '''Throw-away objects for ease of re-initialising draw state on every draw()
    call.
    '''
    __slots__ = (
        'app', '_diff_width', '_chars_per_frame', 'start_time_a', 'end_time_a',
        'start_time_b', 'end_time_b')

    def __init__(self, app):
        self.app = app
        self._diff_width = None
        self._chars_per_frame = None

    @caching_property
    def diff_width(self):
        duration_width = max(len(self.end_time_a), len(self.end_time_b))
        return self.app._terminal.width - duration_width - 1

    @caching_property
    def chars_per_frame(self):
        return self.app._zoom * self.diff_width / self.app._len

    def to_chars(self, frames):
        '''Transforms a point in the space of the "virtual" diff space, given in
        frames, into a x-coordintate on the terminal, taking into account
        various application state parameters.
        '''
        return int(self.chars_per_frame * frames)

    def to_frames(self, chars):
        '''The inverse of to_chars()
        '''
        return int(chars / self.chars_per_frame)


class DiffApp:
    def __init__(self, filename_a, filename_b, terminal=None):
        self.filename_a = filename_a
        self.filename_b = filename_b
        self._psf_a = None
        self._psf_b = None
        self._diff = None
        self._len = None

        self._zoom = 1.0
        self._start_cue = 0
        self._end_cue = 0
        self._active_cue = None

        self._draw_state = None

        self._loop = asyncio.get_event_loop()
        self._terminal = terminal or LinePrintingTerminal()
        self._common_fmt = self._terminal.yellow
        self._insertion_fmt = self._terminal.green
        self._loop.add_reader(self._terminal.infile, self._handle_input)
        self._keyboard = Keyboard()
        self.bindings = {
            '+': self._zoom_in,
            '=': self._zoom_in,
            '-': self._zoom_out,
            'ctrl i': self._cue_next_hunk,  # Tab, apparently
            'shift tab': self._cue_prev_hunk,
            '[': self._select_start_cue,
            ']': self._select_end_cue,
            'left': self._on_left,
            'right': self._on_right,
        }

    def __call__(self):
        self._psf_a = pysndfile.PySndfile(self.filename_a)
        self._psf_b = pysndfile.PySndfile(self.filename_b)
        with self._terminal.unbuffered_input(), (
                self._terminal.nonblocking_input()), (
                self._terminal.hidden_cursor()):
            try:
                self._loop.run_until_complete(self._run())
            finally:
                self._loop.close()
                print('\n', end='')  # Yuck :-(

    @asyncio.coroutine
    def _run(self):
        diff = yield from self._do_diff()
        self._diff, a_offset, b_offset = self._process_diff(diff)
        self._len = max(
            self._psf_a.frames() + b_offset,
            self._psf_b.frames() + a_offset)
        self._cue_next_hunk()
        self._draw()
        while True:
            yield from asyncio.sleep(1)

    def _handle_input(self):
        key = self._keyboard[self._terminal.infile.read()]
        action = self.bindings.get(key, lambda: None)
        action()
        self._draw()

    @asyncio.coroutine
    def _do_diff(self):
        '''Calls the (fast) C-based binary diffing code to get the differences
        between the two audio files.
        '''
        process = yield from asyncio.create_subprocess_exec(
            bdiff_frames_path, self.filename_a, self.filename_b,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = yield from process.communicate()
        if process.returncode:
            raise RuntimeError(stderr)
        else:
            return tuple(
                Hunk(*map(int, l.split())) for l in stdout.splitlines())

    @staticmethod
    def _process_diff(diff):
        '''Translates the raw absolute frame references in the given diff into a
        common "virtual" space where like sections are aligned.
        '''
        a_offset = 0
        b_offset = 0
        result = []
        for hunk in diff:
            result.append(NormalisedHunk(
                hunk.start_a + b_offset,
                hunk.end_a + b_offset,
                hunk.start_b + a_offset,
                hunk.end_b + a_offset))
            a_offset += hunk.end_a - hunk.start_a
            b_offset += hunk.end_b - hunk.start_b
        return tuple(result), a_offset, b_offset

    def _make_diff_line(self, draw_state, diff, is_b=False):
        diff_line = ['-'] * draw_state.diff_width
        for hunk in diff:
            hunk_num_chars = draw_state.to_chars(len(hunk))
            dl_start_idx = draw_state.to_chars(hunk.start)
            dl_end_idx = dl_start_idx + hunk_num_chars

            # hunk is entirely out of viewport, so don't bother drawing
            if dl_end_idx < 0:
                continue
            elif dl_start_idx > len(diff_line):
                break

            if is_b:
                frames_inserted = hunk.end_b - hunk.start_b
            else:
                frames_inserted = hunk.end_a - hunk.start_a

            insertion_str = '+' * int(
                (frames_inserted / len(hunk)) * hunk_num_chars)
            insertion_str = list(insertion_str.ljust(hunk_num_chars))
            insertion_str[0] = self._insertion_fmt + insertion_str[0]
            insertion_str[-1] += self._common_fmt
            overlay_lists(diff_line, insertion_str, dl_start_idx)
        if is_b:
            duration = draw_state.end_time_b
        else:
            duration = draw_state.end_time_a
        return self._common_fmt(''.join(diff_line)) + ' ' + duration

    @property
    def _start_cue_mark(self):
        if self._active_cue == 'start':
            return self._terminal.reverse('[')
        else:
            return '['

    @property
    def _end_cue_mark(self):
        if self._active_cue == 'stop':
            return self._terminal.reverse(']')
        else:
            return ']'

    def _make_cue_line(self, draw_state):
        cue_line = [' '] * draw_state.diff_width
        overlay_lists(
            cue_line, [self._start_cue_mark],
            draw_state.to_chars(self._start_cue))
        overlay_lists(
            cue_line, [self._end_cue_mark],
            draw_state.to_chars(self._end_cue))
        return ''.join(cue_line)

    @property
    def _diff_width(self):
        if self._zoom > 1:
            pass

    def _draw(self):
        self._draw_state = ds = _DrawState(self)
        ds.end_time_a = fmt_seconds(duration(self._psf_a))
        ds.end_time_b = fmt_seconds(duration(self._psf_b))
        lines = [
            self._make_diff_line(ds, self._diff),
            self._make_cue_line(ds),
            self._make_diff_line(ds, self._diff, is_b=True)
        ]
        self._terminal.print_lines(lines)

    def _zoom_in(self):
        self._zoom *= 1.1

    def _zoom_out(self):
        self._zoom = max(1.0, self._zoom / 1.1)

    def _cue_hunk_helper(self, iterable, predicate):
        self._active_cue = None
        for hunk in iterable:
            if predicate(hunk):
                self._start_cue = hunk.start
                self._end_cue = hunk.end
                break

    def _cue_next_hunk(self):
        self._cue_hunk_helper(
            self._diff, lambda hunk: hunk.start > self._start_cue)

    def _cue_prev_hunk(self):
        self._cue_hunk_helper(
            reversed(self._diff), lambda hunk: hunk.end < self._end_cue)

    def _select_cue_helper(self, cue_name):
        if self._active_cue == cue_name:
            self._active_cue = None
        else:
            self._active_cue = cue_name

    def _select_start_cue(self):
        self._select_cue_helper('start')

    def _select_end_cue(self):
        self._select_cue_helper('stop')

    def _move_active_cue_point(self, d):
        if self._active_cue:
            transform = self._get_frames_transform(self._draw_state.diff_width)
            attr_name = '_{}_cue'.format(self._active_cue)
            setattr(self, attr_name, getattr(self, attr_name) + transform(d))

    def _on_left(self):
        self._move_active_cue_point(-1)

    def _on_right(self):
        self._move_active_cue_point(1)


def diff(filename_a, filename_b):
    '''Diffs the two given audiofiles and throws away the return value so
    blasted argh doesn't try to print the whole bally lot!
    '''
    app = DiffApp(filename_a, filename_b)
    try:
        app()
    except KeyboardInterrupt:
        sys.exit()

if __name__ == '__main__':
    argh.dispatch_command(diff)
