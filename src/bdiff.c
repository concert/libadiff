#include "../include/bdiff.h"
#include "chunk.h"
#include "hunk.h"
#include <stdio.h>

hunk * const bdiff(
        unsigned const sample_size, data_fetcher const df, void * const a,
        void * const b) {
    chunks a_chunks = split_data(sample_size, df, a, 1, 100000);
    chunks b_chunks = split_data(sample_size, df, b, 1, 100000);
    hunk * const h = diff_chunks(a_chunks, b_chunks);
    chunk_free(a_chunks);
    chunk_free(b_chunks);
    return h;
}
