from functools import wraps
from collections import namedtuple, MutableSequence, defaultdict
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
    '''Vector like class to help process multiple related values together.
    Provides useful functionality like mapped attribute lookups and
    distributive calling.
    '''

    __slots__ = '_data',

    def __init__(self, a, b=UNASSIGNED):
        '''Takes either values for a and b or a value to repeat for both'''
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
        return self.map(lambda obj: getattr(obj, name))

    def __call__(self, *args, **kwargs):
        return self.map(lambda obj: obj(*args, **kwargs))

    def distribute(self, *iterables):
        '''Like ab(arg1, arg2...), but we distribute the (iterable) arguments to
        each respective ab:
          ab[0](arg1[0], arg2[0]...)
          ab[1](arg1[1], arg2[1]...)
        '''
        iterators = tuple(map(iter, iterables))
        f_on_next_i = lambda f: f(*map(next, iterators))
        return self.map(f_on_next_i)

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
        # Caution: http://bugs.python.org/issue4806 can give odd results if you
        # just plough the map into cls():
        args = tuple(map(func, iterable))
        return cls(*args)

    def map(self, func, *args, **kwargs):
        return self.from_map(lambda i: func(i, *args, **kwargs), self)

    def reversed(self):
        return self.__class__(*reversed(self))


class Fmt(namedtuple('Fmt', ('str', 'len'))):
    def __str__(self):
        return self.str


class FormattedList(MutableSequence):
    def __init__(self, string, normal):
        self.normal = normal
        self._content = list(string)
        self._fmts = defaultdict(list)

    def __len__(self):
        return len(self._content)

    def __getitem__(self, i):
        return tuple(self._fmts.get(i, ())), self._content[i]

    @staticmethod
    def _crop_formats(fmts, remaining_len):
        for fmt in fmts:
            if fmt.len > remaining_len:
                fmt = fmt._replace(len=remaining_len)
            yield fmt

    def __setitem__(self, i, value):
        fmts, c = value
        self._content[i] = c
        self._fmts[i][:] = self._crop_formats(fmts, len(self) - i)

    def format(self, start, end, fmt_str):
        if not 0 <= start < len(self):
            raise IndexError('Start index out of range')
        if not start <= end <= len(self):
            raise IndexError('End index out of range')
        self._fmts[start].append(Fmt(fmt_str, end - start))

    @staticmethod
    def _change_len_fmts(fmts, predicate, delta):
        for fmt in fmts:
            if predicate(fmt):
                yield fmt._replace(len=fmt.len + delta)
            else:
                yield fmt

    def _guard_index(self, i):
        if not 0 <= i < len(self):
            raise IndexError('Index out of range')

    def __delitem__(self, i):
        self._guard_index(i)
        for j in range(i):
            fmts, c = self[j]
            fmts = self._change_len_fmts(fmts, lambda f: j + f.len > i, -1)
            self[j] = fmts, c
        if self._fmts[i]:
            self._fmts[i + 1].extend(self._change_len_fmts(
                self._fmts[i], lambda _: True, -1))
        for j in range(i, len(self) - 1):
            self[j] = self[j + 1]
        self._content.pop()

    def insert(self, i, char):
        self._guard_index(i)
        for j in range(i):
            fmts, c = self[j]
            fmts = self._change_len_fmts(fmts, lambda f: j + f.len > i, 1)
            self[j] = fmts, c
        self._content.insert(i, char)
        for j in reversed(range(i, len(self) - 1)):
            if self._fmts[j - 1]:
                self._fmts[j] = self._fmts[j - 1]
                del self._fmts[j - 1]

    @staticmethod
    def _start_fmts(stack, ends, i, new_fmts):
        for fmt in new_fmts:
            stack.append(fmt)
            ends[i + fmt.len].append(fmt)
            yield fmt.str

    @staticmethod
    def _end_fmts(stack, ends, i):
        if i in ends:
            for fmt in ends.pop(i):
                stack.remove(fmt)
            yield from map(str, stack)

    def _str_iter(self):
        stack = [self.normal]
        ends = defaultdict(list)
        for i, (fmts, c) in enumerate(self):
            yield from self._end_fmts(stack, ends, i)
            yield from self._start_fmts(stack, ends, i, fmts)
            yield c
        if len(self):
            yield from self._end_fmts(stack, ends, i + 1)

    def __str__(self):
        return ''.join(self._str_iter())


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
        if hours:
            return 0
        elif minutes:
            return 1
        else:
            return 2

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
        precision excluding leading zero components.
        '''
        return self._formats[self.precision].format(t=self)
