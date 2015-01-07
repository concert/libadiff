from unittest import TestCase
from mock import Mock

from pysndfile import PySndfile
from terminal import LinePrintingTerminal
from diff_frames import fmt_seconds, duration, DiffApp, Hunk


class TestDiffFrames(TestCase):
    def setUp(self):
        self.app = DiffApp('some_a', 'some_b')

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


    unprocessed_diff = (
        Hunk(0, 0, 0, 1000),
        Hunk(2000, 5000, 3000, 7000),
        Hunk(6000, 7000, 8000, 8000),
    )

    processed_diff = (
        Hunk(0, 0, 0, 1000),
        Hunk(3000, 6000, 3000, 7000),
        Hunk(11000, 12000, 11000, 11000),
    )

    def test_process_diff(self):
        self.assertEqual(
            self.app._process_diff(self.unprocessed_diff),
            (self.processed_diff, 4000, 5000))

    def _diff_line_helper(self, app, expected_a, expected_b):
        result = app._make_diff_line(self.processed_diff, 12000, 50)
        self.assertEqual(repr(result), repr(expected_a))
        result = app._make_diff_line(
            self.processed_diff, 12000, 50, is_b=True)
        self.assertEqual(repr(result), repr(expected_b))

    def test_make_diff_line_plain(self):
        app = DiffApp(
            'some_a', 'some_b',
            terminal=LinePrintingTerminal(force_styling=None))
        self._diff_line_helper(
            app,
            expected_a='     -------++++++++++++     ----------------+++++',
            expected_b='+++++-------+++++++++++++++++----------------     ')

    def test_make_diff_line_colour(self):
        ins = self.app._insertion_fmt
        com = self.app._common_fmt
        self._diff_line_helper(
            self.app,
            expected_a=com(
                '{ins}     {com}-------{ins}++++++++++++     {com}-------------'
                '---{ins}+++++{com}').format(ins=ins, com=com),
            expected_b=com(
                '{ins}+++++{com}-------{ins}+++++++++++++++++{com}-------------'
                '---{ins}     {com}').format(ins=ins, com=com))
