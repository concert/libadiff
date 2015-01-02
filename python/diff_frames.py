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
    return sndfile.frames() / sndfile.samplerate()


def fmt_seconds(t):
    hours, remainder = divmod(t, 3600)
    minutes, seconds = divmod(remainder, 60)
    if not hours:
        if not minutes:
            return '{:.2f}'.format(seconds)
        return '{:.0f}:{:.1f}'.format(minutes, seconds)
    return '{:.0f}:{:.0f}:{.0f}'.format(hours, minutes, seconds)


class DiffApp:
    def __init__(self, filename_a, filename_b, loop=None):
        self.filename_a = filename_a
        self.filename_b = filename_b
        self._psf_a = None
        self._psf_b = None
        self._diff = None
        self._len = None

        self._loop = loop or asyncio.get_event_loop()
        self._terminal = LinePrintingTerminal()
        self._common_fmt = self._terminal.yellow
        self._insertion_fmt = self._terminal.green
        self._loop.add_reader(self._terminal.infile, self._handle_input)
        self._keyboard = Keyboard()
        self.bindings = {}

    @asyncio.coroutine
    def __call__(self):
        diff = yield from self._do_diff()
        self._diff, a_offset, b_offset = self._process_diff(diff)
        self._psf_a = pysndfile.PySndfile(self.filename_a)
        self._psf_b = pysndfile.PySndfile(self.filename_b)
        self._len = max(
            self._psf_a.frames() + b_offset,
            self._psf_b.frames() + a_offset)
        with self._terminal.unbuffered_input(), (
                self._terminal.nonblocking_input()), (
                self._terminal.hidden_cursor()):
            self._draw()
            try:
                while True:
                    yield from asyncio.sleep(1)
            finally:
                print('\n', end='')  # Yuck :-(

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
        # FIXME: is there a way to do this without an intermediate list?
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

    def _make_diff_line(self, diff, tot_frames, width, is_b=False):
        diff_line = ['-'] * width
        chars_per_frame = width / tot_frames
        def to_chars(frames):
            return int(chars_per_frame * frames)
        for hunk in diff:
            hunk_frames = max(hunk.end_a, hunk.end_b) - hunk.start_a
            hunk_chars = to_chars(hunk_frames)
            if is_b:
                insertion_frames = hunk.end_b - hunk.start_b
                start_idx = to_chars(hunk.start_b)
            else:
                insertion_frames = hunk.end_a - hunk.start_a
                start_idx = to_chars(hunk.start_a)
            insertion_chars = int((insertion_frames / hunk_frames) * hunk_chars)
            insertion_str = '+' * insertion_chars
            insertion_str = insertion_str.ljust(hunk_chars)
            end_idx = start_idx + hunk_chars
            diff_line[start_idx:end_idx] = insertion_str
            diff_line[start_idx] = self._insertion_fmt + diff_line[start_idx]
            diff_line[end_idx] = diff_line[end_idx - 1] + self._common_fmt
        return self._common_fmt(''.join(diff_line))

    def _draw(self):
        duration_a = fmt_seconds(duration(self._psf_a))
        duration_b = fmt_seconds(duration(self._psf_b))
        duration_width = max(len(duration_a), len(duration_b))
        diff_line_width = self._terminal.width - duration_width - 1
        lines = [
            self._make_diff_line(
                self._diff, self._len, diff_line_width) + ' ' + duration_a,
            self._make_diff_line(
                self._diff, self._len, diff_line_width, is_b=True) + ' ' +
            duration_b
        ]
        self._terminal.print_lines(lines)


def diff(filename_a, filename_b):
    '''Diffs the two given audiofiles and throws away the return value so
    blasted argh doesn't try to print the whole bally lot!
    '''
    loop = asyncio.get_event_loop()
    app = DiffApp(filename_a, filename_b)
    try:
        loop.run_until_complete(app())
    except KeyboardInterrupt:
        sys.exit()
    finally:
        loop.close()

argh.dispatch_command(diff)
