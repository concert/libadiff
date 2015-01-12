#!/usr/bin/env python
import os
import sys
import argh
import asyncio
import subprocess
from itertools import chain

import pysndfile

from terminal import LinePrintingTerminal, Keyboard
from util import caching_property, overlay_lists, AB, Time, Clamped

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
            self.__slots__)

    def __iter__(self):
        return chain(*zip(self.starts, self.ends))


class NormalisedHunk(Hunk):
    __slots__ = Hunk.__slots__ + ('offsets',)

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
    def end(self):
        return max(self.ends)

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
        'app', '_diff_width', '_chars_per_frame', 'start_times', 'end_times')

    def __init__(self, app):
        self.app = app
        self.start_times = None
        self.end_times = None

    @caching_property
    def diff_width(self):
        if self.start_times:
            start_time_width = max(map(len, self.start_times)) + 1
        else:
            start_time_width = 0
        end_time_width = max(map(len, self.end_times)) + 1
        return self.app._terminal.width - start_time_width - end_time_width

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
        self.filenames = AB(filename_a, filename_b)
        self._psfs = None
        self._durations = None
        self._diff = None
        self._len = 0

        self._zoom = 1.0
        self._start_cue = 0
        self._end_cue = 0
        self._active_cue = None
        self._cursor = 0
        self._status = ''

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
            'esc': self._on_esc,
        }

    _zoom = Clamped(1.0, None)
    _cursor = Clamped(0, lambda self: self._len)

    def __call__(self):
        self._psfs = AB.from_map(pysndfile.PySndfile, self.filenames)
        self._durations = AB.from_map(duration, self._psfs)
        self._durations = AB.from_map(Time.from_seconds, self._durations)
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

    def _make_diff_repr(self, draw_state, diff, idx):
        diff_repr = ['-'] * draw_state.diff_width
        for hunk in diff:
            hunk_num_chars = draw_state.to_chars(len(hunk))
            dl_start_idx = draw_state.to_chars(hunk.start)
            dl_end_idx = dl_start_idx + hunk_num_chars

            # hunk is entirely out of viewport, so don't bother drawing
            if dl_end_idx < 0:
                continue
            elif dl_start_idx > len(diff_repr):
                break

            frames_inserted = hunk.ends[idx] - hunk.starts[idx]
            insertion_str = '+' * int(
                (frames_inserted / len(hunk)) * hunk_num_chars)
            insertion_str = list(insertion_str.ljust(hunk_num_chars))
            insertion_str[0] = self._insertion_fmt + insertion_str[0]
            insertion_str[-1] += self._common_fmt
            overlay_lists(diff_repr, insertion_str, dl_start_idx)
        diff_repr[0] = self._common_fmt + diff_repr[0]
        diff_repr[-1] += self._terminal.normal
        return diff_repr

    def _make_diff_line(self, draw_state, diff, idx):
        duration = draw_state.end_times[idx]
        diff_repr = self._make_diff_repr(draw_state, diff, idx)
        overlay_lists(diff_repr, '|', draw_state.to_chars(self._cursor))
        return ''.join(diff_repr) + ' ' + duration

    @property
    def _start_cue_mark(self):
        if self._active_cue == 'start':
            return self._terminal.reverse('[')
        else:
            return '['

    @property
    def _end_cue_mark(self):
        if self._active_cue == 'end':
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

    def _draw(self):
        self._draw_state = ds = _DrawState(self)
        ds.end_times = AB.from_map(str, self._durations)
        if self._zoom == 1:
            pass
        else:
            # FIXME: Show times of each end when zoomed in
            pass
        lines = [
            self._make_diff_line(ds, self._diff, 0),
            self._make_cue_line(ds),
            self._make_diff_line(ds, self._diff, 1),
            self._status.ljust(self._terminal.width)
        ]
        self._terminal.print_lines(lines)

    def _zoom_in(self):
        self._zoom *= 1.1

    def _zoom_out(self):
        self._zoom /= 1.1

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
        self._select_cue_helper('end')

    def _move_active_cue_point(self, d):
        attr_name = '_{}_cue'.format(self._active_cue)
        setattr(
            self, attr_name,
            getattr(self, attr_name) + self._draw_state.to_frames(d))

    def _move_cursor(self, d):
        self._cursor += self._draw_state.to_frames(d)

    def _on_arrow(self, d):
        if self._active_cue:
            self._move_active_cue_point(d)
        else:
            self._move_cursor(d)

    def _on_left(self):
        self._on_arrow(-1)

    def _on_right(self):
        self._on_arrow(1)

    def _on_esc(self):
        if self._active_cue:
            self._active_cue = None
        else:
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
