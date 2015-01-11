from unittest import TestCase

from util import overlay_lists, caching_property, AB, Time


class TestUtil(TestCase):
    def test_time(self):
        v = ValueError()
        data = (
            (1.567, '1.57', '1.57', '0:01.6', '0:00:02'),
            (59.899, '59.90', '59.90', '0:59.9', '0:01:00'),
            (59.999, '1:00.0', v, '1:00.0', '0:01:00'),
            (61.789, '1:01.8', v, '1:01.8', '0:01:02'),
            (3599.99, '1:00:00', v, v, '1:00:00'),
            (3601.5, '1:00:02', v, v, '1:00:02'),
        )
        for seconds, free, fixed_2, fixed_1, fixed_0 in data:
            self.assertEqual(str(Time.from_seconds(seconds)), free)
            def check(precision, expected, msg):
                if isinstance(expected, Exception):
                    self.assertRaises(
                        type(expected), Time.from_seconds, seconds, precision)
                else:
                    self.assertEqual(
                        str(Time.from_seconds(seconds, precision)), expected,
                        msg.format(seconds))
            check(2, fixed_2, '{} fixed 2')
            check(1, fixed_1, '{} fixed 1')
            check(0, fixed_0, '{} fixed 0')

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

    def test_caching_property(self):
        self.computed = 0
        class Foo:
            def __init__(self, return_value):
                self.return_value = return_value

            @caching_property
            def something(instance):
                self.computed += 1
                return instance.return_value

        f1 = Foo('bananas')
        self.assertEqual(f1.something, 'bananas')
        self.assertEqual(self.computed, 1)
        self.assertEqual(f1.something, 'bananas')
        self.assertEqual(self.computed, 1)
        f2 = Foo('')
        self.assertEqual(f2.something, '')
        self.assertEqual(self.computed, 2)
        self.assertEqual(f2.something, '')
        self.assertEqual(self.computed, 2)

    def test_ab(self):
        self.assertEqual(AB(1, 2), AB(1, 2))
        self.assertEqual(AB(1, 2), (1, 2))
        self.assertNotEqual(AB(1, 2), AB(3, 4))
        self.assertNotEqual(AB(1, 2), (1, 2, 3))
        self.assertEqual(AB(1, 2) + AB(3, 2), AB(4, 4))
        self.assertEqual(AB(1, 2) - AB(3, 2), AB(-2, 0))
        chars = 'ab'
        ab = AB(*chars)
        for i, c in enumerate(chars):
            self.assertEqual(ab[i], c)
            self.assertEqual(getattr(ab, c), c)
            ab[i] = i
            self.assertEqual(ab[i], i)
