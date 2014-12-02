#include "../include/bdiff.h"
#include "chunk.h"
#include "block.h"
#include "hunk.h"
#include <stdio.h>

hunk * const bdiff(
        unsigned const sample_size, data_fetcher const df, void * const a,
        void * const b) {
    chunks a_chunks = split_data(sample_size, df, a);
    chunks b_chunks = split_data(sample_size, df, b);
    blocks unique_a = unique_blocks(a_chunks, b_chunks);
    blocks unique_b = unique_blocks(b_chunks, a_chunks);
    chunk_free(a_chunks);
    chunk_free(b_chunks);
    hunk * const h = pair_blocks(unique_a, unique_b);
    block_free(unique_a);
    block_free(unique_b);
    return h;
}
