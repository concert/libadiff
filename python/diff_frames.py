#!/usr/bin/env python
import os
import sys
import argh
import asyncio
import subprocess
from collections import namedtuple

import pysndfile

from terminal import Terminal, Keyboard

bdiff_frames_path = os.path.join(
    os.path.dirname(__file__), os.pardir, 'diff_frames')

Hunk = namedtuple('Hunk', ('start_a', 'end_a', 'start_b', 'end_b'))


class DiffApp:
    def __init__(self, filename_a, filename_b, loop=None):
        self.filename_a = filename_a
        self.filename_b = filename_b
        self._diff = None
        self._len = None

        self._loop = loop or asyncio.get_event_loop()
        self._terminal = Terminal()
        self._loop.add_reader(self._terminal.infile, self._handle_input)
        self._keyboard = Keyboard()
        self.bindings = {}

    @asyncio.coroutine
    def __call__(self):
        diff = yield from self._do_diff()
        self._diff, a_offset, b_offset = self._process_diff(diff)
        psf_a = pysndfile.PySndfile(self.filename_a)
        psf_b = pysndfile.PySndfile(self.filename_b)
        self._len = max(
            psf_a.frames() + b_offset,
            psf_b.frames() + a_offset)
        with self._terminal.unbuffered_input(), (
                self._terminal.nonblocking_input()), (
                self._terminal.hidden_cursor()):
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

    def _draw_lines(self, lines):
        print('\n'.join(lines), end='')
        print(self._terminal.move_up * (len(lines) + 1)) # Wha? Why + 1?!
        print(self._terminal.move_x(1))

    def _draw(self):
        if self._diff:
            print('\n'.join(map(repr, self._diff)), self._len)


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
