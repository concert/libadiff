from functools import wraps


def caching_property(method):
    attr_name = '_' + method.__name__
    @property
    @wraps(method)
    def new_property(self):
        if not getattr(self, attr_name):
            setattr(self, attr_name, method(self))
        return getattr(self, attr_name)
    return new_property


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


class Time:
    '''Formats the given time in seconds to an appropriate prescision excluding
    leading zero components. Edge cases around overflowing make this
    non-trivial.
    '''

    __slots__ = 'hours', 'minutes', 'seconds'

    def __init__(self, seconds):
        self.hours, self.minutes, self.seconds = self._round_hms(
            *self._split(seconds))

    @staticmethod
    def _split(seconds):
        hours, remainder = divmod(seconds, 3600)
        minutes, seconds = divmod(remainder, 60)
        return hours, minutes, seconds

    @staticmethod
    def _round_pair(big, small, precision):
        if round(small, precision) == 60:
            big += 1
            small = 0.0
        return big, small

    @staticmethod
    def _select(hours, minutes, seconds, a, b, c):
        if not hours:
            if not minutes:
                return c
            return b
        return a

    def _round_hms(self, hours, minutes, seconds):
        second_prec = self._select(hours, minutes, seconds, 0, 1, 2)
        minutes, seconds = self._round_pair(minutes, seconds, second_prec)
        hours, minutes = self._round_pair(hours, minutes, 0)
        return hours, minutes, seconds

    def __str__(self):
        fmt_str = self._select(
            self.hours, self.minutes, self.seconds,
            '{hours:.0f}:{minutes:02.0f}:{seconds:02.0f}',
            '{minutes:.0f}:{seconds:04.1f}',
            '{seconds:.2f}')
        return fmt_str.format(
            hours=self.hours, minutes=self.minutes, seconds=self.seconds)


def fmt_seconds(seconds):
    return str(Time(seconds))
