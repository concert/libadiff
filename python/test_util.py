from unittest import TestCase

from util import (
    overlay_lists, cache, AB, Time, Clamped, FormattedChar, CharList)


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

            @property
            @cache
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
    def test_variable_constructor(self):
        self.assertEqual(AB(1), AB(1, 1))
        self.assertRaises(TypeError, AB, 1, 2, 3)

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

    def test_map(self):
        times_two = lambda n: n * 2
        ab = AB(1, 2)
        self.assertEqual(ab.map(times_two), AB(2, 4))
        times_m = lambda n, m: n * m
        self.assertEqual(ab.map(times_m, 3), AB(3, 6))

    def test_attr_lookup_and_call(self):
        ab = AB('hello', 'world')
        self.assertEqual(ab.index('l'), AB(2, 3))

    def test_distribute(self):
        f = lambda x, y: x + y
        ab = AB(f, f)
        cd = AB('c', 'd')
        ef = AB('e', 'f')
        self.assertEqual(ab.distribute(cd, ef), AB('ce', 'df'))


class TestClamped(TestCase):
    def setUp(self):
        clamped = Clamped(2, 4)
        class A:
            static = clamped
            def get_min(ins):
                return ins._min
            def get_max(ins):
                return ins._max
            dynamic = Clamped(get_min, get_max)
        self.assertIs(A.static, clamped)
        self.A = A

    def test_static_limits(self):
        a = self.A()
        with self.assertRaises(AttributeError):
            a.static

        a.static = 3
        self.assertEqual(a.static, 3)
        a.static -= 2
        self.assertEqual(a.static, 2)
        a.static = 72
        self.assertEqual(a.static, 4)

    def test_dynamic_limits(self):
        a = self.A()
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

    def test_unbounded(self):
        a = self.A()
        def check(min_, max_, on_minus, on_zero, on_plus):
            a._min = min_
            a._max = max_
            data = (-2, on_minus), (0, on_zero), (2, on_plus)
            for i, expected in data:
                a.dynamic = i
                self.assertEqual(a.dynamic, expected)
        check(None, None, -2, 0, 2)
        check(None, 1, -2, 0, 1)
        check(-1, None, -1, 0, 2)


class TestFormattedChar(TestCase):
    def test_str(self):
        f = FormattedChar('c', 'pre:', ':post')
        self.assertEqual(str(f), 'pre:c:post')


class TestCharList(TestCase):
    def setUp(self):
        self.l = CharList('hello world')

    def test_str(self):
        self.assertEqual(str(self.l), 'hello world')

    def test_format_index(self):
        for _ in range(2):
            self.l.pre_format_index(4, '[')
            self.l.post_format_index(5, ']')
            self.assertEqual(str(self.l), 'hell[o ]world')
        self.l.prepend_format_index(3, ' {')
        self.l.append_format_index(6, '} ')
        self.assertEqual(str(self.l), 'hel {l[o ]w} orld')
        self.l.prepend_format_index(4, '[')
        self.l.append_format_index(5, ']')
        self.assertEqual(str(self.l), 'hel {l[[o ]]w} orld')

    def test_get_format(self):
        self.assertEqual(self.l.get_format(3), '')
        self.l.pre_format_index(1, '[')
        self.assertEqual(self.l.get_format(3), '[')
        self.l.post_format_index(1, ']')
        self.assertEqual(self.l.get_format(3), ']')

    def test_overlay_char_lists(self):
        new_1 = CharList('bob')
        new_1.pre_format_index(1, '[')
        new_1.post_format_index(2, ']')
        new_2 = CharList('fallen off')
        new_2.pre_format_index(0, '(')
        new_2.post_format_index(7, ')')
        overlay_lists(self.l, new_1, 1)
        overlay_lists(self.l, new_2, 9)
        self.assertEqual(str(self.l), 'hb[ob]o wor(fa')
