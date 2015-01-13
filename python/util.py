from functools import wraps
from collections import namedtuple
from itertools import starmap
from operator import add, sub, eq


def cache(method):
    attr_name = '_' + method.__name__
    @wraps(method)
    def new_method(self):
        if not hasattr(self, attr_name):
            setattr(self, attr_name, method(self))
        return getattr(self, attr_name)
    return new_method


def overlay_lists(base, new, offset):
    '''Mutates the given base list by overlaying the new list on top at a given
    offset.
    '''
    if offset + len(new) >= 0:
        for i in range(len(new)):
            base_idx = i + offset
            if 0 <= base_idx < len(base):
                base[base_idx] = new[i]
            elif base_idx >= len(base):
                break


class Clamped:
    '''Descriptor that limits the range of an attribute, given two callbacks
    values or callbacks for minimum values.
    '''
    def __init__(self, min_, max_):
        self.min = min_ if callable(min_) else lambda _: min_
        self.max = max_ if callable(max_) else lambda _: max_

    @property
    def identifier(self):
        return '_{:x}'.format(id(self))

    def __get__(self, obj, type_):
        if obj is None:
            return self
        else:
            return getattr(obj, self.identifier)

    @staticmethod
    def _limit(value, min_, max_):
        if min_ is None and max_ is None:
            return value
        if min_ is None:
            min_ = min(value, max_)
        if max_ is None:
            max_ = max(value, min_)
        return sorted((value, min_, max_))[1]

    def __set__(self, obj, value):
        min_ = self.min(obj)
        max_ = self.max(obj)
        setattr(obj, self.identifier, self._limit(value, min_, max_))

UNASSIGNED = object()


class AB:
    __slots__ = '_data',

    def __init__(self, a, b=UNASSIGNED):
        'Takes either values for a and b or a value to repeat for both'
        if b is UNASSIGNED:
            b = a
        self._data = [a, b]

    def __repr__(self):
        return '{}({}, {})'.format(self.__class__.__name__, *self)

    def __iter__(self):
        return iter(self._data)

    def __len__(self):
        return len(self._data)

    def __getitem__(self, i):
        return self._data[i]

    def __setitem__(self, i, value):
        self._data[i] = value

    def __getattr__(self, name):
        return self.from_map(lambda obj: getattr(obj, name), self)

    def __call__(self, *args, **kwargs):
        return self.from_map(lambda obj: obj(*args, **kwargs), self)

    def __add__(self, other):
        return self.__class__(*starmap(add, zip(self, other)))

    def __sub__(self, other):
        return self.__class__(*starmap(sub, zip(self, other)))

    def __eq__(self, other):
        return len(self) == len(other) and all(starmap(eq, zip(self, other)))

    a = property(lambda self: self[0])
    b = property(lambda self: self[1])

    @classmethod
    def from_map(cls, func, iterable):
        return cls(*map(func, iterable))

    def map(self, func, *args, **kwargs):
        return self.from_map(lambda i: func(i, *args, **kwargs), self)

    def reversed(self):
        return self.__class__(*reversed(self))


class FormattedChar:
    __slots__ = 'pre_fmt', 'c', 'post_fmt'

    def __init__(self, c, pre_fmt='', post_fmt=''):
        self.pre_fmt = pre_fmt
        self.c = c
        self.post_fmt = post_fmt

    def __str__(self):
        return self.pre_fmt + self.c + self.post_fmt

    def __repr__(self):
        return 'c({!r}, {!r}, {!r})'.format(
            self.c, self.pre_fmt, self.post_fmt)


class CharList(list):
    def _format_index(self, i, name, fmt):
        try:
            setattr(self[i], name, fmt)
        except AttributeError:
            self[i] = FormattedChar(self[i], **{name: fmt})

    def pre_format_index(self, i, fmt):
        self._format_index(i, 'pre_fmt', fmt)

    def post_format_index(self, i, fmt):
        self._format_index(i, 'post_fmt', fmt)

    def __str__(self):
        return ''.join(map(str, self))


class Time(namedtuple('Time', ('hours', 'minutes', 'seconds', 'precision'))):
    '''Handle rounding and pretty formatting of time. Precision has two related
    meanings:
    1. The number of decimal places to which to render seconds
    2. Which units to show in the output
    Because of this strong coupling, you can't ask for a "hour-like" time at a
    high precision.

    Calculating the exact breakdown into hours, minutes and seconds components
    is complicated by the fact that the precision to which we want to round the
    seconds component is dependent on whether the seconds component overflows
    at a given precision!
    '''
    _formats = {
        0: '{t.hours:.0f}:{t.minutes:02.0f}:{t.seconds:02.0f}',
        1: '{t.minutes:.0f}:{t.seconds:04.1f}',
        2: '{t.seconds:.2f}',
    }

    def __new__(cls, hours, minutes, seconds, target_precision=None):
        if target_precision is None:
            precision = cls._get_precision(hours, minutes, seconds)
        else:
            precision = target_precision
        hours, minutes, seconds = cls._round(
            hours, minutes, seconds, precision)
        precision = cls._get_precision(hours, minutes, seconds)
        if target_precision is not None:
            if precision < target_precision:
                raise ValueError("Precision too high for large time")
            else:
                precision = target_precision
        return super().__new__(cls, hours, minutes, seconds, precision)

    @classmethod
    def from_seconds(cls, seconds, precision=None):
        hours, remainder = divmod(seconds, 3600)
        minutes, seconds = divmod(remainder, 60)
        return cls(hours, minutes, seconds, precision)

    @staticmethod
    def _get_precision(hours, minutes, seconds):
        if not hours:
            if not minutes:
                return 2
            return 1
        return 0

    @staticmethod
    def _overflow(big, small, precision):
        if round(small, precision) == 60:
            big += 1
            small = 0.0
        return big, small

    @classmethod
    def _round(cls, hours, minutes, seconds, precision):
        minutes, seconds = cls._overflow(minutes, seconds, precision)
        hours, minutes = cls._overflow(hours, minutes, 0)
        return hours, minutes, seconds

    def __str__(self):
        '''Formats the given time in seconds to an appropriate or given
        prescision excluding leading zero components.
        '''
        return self._formats[self.precision].format(t=self)
