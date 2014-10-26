from __future__ import division, print_function
from collections import OrderedDict, defaultdict
from blessings import Terminal

term = Terminal()

with open('data_a.txt') as f:
    data_a = f.read().replace('\n', '  ')
with open('data_b.txt') as f:
    data_b = f.read().replace('\n', '  ')


class Chunk(object):
    def __init__(self, data, prev_chunk, end):
        self._data = data  # reference to the whole original data object
        self.prev = prev_chunk
        self.end = end
        self._hash = None

    def __repr__(self):
        return '{}({!r})'.format(self.__class__.__name__, self.data)

    @property
    def start(self):
        return self.prev.end if self.prev else 0

    @property
    def data(self):
        return self._data[self.start:self.end]

    def copy(self):
        return self.__class__(self._data, self.prev, self.end)

    def __hash__(self):
        if not self._hash:
            self._hash = hash(self.data)
        return self._hash

    def __eq__(self, other):
        return hash(self) == hash(other)

    def __neq__(self, other):
        return not self == other


def _chunks(data, window_size):
    '''Uses a moving window over `data` to split it into shift-resistant
    chunks, by splitting when some bits of the window's hash are all zero.
    '''
    chunk = None
    for i in xrange(len(data) - window_size + 1):
        window_hash = hash(data[i:i + window_size])
        if window_hash & 0b11111 == 0:
            chunk = Chunk(data, prev_chunk=chunk, end=i)
            yield chunk
    yield Chunk(data, chunk, len(data))


def chunks(data, window_size=8):
    return OrderedDict((c, c) for c in _chunks(data, window_size))


def find_prev_common_chunk(chunk, other_data):
    while chunk is not None:
        if chunk in other_data:
            return chunk
        chunk = chunk.prev


class DiffHunk(object):
    def __init__(self):
        self._a = None
        self._b = None

    def _extend(self, internal_chunk, new_data_chunk):
        # FIXME: this is minging!
        if internal_chunk is None:
            internal_chunk = new_data_chunk.copy()
        internal_chunk.end = new_data_chunk.end
        return internal_chunk

    def extend_a(self, chunk):
        self._a = self._extend(self._a, chunk)

    def extend_b(self, chunk):
        self._b = self._extend(self._b, chunk)

    def __repr__(self):
        a_data = self._a.data if self._a else 'poop'
        b_data = self._b.data if self._b else 'poop'
        return '{}(a={!r}, b={!r})'.format(
            self.__class__.__name__, a_data, b_data)


def diff(data_a, data_b):
    a_chunks = chunks(data_a)
    b_chunks = chunks(data_b)
    uniq_to_a = [c for c in a_chunks if c not in b_chunks]
    uniq_to_b = [c for c in b_chunks if c not in a_chunks]
    result = defaultdict(DiffHunk)
    for chunk in uniq_to_a:
        pcc = find_prev_common_chunk(chunk, b_chunks)
        result[pcc].extend_a(chunk)
    for chunk in uniq_to_b:
        pcc = find_prev_common_chunk(chunk, a_chunks)
        result[pcc].extend_b(chunk)
    def key(i):
        common_chunk, diff_hunk = i
        if common_chunk:
            return common_chunk.end
        else:
            return 0
    result = OrderedDict(sorted(result.items(), key=key))
    return result

my_diff = diff(data_a, data_b)

scaling_factor = term.width / max(len(data_a), len(data_b))


def print_chunk(c, i, j, scaling_factor):
    num_dashes = int(round((j - i) * scaling_factor))
    print(c * num_dashes, end='')


def print_chunks(a_chunks, b_chunks, scaling_factor):
    for h, (i, j) in a_chunks.iteritems():
        if h in b_chunks:
            print_chunk('-', i, j, scaling_factor)
        else:
            print_chunk('+', i, j, scaling_factor)
    print('')


def print_mockup():
    print(
        '[A] ', term.green('+' * 15), '-' * 20,
        term.yellow('+' * 4), term.reverse_yellow('+'),  term.yellow('+++'),
        '---', ' ' * 13, '---', sep='')
    print(
        ' >  ', ' ' * 15, '             0:12.3[', ' ' * 24, ']0:28.4', sep='')
    print(
        ' B  ', ' ' * 15, '-' * 20, term.yellow('+' * 8), '---',
        term.green('+' * 13), '---', sep='')
