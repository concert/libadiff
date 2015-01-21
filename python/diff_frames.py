#!/usr/bin/env python
import os
import sys
import argh
import asyncio
import subprocess
from itertools import chain
from contextlib import contextmanager
from math import ceil

import pysndfile

from terminal import LinePrintingTerminal, Keyboard
from util import cache, overlay_lists, AB, Time, Clamped, FormattedList

bdiff_frames_path = os.path.join(
    os.path.dirname(__file__), os.pardir, 'diff_frames')


class Hunk:
    __slots__ = 'starts', 'ends'

    def __init__(self, start_a, end_a, start_b, end_b):
        '''
        :parameter start_a: The frame in A where the files start to differ.
        :parameter end_a: The frame in A after which the files cease to differ.
        :parameter start_b: The frame in B where the files start to differ.
        :parameter end_b: The frame in B after which the file cease to differ.
        '''
        self.starts = AB(start_a, start_b)
        self.ends = AB(end_a, end_b)

    def _get_attr_string(self):
        return 'start_a={}, end_a={}, start_b={}, end_b={}'.format(*self)

    def __repr__(self):
        return '{}({})'.format(
            self.__class__.__name__, self._get_attr_string())

    def __eq__(self, other):
        return all(
            getattr(self, name) == getattr(other, name) for name in
            self.__slots__ if not name.startswith('_'))

    def __iter__(self):
        return chain(*zip(self.starts, self.ends))


class NormalisedHunk(Hunk):
    __slots__ = Hunk.__slots__ + ('offsets', '_end')

    def __init__(self, start_a, end_a, start_b, end_b, offset_a, offset_b):
        super().__init__(start_a, end_a, start_b, end_b)
        self.offsets = AB(offset_a, offset_b)

    @classmethod
    def from_hunk(cls, hunk, offsets):
        '''
        :parameter hunk: The :class:`~Hunk` object that we wish to normalise
            with the given offsets.
        :parameter offsets: The number of frames that must be subtracted from
            the normalised diff space to get an actual frame in each file.
        '''
        starts = hunk.starts + offsets
        ends = hunk.ends + offsets
        return cls(starts.a, ends.a, starts.b, ends.b, offsets.a, offsets.b)

    @classmethod
    def process_diff(cls, diff):
        '''Translates the raw absolute frame references in the given diff into a
        common "virtual" space where like sections are aligned.
        '''
        offsets = AB(0, 0)
        for hunk in diff:
            yield cls.from_hunk(hunk, offsets)
            offsets += (hunk.ends - hunk.starts).reversed()

    def _get_attr_string(self):
        attr_string = super()._get_attr_string()
        attr_string += ', offset_a={}, offset_b={}'.format(*self.offsets)
        return attr_string

    @property
    def start(self):
        return self.starts.a

    @property
    @cache
    def end(self):
        return max(self.ends)

    def scale(self, width):
        inserted_frames = self.ends - self.starts
        return inserted_frames.map(
            lambda f: (f / len(self)) * width
        ).map(ceil).map(int)

    def __len__(self):
        return self.end - self.start


def duration(sndfile):
    '''
    :parameter sndfile: The :class:`PySndFile` instance from which to calculate
        the duration.
    :returns float: the duration of the given audio file, in seconds.
    '''
    return sndfile.frames() / sndfile.samplerate()


class _DrawState:
    '''Throw-away objects for ease of re-initialising draw state on every draw()
    call.
    '''
    __slots__ = (
        'app', '_diff_width', '_chars_per_frame', '_cursor_pos', 'start_times',
        'end_times')

    def __init__(self, app):
        self.app = app
        self.start_times = None
        self.end_times = None

    @property
    @cache
    def diff_width(self):
        if self.start_times:
            start_time_width = max(map(len, self.start_times)) + 1
        else:
            start_time_width = 0
        end_time_width = max(map(len, self.end_times)) + 1
        return self.app._terminal.width - start_time_width - end_time_width

    @property
    @cache
    def chars_per_frame(self):
        return self.app._zoom * self.diff_width / self.app._len

    def to_chars(self, frames):
        '''Transforms a point in the space of the "virtual" diff space, given in
        frames, into a x-coordinate on the terminal, taking into account
        various application state parameters.
        '''
        return int(ceil(self.chars_per_frame * frames))

    def to_frames(self, chars):
        '''The inverse of to_chars()
        '''
        return int(chars / self.chars_per_frame)

    @property
    @cache
    def cursor_pos(self):
        return self.to_chars(self.app._cursor)


class DiffApp:
    def __init__(self, filename_a, filename_b, terminal=None):
        self.filenames = AB(filename_a, filename_b)
        self._psfs = None
        self._durations = None
        self._diff = None
        self._len = 0

        self._draw_state = None

        self._zoom = 1.0
        self._cues_are_free = True
        self._start_cue = 0
        self._end_cue = 0
        self._active_stream = 1
        self._cursor = 0
        self._status = ''

        self._loop = asyncio.get_event_loop()
        self._terminal = terminal or LinePrintingTerminal()
        self._insertion_fmt = self._terminal.green
        self._change_fmt = self._terminal.yellow
        self._inactive_cursor_fmt = self._terminal.on_bright_black
        self._active_cursor_fmt = (
            self._inactive_cursor_fmt + self._terminal.reverse)
        self._loop.add_reader(self._terminal.infile, self._handle_input)
        self._keyboard = Keyboard()
        self.bindings = {
            '+': self._zoom_in,
            '=': self._zoom_in,
            '-': self._zoom_out,
            'ctrl i': self._cue_next_hunk,  # Tab, apparently
            'shift tab': self._cue_prev_hunk,
            '[': self._set_start_cue,
            ']': self._set_end_cue,
            'up': self._on_up,
            'down': self._on_down,
            'left': self._on_left,
            'right': self._on_right,
            'esc': self._on_esc,
            'home': self._on_home,
            'end': self._on_end,
        }

    def _start_cue_max(self):
        if self._cues_are_free:
            return self._len
        else:
            return self._end_cue - self._draw_state.to_frames(1)

    def _end_cue_min(self):
        if self._cues_are_free:
            return 0
        else:
            return self._start_cue + self._draw_state.to_frames(1)

    def _cursor_max(self):
        if self._draw_state:
            return self._len - self._draw_state.to_frames(1)
        else:
            return self._len

    @contextmanager
    def _free_cues(self):
        '''We normally want to constrain the movement of the cue points so they
        can't cross, but when updating both together, e.g. when moving hunk, we
        need to lift the constraint.
        '''
        self._cues_are_free = True
        yield
        self._cues_are_free = False

    _zoom = Clamped(1.0, None)
    _start_cue = Clamped(0, _start_cue_max)
    _end_cue = Clamped(_end_cue_min, lambda self: self._len)
    _cursor = Clamped(0, _cursor_max)
    _active_stream = Clamped(0, lambda self: len(self.filenames) - 1)

    def __call__(self):
        self._psfs = AB.from_map(pysndfile.PySndfile, self.filenames)
        self._durations = self._psfs.map(duration).map(Time.from_seconds)
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
        self._diff = tuple(NormalisedHunk.process_diff(diff))
        self._len = max(self._psfs.frames() + self._diff[-1].offsets)
        self._cue_next_hunk()
        self._draw()
        while True:
            yield from asyncio.sleep(1)

    def _handle_input(self):
        key = self._keyboard[self._terminal.infile.read()]
        self._status = key
        action = self.bindings.get(key, lambda: None)
        action()
        self._draw()

    @asyncio.coroutine
    def _do_diff(self):
        '''Calls the (fast) C-based binary diffing code to get the differences
        between the two audio files.
        '''
        process = yield from asyncio.create_subprocess_exec(
            bdiff_frames_path, self.filenames.a, self.filenames.b,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = yield from process.communicate()
        if process.returncode:
            raise RuntimeError(stderr)
        else:
            return tuple(
                Hunk(*map(int, l.split())) for l in stdout.splitlines())

    def _make_formatted_list(self, string):
        return FormattedList(string, self._terminal.normal)

    def _make_diff_reprs(self, draw_state, diff):
        l = lambda: self._make_formatted_list('-' * draw_state.diff_width)
        diff_reprs = AB(l(), l())
        for hunk in diff:
            hunk_width = draw_state.to_chars(len(hunk))
            dr_start_idx = draw_state.to_chars(hunk.start)
            dr_end_idx = dr_start_idx + hunk_width

            # hunk is entirely out of viewport, so don't bother drawing
            if dr_end_idx < 0:
                continue
            elif dr_start_idx > draw_state.diff_width:
                break

            hunk_scales = hunk.scale(hunk_width)
            hunk_lists = hunk_scales.map(lambda hs: '+' * hs).ljust(
                hunk_width).map(self._make_formatted_list)
            if any(s < hunk_width for s in hunk_scales):
                change_threshold = min(hunk_scales)
                hunk_lists.format(0, change_threshold, self._change_fmt)
                hunk_lists.format(
                    change_threshold, hunk_width, self._insertion_fmt)
            else:
                hunk_lists.format(0, hunk_width, self._change_fmt)
            for dr, hs in zip(diff_reprs, hunk_lists):
                overlay_lists(dr, hs, dr_start_idx)
        return diff_reprs

    def _make_diff_lines(self, draw_state, diff):
        diff_reprs = self._make_diff_reprs(draw_state, diff)
        for i, diff_repr in enumerate(diff_reprs):
            if i == self._active_stream:
                fmt = self._active_cursor_fmt
            else:
                fmt = self._inactive_cursor_fmt
            diff_repr.format(
                draw_state.cursor_pos, draw_state.cursor_pos + 1, fmt)
        return diff_reprs.map(str) + AB(' ', ' ') + draw_state.end_times

    def _make_cue_line(self, draw_state):
        cue_line = self._make_formatted_list(' ' * draw_state.diff_width)
        cue_line[draw_state.to_chars(self._start_cue)] = (), '['
        cue_line[draw_state.to_chars(self._end_cue)] = (), ']'
        cursor_pos = draw_state.to_chars(self._cursor)
        cue_line.format(
            draw_state.cursor_pos, draw_state.cursor_pos + 1,
            self._inactive_cursor_fmt)
        return str(cue_line)

    def _draw(self):
        self._draw_state = ds = _DrawState(self)
        ds.end_times = self._durations.map(str)
        diff_lines = self._make_diff_lines(ds, self._diff)
        if self._zoom == 1:
            pass
        else:
            # FIXME: Show times of each end when zoomed in
            pass
        lines = [
            diff_lines.a,
            self._make_cue_line(ds),
            diff_lines.b,
            self._status.ljust(self._terminal.width)
        ]
        self._terminal.print_lines(lines)

    def _zoom_in(self):
        self._zoom *= 1.1

    def _zoom_out(self):
        self._zoom /= 1.1

    def _cue_hunk_helper(self, iterable, predicate):
        for hunk in iterable:
            if predicate(hunk):
                with self._free_cues():
                    self._start_cue = hunk.start
                    self._end_cue = hunk.end
                self._cursor = self._start_cue
                break

    def _cue_next_hunk(self):
        self._cue_hunk_helper(
            self._diff, lambda hunk: hunk.start > self._cursor)

    def _cue_prev_hunk(self):
        self._cue_hunk_helper(
            reversed(self._diff), lambda hunk: hunk.end < self._cursor)

    def _set_start_cue(self):
        self._start_cue = self._cursor

    def _set_end_cue(self):
        self._end_cue = self._cursor

    def _move_cursor(self, d):
        self._cursor += self._draw_state.to_frames(d)

    def _on_up(self):
        self._active_stream -= 1

    def _on_down(self):
        self._active_stream += 1

    def _on_left(self):
        self._move_cursor(-1)

    def _on_right(self):
        self._move_cursor(1)

    def _on_home(self):
        if self._cursor == self._start_cue:
            self._cursor = 0
        else:
            self._cursor = self._start_cue

    def _on_end(self):
        if self._cursor == self._end_cue:
            self._cursor = self._len
        else:
            self._cursor = self._end_cue

    def _on_esc(self):
        self._cursor = self._start_cue


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
