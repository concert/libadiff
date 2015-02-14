#include "../include/bdiff.h"
#include "chunk.h"
#include "hunk.h"
#include <stdio.h>
#include <assert.h>

static unsigned const max_chunk_size = 10000;

hunk * const bdiff(
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

static inline unsigned min(unsigned a, unsigned b) {
    return (a < b) ? a : b;
}

static unsigned find_start_delta(
        data_fetcher const df, unsigned const sample_size, char * const buf_a,
        char * const buf_b, void * const a, void * const b) {
    unsigned delta_offset = 0;
    while (1) {
        unsigned const n_read_a = df(a, buf_a, buf_size/sample_size);
        unsigned const n_read_b = df(b, buf_b, buf_size/sample_size);
        unsigned const min_read = min(n_read_a, n_read_b);
        for (
                unsigned byte_idx = 0; byte_idx < (sample_size * min_read);
                byte_idx++) {
            if (buf_a[byte_idx] != buf_b[byte_idx]) {
                return (byte_idx / sample_size) + delta_offset;
            }
        }
        if (n_read_a != n_read_b) {
            return min_read + delta_offset;
        } else {
            delta_offset += min_read;
        }
    }
}

static unsigned find_end_delta(
        data_fetcher const df, unsigned const sample_size, unsigned end_delta,
        char * const buf_a, char * const buf_b, void * const a,
        void * const b) {
    unsigned loop_start_delta = end_delta;
    while (loop_start_delta) {
        unsigned const n_read = df(
            a, buf_a, min(buf_size/sample_size, loop_start_delta));
        assert(df(b, buf_b, n_read) == n_read);
        for (
                unsigned byte_idx = 0; byte_idx < (sample_size * n_read);
                byte_idx++) {
            if (buf_a[byte_idx] != buf_b[byte_idx]) {
                end_delta = loop_start_delta - (byte_idx / sample_size) - 1;
            }
        }
        loop_start_delta -= n_read;
    }
    return end_delta;
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
    for (; rough_hunks != NULL; rough_hunks = rough_hunks->next) {
        ds(a, rough_hunks->a.start);
        ds(b, rough_hunks->b.start);
        char buf_a[buf_size], buf_b[buf_size];
        unsigned const start_delta = find_start_delta(
            df, sample_size, buf_a, buf_b, a, b);
        unsigned end_delta = min(
            rough_hunks->a.end - (rough_hunks->a.start + start_delta),
            rough_hunks->b.end - (rough_hunks->b.start + start_delta));
        end_delta = min(end_delta, max_chunk_size);
        if (end_delta) {
            ds(a, rough_hunks->a.end - end_delta);
            ds(b, rough_hunks->b.end - end_delta);
            end_delta = find_end_delta(
                df, sample_size, end_delta, buf_a, buf_b, a, b);
        }
        append_hunk(
            &precise_hunks_head, &precise_hunks_tail,
            rough_hunks->a.start + start_delta,
            rough_hunks->a.end - end_delta,
            rough_hunks->b.start + start_delta,
            rough_hunks->b.end - end_delta);
    }
    return precise_hunks_head;
}
