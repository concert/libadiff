#!/usr/bin/env python
import os
import argh
import subprocess

bdiff_frames_path = os.path.join(
    os.path.dirname(__file__), os.pardir, 'diff_frames')


def _diff(filename_a, filename_b):
    p = subprocess.Popen(
        (bdiff_frames_path, filename_a, filename_b),
        stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    # FIXME: might be a cleaner way to use subprocess...
    if p.returncode:
        raise RuntimeError(err)
    return [tuple(map(int, l.split())) for l in out.splitlines()]


def diff(filename_a, filename_b):
    '''Diffs the two given audiofiles and throws away the return value so
    blasted argh doesn't try to print the whole bally lot!
    '''
    result = _diff(filename_a, filename_b)
    from pprint import pprint
    pprint(result)

argh.dispatch_command(diff)
