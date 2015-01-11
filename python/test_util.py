from unittest import TestCase

from util import overlay_lists, caching_property, AB, Time, Clamped


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


class TestAB(TestCase):
    def test_eq(self):
        self.assertEqual(AB(1, 2), AB(1, 2))
        self.assertEqual(AB(1, 2), (1, 2))
        self.assertNotEqual(AB(1, 2), AB(3, 4))
        self.assertNotEqual(AB(1, 2), (1, 2, 3))
        self.assertEqual(AB(1, 2) + AB(3, 2), AB(4, 4))
        self.assertEqual(AB(1, 2) - AB(3, 2), AB(-2, 0))

    def test_get_set_item(self):
        chars = 'ab'
        ab = AB(*chars)
        for i, c in enumerate(chars):
            self.assertEqual(ab[i], c)
            self.assertEqual(getattr(ab, c), c)
            ab[i] = i
            self.assertEqual(ab[i], i)

    def test_reversed(self):
        self.assertEqual(AB(1, 2).reversed(), AB(2, 1))

    def test_from_map(self):
        plus_one = lambda n: n + 1
        self.assertEqual(AB.from_map(plus_one, AB(1, 2)), AB(2, 3))
        self.assertRaises(TypeError, AB.from_map, plus_one, (1, 2, 3))

    def test_attr_lookup_and_call(self):
        ab = AB('hello', 'world')
        print(ab.index)
        self.assertEqual(ab.index('l'), AB(2, 3))


class TestClamped(TestCase):
    def test_limits(self):
        clamped = Clamped(2, 4)

        class A:
            attr = clamped

            def get_min(ins):
                return ins._min

            def get_max(ins):
                return ins._max
            dynamic = Clamped(get_min, get_max)

        self.assertIs(A.attr, clamped)

        a = A()
        with self.assertRaises(AttributeError):
            a.attr

        a.attr = 3
        self.assertEqual(a.attr, 3)
        a.attr -= 2
        self.assertEqual(a.attr, 2)
        a.attr = 72
        self.assertEqual(a.attr, 4)

        def check(min_, max_, ok, too_small, too_big):
            a._min = min_
            a._max = max_
            a.dynamic = ok
            self.assertEqual(a.dynamic, ok)
            a.dynamic = too_small
            self.assertEqual(a.dynamic, min_)
            a.dynamic = too_big
            self.assertEqual(a.dynamic, max_)
        check('p', 'w', 'q', 'a', 'z')
        check((1, 1), (10, 10), (5, 5), (0, 10), (11, 0))
