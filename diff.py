from __future__ import division, print_function
from collections import OrderedDict, deque


class ddeque(deque):
    '''Defaulting deque'''
    def popleft(self, default=(None, None)):
        try:
            return super(ddeque, self).popleft()
        except IndexError:
            return default


class Chunk(object):
    def __init__(self, data, prev_chunk, end):
        self.source = data  # reference to the whole original data object
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
        return self.source[self.start:self.end]

    def copy(self):
        return self.__class__(self.source, self.prev, self.end)

    def __hash__(self):
        if not self._hash:
            self._hash = hash(self.data)
        return self._hash

    def __eq__(self, other):
        return hash(self) == hash(other)

    def __neq__(self, other):
        return not self == other


class Block(object):
    def __init__(self, start_chunk, end_chunk):
        assert(start_chunk.source is end_chunk.source)
        self.start_chunk = start_chunk
        self.end_chunk = end_chunk

    def __repr__(self):
        return '{}({:d} {!r})'.format(
            self.__class__.__name__, self.start, self.data)

    @property
    def start(self):
        return self.start_chunk.start

    @property
    def end(self):
        return self.end_chunk.end

    @property
    def data(self):
        return self.start_chunk.source[self.start:self.end]

    def __len__(self):
        return self.end - self.start


class DiffHunk(object):
    # "Start" is the "Virtual" index, in an imaginary array containing all the
    # data, where this hunk of changes starts.
    def __init__(self, start, a=None, b=None):
        self.start = start
        self.a = a
        self.b = b

    @property
    def end(self):
        return self.start + len(self)

    def __repr__(self):
        a_data = self.a.data if self.a else ''
        b_data = self.b.data if self.b else ''
        return '{}({:d}, a={!r}, b={!r})'.format(
            self.__class__.__name__, self.start, a_data, b_data)

    def __len__(self):
        # VSize
        a_len = len(self.a) if self.a else 0
        b_len = len(self.b) if self.b else 0
        return max(a_len, b_len)


class Diff(object):
    def __init__(self, data_a, data_b, window_size=8):
        self.data_a = data_a
        self.data_b = data_b
        self._window_size = window_size
        self._cached_diff = None

    def __str__(self):
        return '\n'.join(map(str, self))

    def _chunk_gen(self, data):
        '''Uses a moving window over `data` to split it into shift-resistant
        chunks, by splitting when some bits of the window's hash are all zero.
        '''
        chunk = None
        for i in xrange(len(data) - self._window_size + 1):
            window_hash = hash(data[i:i + self._window_size])
            if window_hash & 0b11111 == 0:
                chunk = Chunk(data, prev_chunk=chunk, end=i)
                yield chunk
        yield Chunk(data, chunk, len(data))

    def _chunk_od(self, data):
        return OrderedDict((c, c) for c in self._chunk_gen(data))

    @staticmethod
    def _contiguous_block_gen(chunks):
        i = iter(chunks)
        start_chunk = i.next()
        prev_chunk = start_chunk
        for chunk in i:
            if chunk.prev != prev_chunk:
                yield Block(start_chunk, prev_chunk)
                start_chunk = chunk
            prev_chunk = chunk
        yield Block(start_chunk, prev_chunk)

    @classmethod
    def _uniq_blocks(cls, chunks, others_chunks):
        return cls._contiguous_block_gen(
            c for c in chunks if c not in others_chunks)

    def _do_diff(self):
        a_chunks = self._chunk_od(self.data_a)
        b_chunks = self._chunk_od(self.data_b)
        uniq_blocks_a = self._uniq_blocks(a_chunks, b_chunks)
        uniq_blocks_b = self._uniq_blocks(b_chunks, a_chunks)
        anchored_blocks_a = ddeque(
            (block.start_chunk.prev, block) for block in uniq_blocks_a)
        anchored_blocks_b = ddeque(
            (block.start_chunk.prev, block) for block in uniq_blocks_b)
        a_offset = b_offset = 0
        a_anchor, a_block = anchored_blocks_a.popleft()
        b_anchor, b_block = anchored_blocks_b.popleft()
        result = []
        while a_block or b_block:
            if a_block:
                # Positions in our "virtual stream" of data:
                a_start = a_block.start + a_offset
            if b_block:
                b_start = b_block.start + b_offset
            if a_anchor == b_anchor:
                # Dealing with change:
                assert a_start == b_start, '{} != {}'.format(a_start, b_start)
                hunk = DiffHunk(start=a_start, a=a_block, b=b_block)
                result.append(hunk)
                a_offset += len(hunk) - len(a_block)
                b_offset += len(hunk) - len(b_block)
                a_anchor, a_block = anchored_blocks_a.popleft()
                b_anchor, b_block = anchored_blocks_b.popleft()
            else:
                # Dealing with insertion:
                if not b_block or a_start > b_start:
                    # a_block is next insertion in our virtual stream of diffs
                    hunk = DiffHunk(start=a_start, a=a_block)
                    result.append(hunk)
                    b_offset += len(a_block)
                    a_anchor, a_block = anchored_blocks_a.popleft()
                else:
                    # b_block is next insertion in our virtual stream of diffs
                    hunk = DiffHunk(start=b_start, b=b_block)
                    result.append(hunk)
                    a_offset += len(b_block)
                    b_anchor, b_block = anchored_blocks_b.popleft()
        return result

    @property
    def _diff(self):
        if not self._cached_diff:
            self._cached_diff = self._do_diff()
        return self._cached_diff

    def __iter__(self):
        return iter(self._diff)

    def __len__(self):
        return self[-1].end

    def __getitem__(self, index):
        return self._diff[index]

diff = Diff(data_a, data_b)
print(len(diff))

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


if __name__ == '__main__':
    import sys
    from blessings import Terminal
    term = Terminal()

    with open(sys.argv[1]) as f:
        data_a = f.read().replace('\n', '  ')
    with open(sys.argv[2]) as f:
        data_b = f.read().replace('\n', '  ')
    print(Diff(data_a, data_b))
