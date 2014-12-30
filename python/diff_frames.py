#!/usr/bin/env python
import os
import argh
import asyncio
import subprocess

from terminal import Terminal, Keyboard

bdiff_frames_path = os.path.join(
    os.path.dirname(__file__), os.pardir, 'diff_frames')


class DiffApp:
    def __init__(self, filename_a, filename_b):
        self.filename_a = filename_a
        self.filename_b = filename_b
        self._psf_a = None
        self._psf_b = None

        self.terminal = Terminal()
        self.keyboard = Keyboard()
        self.bindings = {}

    @asyncio.coroutine
    def __call__(self):
        result = yield from self._do_diff()
        return result

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
    t = asyncio.Task(app())
    loop.run_until_complete(t)
    print(t.result())
    loop.close()

argh.dispatch_command(diff)
