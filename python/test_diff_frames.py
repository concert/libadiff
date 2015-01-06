from unittest import TestCase
from mock import Mock

from pysndfile import PySndfile
from diff_frames import fmt_seconds, duration


class TestDiffFrames(TestCase):
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
