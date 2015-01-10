from unittest import TestCase

from util import fmt_seconds, overlay_lists, caching_property, AB


class TestUtil(TestCase):
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
