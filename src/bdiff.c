#include "../include/bdiff.h"
#include "chunk.h"
#include "hunk.h"
#include <stdio.h>
#include <assert.h>

static unsigned const max_chunk_size = 10000;

/*
 * Perform a chunk based diff of two binary streams.
 * This method has algorithmic complexity
 * O(length_stream_a + length_stream_b).
 */
hunk * const bdiff_rough(
        unsigned const sample_size, data_fetcher const df, void * const a,
        void * const b) {
    chunks a_chunks = split_data(sample_size, df, a, 10, max_chunk_size);
    chunks b_chunks = split_data(sample_size, df, b, 10, max_chunk_size);
    hunk * const h = diff_chunks(a_chunks, b_chunks);
    chunk_free(a_chunks);
    chunk_free(b_chunks);
    return h;
}

static const unsigned buf_size = 8192;

static inline unsigned min(unsigned const a, unsigned const b) {
    return (a < b) ? a : b;
}

static inline unsigned min3(
        unsigned const a, unsigned const b, unsigned const c) {
    return min(min(a, b), c);
}

static inline unsigned max(unsigned const a, unsigned const b) {
    return (a > b) ? a : b;
}

typedef struct {
    data_fetcher const df;
    data_seeker const ds;
    unsigned const sample_size;
    char * const buf_a;
    char * const buf_b;
} fss2b;

/*
 * Search through two file sections from an aligned point returning the first
 * differing sample relative to the start of the sections.
 */
static unsigned find_start_delta(
        fss2b stuff, void * const a, void * const b, unsigned const a_start,
        unsigned const b_start, unsigned const max_length) {
    unsigned delta_offset = 0;
    stuff.ds(a, a_start);
    stuff.ds(b, b_start);
    while (delta_offset < max_length) {
        unsigned const n_read_a = stuff.df(
            a, stuff.buf_a,
            min(buf_size/stuff.sample_size, max_length - delta_offset));
        unsigned const n_read_b = stuff.df(
            b, stuff.buf_b,
            min(buf_size/stuff.sample_size, max_length - delta_offset));
        unsigned const min_read = min(n_read_a, n_read_b);
        for (
                unsigned byte_idx = 0;
                byte_idx < (stuff.sample_size * min_read);
                byte_idx++) {
            if (stuff.buf_a[byte_idx] != stuff.buf_b[byte_idx]) {
                return (byte_idx / stuff.sample_size) + delta_offset;
            }
        }
        if (n_read_a != n_read_b) {
            return min_read + delta_offset;
        } else {
            if (min_read) {
                delta_offset += min_read;
            } else {
                break;
            }
        }
    }
    return delta_offset;
}

/*
 * Search through two segments that should realign by end_delta reporting the
 * position of last differing sample.
 */
static unsigned find_end_delta(
        fss2b stuff, unsigned end_delta, void * const a, unsigned const a_end,
        void * const b, unsigned const b_end) {
    stuff.ds(a, a_end - end_delta);
    stuff.ds(b, b_end - end_delta);
    unsigned loop_start_delta = end_delta;
    while (loop_start_delta) {
        unsigned const n_read = stuff.df(
            a, stuff.buf_a, min(buf_size/stuff.sample_size, loop_start_delta));
        assert(n_read != 0);
        assert(stuff.df(b, stuff.buf_b, n_read) == n_read);
        for (
                unsigned byte_idx = 0; byte_idx < (stuff.sample_size * n_read);
                byte_idx++) {
            if (stuff.buf_a[byte_idx] != stuff.buf_b[byte_idx]) {
                end_delta =
                    loop_start_delta - (byte_idx / stuff.sample_size) - 1;
            }
        }
        loop_start_delta -= n_read;
    }
    return end_delta;
}

/*
 * Scan for the start of the "fixed" sequence at the end of the given region of
 * the "sliding" sequence.
 */
static unsigned slidey_aligner(
        fss2b stuff, void * const fixed, void * const sliding,
        unsigned const fixed_start, unsigned const sliding_end,
        unsigned slide_distance) {
    for (; slide_distance; slide_distance--) {
        stuff.ds(sliding, sliding_end - slide_distance);
        stuff.ds(fixed, fixed_start);
        assert(stuff.df(fixed, stuff.buf_a, 1) == 1);
        assert(stuff.df(sliding, stuff.buf_b, 1) == 1);
        unsigned i = 0;
        for (; i < stuff.sample_size; i++) {
            if (stuff.buf_a[i] != stuff.buf_b[i]) {
                break;
            }
        }
        if (i != stuff.sample_size) {
            continue;
        }
        unsigned const start_delta = find_start_delta(
            stuff, fixed, sliding, fixed_start + 1,
            sliding_end - slide_distance + 1, slide_distance);
        if (slide_distance == start_delta) {
            break;
        }
    }
    return slide_distance;
}

/*
 * Subtract 2 unsigned numbers returning 0 when we would otherwise underflow.
 */
static inline unsigned clamped_subtract(unsigned const a, unsigned const b) {
    return (a > b) ? a - b : 0;
}

/*
 * Takes a set of "rough" hunks (start and end points aligned to chunk
 * boundaries) and reads the data around the start and end points to narrow
 * down exactly when the differing region starts and ends.
 */
hunk * const bdiff_narrow(
        hunk * rough_hunks, unsigned const sample_size, data_seeker const ds,
        data_fetcher const df, void * const a, void * const b) {
    hunk * precise_hunks_head = NULL, * precise_hunks_tail = NULL;
    unsigned end_shove_a = 0, end_shove_b = 0;
    char buf_a[buf_size], buf_b[buf_size];
    fss2b stuff = (fss2b) {
        .df = df, .ds = ds, .sample_size = sample_size, .buf_a = buf_a,
        .buf_b = buf_b};
    for (; rough_hunks != NULL; rough_hunks = rough_hunks->next) {
        if (end_shove_a) {
            end_shove_a = slidey_aligner(
                stuff, a, b, rough_hunks->a.start, precise_hunks_tail->b.end,
                min3(
                    precise_hunks_tail->b.end - precise_hunks_tail->b.start,
                    rough_hunks->a.end - rough_hunks->a.start,
                    max_chunk_size));
            precise_hunks_tail->b.end -= end_shove_a;
        } else if (end_shove_b) {
            end_shove_b = slidey_aligner(
                stuff, b, a, rough_hunks->b.start, precise_hunks_tail->a.end,
                min3(
                    precise_hunks_tail->a.end - precise_hunks_tail->a.start,
                    rough_hunks->b.end - rough_hunks->b.start,
                    max_chunk_size));
            precise_hunks_tail->a.end -= end_shove_b;
        }
        unsigned const start_delta = find_start_delta(
            stuff, a, b, rough_hunks->a.start + end_shove_a,
            rough_hunks->b.start + end_shove_b, max_chunk_size + 1);
        assert(start_delta != max_chunk_size + 1);
        end_shove_a += start_delta;
        end_shove_b += start_delta;
        if (
                ((rough_hunks->b.start + end_shove_b) ==
                    rough_hunks->b.end) &&
                ((rough_hunks->a.start + end_shove_a) ==
                    rough_hunks->a.end)) {
            // Discard a hunk that after narrowing contains nothing in either
            end_shove_a = end_shove_b = 0;
            continue;
        }
        append_hunk(
            &precise_hunks_head, &precise_hunks_tail,
            rough_hunks->a.start + end_shove_a,
            rough_hunks->a.end,
            rough_hunks->b.start + end_shove_b,
            rough_hunks->b.end);
        end_shove_a = clamped_subtract(
            precise_hunks_tail->a.start, precise_hunks_tail->a.end);
        end_shove_b = clamped_subtract(
            precise_hunks_tail->b.start, precise_hunks_tail->b.end);
        assert(!(end_shove_a && end_shove_b));
        precise_hunks_tail->a.end += max(end_shove_a, end_shove_b);
        precise_hunks_tail->b.end += max(end_shove_a, end_shove_b);
        unsigned end_delta = min3(
            precise_hunks_tail->a.end - precise_hunks_tail->a.start,
            precise_hunks_tail->b.end - precise_hunks_tail->b.start,
            max_chunk_size);
        if (end_delta) {
            end_delta = find_end_delta(
                stuff, end_delta, a, precise_hunks_tail->a.end, b,
                precise_hunks_tail->b.end);
        }
        precise_hunks_tail->a.end -= end_delta;
        precise_hunks_tail->b.end -= end_delta;
    }
    return precise_hunks_head;
}

/*
 * Find a semantically correct binary diff of two streams.
 */
hunk * const bdiff(
        unsigned const sample_size, data_seeker const ds,
        data_fetcher const df, void * const a, void * const b) {
    hunk * const rough_hunks = bdiff_rough(sample_size, df, a, b);
    hunk * const precise_hunks = bdiff_narrow(
        rough_hunks, sample_size, ds, df, a, b);
    hunk_free(rough_hunks);
    return precise_hunks;
}
