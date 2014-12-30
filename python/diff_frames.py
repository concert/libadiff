#!/usr/bin/env python
import os
import argh
import asyncio
import subprocess

import pysndfile

from terminal import Terminal, Keyboard

bdiff_frames_path = os.path.join(
    os.path.dirname(__file__), os.pardir, 'diff_frames')


class DiffApp:
    def __init__(self, filename_a, filename_b, loop=None):
        self.filename_a = filename_a
        self.filename_b = filename_b
        self._psf_a = None
        self._psf_b = None

        self._loop = loop or asyncio.get_event_loop()
        self._terminal = Terminal()
        self._loop.add_reader(self._terminal.infile, self._handle_input)
        self._keyboard = Keyboard()
        self.bindings = {}

    @asyncio.coroutine
    def __call__(self):
        self._psf_a = pysndfile.PySndfile(self.filename_a)
        self._psf_b = pysndfile.PySndfile(self.filename_b)
        result = yield from self._do_diff()
        print(result, self._psf_a.frames(), self._psf_b.frames())
        with self._terminal.unbuffered_input(), (
                self._terminal.nonblocking_input()), (
                self._terminal.hidden_cursor()):
            while True:
                yield from asyncio.sleep(1)

    def _handle_input(self):
        key = self._keyboard[self._terminal.infile.read()]
        action = self.bindings.get(key, lambda: print(key))
        action()

    @asyncio.coroutine
    def _do_diff(self):
        process = yield from asyncio.create_subprocess_exec(
            bdiff_frames_path, self.filename_a, self.filename_b,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = yield from process.communicate()
        if process.returncode:
            raise RuntimeError(stderr)
        else:
            return [tuple(map(int, l.split())) for l in stdout.splitlines()]


def diff(filename_a, filename_b):
    '''Diffs the two given audiofiles and throws away the return value so
    blasted argh doesn't try to print the whole bally lot!
    '''
    loop = asyncio.get_event_loop()
    app = DiffApp(filename_a, filename_b)
    loop.run_until_complete(app())
    loop.close()

argh.dispatch_command(diff)
