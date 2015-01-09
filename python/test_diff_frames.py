from unittest import TestCase
from mock import Mock

from pysndfile import PySndfile
from terminal import LinePrintingTerminal
from diff_frames import (
    fmt_seconds, duration, DiffApp, Hunk, NormalisedHunk, overlay_lists)


class TestHunk(TestCase):
    def test_eq(self):
        self.assertEqual(Hunk(1, 2, 3, 4), Hunk(1, 2, 3, 4))
        self.assertNotEqual(Hunk(1, 2, 3, 4), Hunk(5, 6, 7, 8))


class TestNormalisedHunk(TestCase):
    def test_len(self):
        self.assertEqual(len(NormalisedHunk(1, 5, 1, 10)), 9)


class TestDiffFrames(TestCase):
    def setUp(self):
        self.app = DiffApp(
            'some_a', 'some_b',
            terminal=LinePrintingTerminal(force_styling=None))

    def test_duration(self):
        mock_snd_file = Mock(spec=PySndfile)
        mock_snd_file.frames.return_value = 44100 * 42 + 441
        mock_snd_file.samplerate.return_value = 44100
        self.assertEqual(duration(mock_snd_file), 42.01)

    def test_fmt_seconds(self):
        self.assertEqual(fmt_seconds(1.567), '1.57')
        self.assertEqual(fmt_seconds(59.899), '59.90')
        self.assertEqual(fmt_seconds(59.999), '1:00.0')
        self.assertEqual(fmt_seconds(61.789), '1:01.8')
        self.assertEqual(fmt_seconds(3599.99), '1:00:00')
        self.assertEqual(fmt_seconds(3601.5), '1:00:02')

    def test_overlay_lists(self):
        data = (
            (0, ('---', '+--', '++-', '+++', '+++')),
            (1, ('---', '-+-', '-++', '-++', '-++')),
            (5, ('---', '---', '---', '---', '---')),
            (-1, ('---', '---', '+--', '++-', '+++')),
            (-4, ('---', '---', '---', '---', '---')),
        )
        for offset, expecteds in data:
            for i, expected in enumerate(expecteds):
                base = ['-'] * 3
                new = ['+'] * i
                overlay_lists(base, new, offset)
                self.assertEqual(base, list(expected))

    unprocessed_diff = (
        Hunk(0, 0, 0, 1000),
        Hunk(2000, 5000, 3000, 7000),
        Hunk(6000, 7000, 8000, 8000),
    )

    processed_diff = (
        NormalisedHunk(0, 0, 0, 1000),
        NormalisedHunk(3000, 6000, 3000, 7000),
        NormalisedHunk(11000, 12000, 11000, 11000),
    )
    diff_line_a = '    --------++++++++++++    ----------------++++'
    diff_line_b = '++++--------++++++++++++++++----------------    '

    def test_process_diff(self):
        self.assertEqual(
            self.app._process_diff(self.unprocessed_diff),
            (self.processed_diff, 4000, 5000))

    def _diff_line_helper(self, app, expected_a, expected_b):
        app._len = 12000  # each 1000 frames is 4 characters at 48 width
        result = app._make_diff_line(self.processed_diff, 48)
        self.assertEqual(repr(result), repr(expected_a))
        result = app._make_diff_line(
            self.processed_diff, 48, is_b=True)
        self.assertEqual(repr(result), repr(expected_b))

    def test_make_diff_line_plain(self):
        self._diff_line_helper(
            self.app, expected_a=self.diff_line_a, expected_b=self.diff_line_b)

    def test_make_diff_line_colour(self):
        app = DiffApp('some_a', 'some_b')
        ins = app._insertion_fmt
        com = app._common_fmt
        self._diff_line_helper(
            app,
            expected_a=com(
                '{ins}    {com}--------{ins}++++++++++++    {com}-------------'
                '---{ins}++++{com}').format(ins=ins, com=com),
            expected_b=com(
                '{ins}++++{com}--------{ins}++++++++++++++++{com}-------------'
                '---{ins}    {com}').format(ins=ins, com=com))

    def multiply_up(self, string, multiplier):
        return ''.join(c * multiplier for c in string)

    def test_make_diff_line_zoom(self):
        self.app._zoom = 2.0
        self._diff_line_helper(
            self.app,
            expected_a=self.multiply_up(self.diff_line_a, 2)[:48],
            expected_b=self.multiply_up(self.diff_line_b, 2)[:48])

    def test_zoom(self):
        self.app._zoom_in()
        self.assertEqual(self.app._zoom, 1.1)
        self.app._zoom_in()
        self.assertEqual(self.app._zoom, 1.1 ** 2)
        self.app._zoom_out()
        self.assertEqual(self.app._zoom, 1.1)
        self.app._zoom_out()
        self.assertEqual(self.app._zoom, 1.0)
        self.app._zoom_out()
        self.assertEqual(self.app._zoom, 1.0)
