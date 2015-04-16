#include "../include/bdiff.h"
#include "bdiff_defs.h"
#include "chunk.h"
#include "hunk.h"

/*
 * Perform a chunk based diff of two binary streams.
 * This method has algorithmic complexity
 * O(length_stream_a + length_stream_b).
 */
hunk * const bdiff_rough(
        unsigned const sample_size, data_fetcher const df, void * const a,
        void * const b) {
    chunks a_chunks = split_data(
        sample_size, df, a, min_chunk_size, max_chunk_size);
    chunks b_chunks = split_data(
        sample_size, df, b, min_chunk_size, max_chunk_size);
    hunk * const h = diff_chunks(a_chunks, b_chunks);
    chunk_free(a_chunks);
    chunk_free(b_chunks);
    return h;
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
