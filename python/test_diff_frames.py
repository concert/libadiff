from unittest import TestCase
from mock import Mock, patch

from pysndfile import PySndfile

from util import AB
from terminal import LinePrintingTerminal
from diff_frames import duration, DiffApp, Hunk, NormalisedHunk, _DrawState


class TestHunk(TestCase):
    def test_eq(self):
        self.assertEqual(Hunk(1, 2, 3, 4), Hunk(1, 2, 3, 4))
        self.assertNotEqual(Hunk(1, 2, 3, 4), Hunk(5, 6, 7, 8))


class TestNormalisedHunk(TestCase):
    def test_len(self):
        self.assertEqual(len(NormalisedHunk(1, 5, 1, 10, 0, 0)), 9)


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

    unprocessed_diff = (
        Hunk(0, 0, 0, 1000),
        Hunk(2000, 5000, 3000, 7000),
        Hunk(6000, 7000, 8000, 8000),
    )

    processed_diff = (
        NormalisedHunk(0, 0, 0, 1000, 0, 0),
        NormalisedHunk(3000, 6000, 3000, 7000, 1000, 0),
        NormalisedHunk(11000, 12000, 11000, 11000, 5000, 3000),
    )
    diff_lines = AB(
        '    --------++++++++++++    ----------------++++',
        '++++--------++++++++++++++++----------------    ')

    def test_normalise_diff(self):
        normalised_diff = tuple(
            NormalisedHunk.process_diff(self.unprocessed_diff))
        self.assertEqual(normalised_diff, self.processed_diff)

    def _diff_line_helper(self, app, expecteds):
        app._len = 12000  # each 1000 frames is 4 characters at 48 width
        draw_state = _DrawState(app)
        with patch.object(_DrawState, 'diff_width', 48):
            results = app._make_diff_reprs(draw_state, self.processed_diff)
            results = results.map(str)
            for result, expected in zip(results, expecteds):
                self.assertEqual(repr(result), repr(expected))

    def test_make_diff_repr_plain(self):
        self._diff_line_helper(self.app, self.diff_lines)

    def test_make_diff_line_colour(self):
        app = DiffApp('some_a', 'some_b')
        ins = app._insertion_fmt
        com = app._common_fmt
        expecteds = AB(
            com('{ins}    {com}--------{ins}++++++++++++    {com}-------------'
                '---{ins}++++{com}').format(ins=ins, com=com),
            com('{ins}++++{com}--------{ins}++++++++++++++++{com}-------------'
                '---{ins}    {com}').format(ins=ins, com=com))
        self._diff_line_helper(app, expecteds)

    def _multiply_up(self, string):
        return ''.join(c * 2 for c in string)[:48]

    def test_make_diff_line_zoom(self):
        self.app._zoom = 2.0
        self._diff_line_helper(
            self.app, AB(*map(self._multiply_up, self.diff_lines)))

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
