#!/usr/bin/env python
import os
import sys
import argh
import asyncio
import subprocess
from collections import namedtuple

import pysndfile

from terminal import LinePrintingTerminal, Keyboard

bdiff_frames_path = os.path.join(
    os.path.dirname(__file__), os.pardir, 'diff_frames')

Hunk = namedtuple('Hunk', ('start_a', 'end_a', 'start_b', 'end_b'))


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


class DiffApp:
    def __init__(self, filename_a, filename_b, terminal=None):
        self.filename_a = filename_a
        self.filename_b = filename_b
        self._psf_a = None
        self._psf_b = None
        self._diff = None
        self._len = None

        self._zoom = 1.0

        self._loop = asyncio.get_event_loop()
        self._terminal = terminal or LinePrintingTerminal()
        self._common_fmt = self._terminal.yellow
        self._insertion_fmt = self._terminal.green
        self._loop.add_reader(self._terminal.infile, self._handle_input)
        self._keyboard = Keyboard()
        self.bindings = {}

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
            result.append(Hunk(
                hunk.start_a + b_offset,
                hunk.end_a + b_offset,
                hunk.start_b + a_offset,
                hunk.end_b + a_offset))
            a_offset += hunk.end_a - hunk.start_a
            b_offset += hunk.end_b - hunk.start_b
        return tuple(result), a_offset, b_offset

    def _get_transform(self, width):
        '''Returns a function that transforms a point in the space of the
        "virtual" diff space, given in frames, into a x-coordintate on the
        terminal, taking into account various application state parameters.
        '''
        chars_per_frame = self._zoom * width / self._len
        def transform(frames):
            return int(chars_per_frame * frames)
        return transform

    def _make_diff_line(self, diff, width, is_b=False):
        diff_line = ['-'] * width
        transform = self._get_transform(width)
        for hunk in diff:
            hunk_num_frames = max(hunk.end_a, hunk.end_b) - hunk.start_a
            hunk_num_chars = transform(hunk_num_frames)
            dl_start_idx = transform(hunk.start_a)  # Identical for .start_b
            dl_end_idx = dl_start_idx + hunk_num_chars

            # hunk is entirely out of viewport:
            if dl_end_idx < 0:
                continue
            elif dl_start_idx > len(diff_line):
                break

            if is_b:
                frames_inserted = hunk.end_b - hunk.start_b
            else:
                frames_inserted = hunk.end_a - hunk.start_a

            insertion_str = '+' * int(
                (frames_inserted / hunk_num_frames) * hunk_num_chars)
            insertion_str = list(insertion_str.ljust(hunk_num_chars))
            insertion_str[0] = self._insertion_fmt + insertion_str[0]
            insertion_str[-1] += self._common_fmt
            overlay_lists(diff_line, insertion_str, dl_start_idx)
        return self._common_fmt(''.join(diff_line))

    def _draw(self):
        duration_a = fmt_seconds(duration(self._psf_a))
        duration_b = fmt_seconds(duration(self._psf_b))
        duration_width = max(len(duration_a), len(duration_b))
        diff_line_width = self._terminal.width - duration_width - 1
        lines = [
            self._make_diff_line(
                self._diff, diff_line_width) + ' ' + duration_a,
            self._make_diff_line(
                self._diff, diff_line_width, is_b=True) + ' ' +
            duration_b
        ]
        self._terminal.print_lines(lines)


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
