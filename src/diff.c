#include "../include/diff.h"
#include "chunk.h"
#include "block.h"
#include "hash_multiset.h"
#include "diff_hunk.h"
#include <stdio.h>
#include <assert.h>

blocks unique_blocks(restrict chunks ours, restrict chunks theirs) {
    if (ours == NULL) {
        return NULL;
    }

    hash_multiset their_hashes = hash_multiset_new();
    while (theirs != NULL) {
        hash_multiset_insert(
            their_hashes, theirs->hash);
        theirs = theirs->next;
    }

    block * head = NULL, * tail = NULL;
    chunk const zero = {.start = 0, .end = 0};
    chunk const * previous_common = &zero;
    while (ours->next != NULL) {
        if (hash_multiset_pop(their_hashes, ours->hash)) {
            // We're processing a chunk common to ours and theirs
            if (ours->start != previous_common->end) {
                // There's a gap, we skipped over some chunks unique to ours
                tail = block_new(tail, previous_common, ours->start);
                if (head == NULL) {
                    head = tail;
                }
            }
            previous_common = ours;
        }
        ours = ours->next;
    }
    const unsigned hunk_end = hash_multiset_pop(their_hashes, ours->hash) ?
        ours->start : ours->end;
    if (hunk_end != previous_common->end) {
        // Unique block at end
        tail = block_new(tail, previous_common, hunk_end);
        if (head == NULL) {
            head = tail;
        }
    }
    hash_multiset_destroy(their_hashes);
    return head;
}

diff_hunk * pair_blocks(blocks a, blocks b) {
    diff_hunk * head = NULL, * tail = NULL;
    unsigned a_offset = 0, b_offset = 0;
    unsigned a_start = 0, b_start = 0;
    while ((a != NULL) || (b != NULL)) {
        if (a != NULL) {
            a_start = a->start + a_offset;
        }
        if (b != NULL) {
            b_start = b->start + b_offset;
        }
        if ((a != NULL) && (b != NULL) && (a->hash == b->hash)) {
            assert(a_start == b_start);
            tail = diff_hunk_new(tail, &(a->v), &(b->v));
            unsigned const a_length =
                a->end - a->start, b_length = b->end - b->start;
            unsigned const hunk_len = (a_length > b_length) ?
                a_length : b_length;
            a_offset += hunk_len - a_length;
            b_offset += hunk_len - b_length;
            a = a->next;
            b = b->next;
        } else {
            if ((b == NULL) || (a_start > b_start)) {
                //  a is next insertion in our virtual stream of diffs
                tail = diff_hunk_new(tail, &(a->v), NULL);
                b_offset += a->end - a->start;
                a = a->next;
            } else {
                //  b is next insertion in our virtual stream of diffs
                tail = diff_hunk_new(tail, NULL, &(b->v));
                a_offset += b->end - b->start;
                b = b->next;
            }
        }
        if ((head == NULL) && (tail != NULL)) {
            head = tail;
        }
    }
    return head;
}

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
