#include "../include/bdiff.h"
#include "chunk.h"
#include "block.h"
#include "diff_hunk.h"
#include <stdio.h>

int main() {
    char const * const a = "foobar";
    chunk * a_chunks = NULL, * a_tail = NULL;
    a_chunks = a_tail = chunk_new(a, a_tail, 0, 3);
    a_tail = chunk_new(a, a_tail, 3, 6);
    char const * const b = "foobarbar";
    chunk * b_chunks = NULL, * b_tail = NULL;
    b_chunks = b_tail = chunk_new(b, b_tail, 0, 3);
    b_tail = chunk_new(b, b_tail, 3, 6);
    b_tail = chunk_new(b, b_tail, 6, 9);

    blocks unique_a = unique_blocks(a_chunks, b_chunks);
    blocks unique_b = unique_blocks(b_chunks, a_chunks);
    printf("a: %p b: %p\n", (void*)  unique_a, (void*) unique_b);
    diff_hunk * dh = pair_blocks(unique_a, unique_b);
    printf("bs: %u be: %u\n", dh->b->start, dh->b->end);
    diff_hunk_free(dh);
    chunk_free(unique_a);
    chunk_free(unique_b);
}
