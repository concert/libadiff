#include "block.h"
#include <stdlib.h>
#include "hash_counting_table.h"

static block * block_new(
        block * prev, unsigned const start, unsigned const end,
        unsigned const other_start) {
    block * new_block = malloc(sizeof(block));
    *new_block = (block) {
        .start = start, .end = end, .other_start = other_start};
    if (prev != NULL) {
        prev->next = new_block;
    }
    return new_block;
}

/*! Takes two lists of content-defined data chunks and returns contiguous
 * sections of the first that are not in the second.
 *
 * We use a hash table to store counts of the data chunks' hashes from the
 * second list ("theirs"), so that we can quickly verify whether they appear in
 * the first ("ours"). (We may have repetitive data where multiple chunks have
 * the same hash value, which is why we need the counts, rather than just a
 * set of the hashes.)
 *
 * When we encounter data that is unique to the first (which here we determine
 * by noticing gaps in the data common to both lists of chunks), we create
 * block objects recording those portions of data, which is what we eventually
 * return.
 */
blocks unique_blocks(restrict chunks ours, restrict chunks theirs) {
    if (ours == NULL) {
        return NULL;
    }

    chunks const theirs_head = theirs;
    hash_counting_table their_hashes = hash_counting_table_new();
    while (theirs != NULL) {
        hash_counting_table_inc(
            their_hashes, theirs->hash);
        theirs = theirs->next;
    }
    theirs = theirs_head;

    unsigned their_last_common = 0;
    block * head = NULL, * tail = NULL;
    chunk const zero = {.start = 0, .end = 0};
    chunk const * previous_common = &zero;
    while (ours->next != NULL) {
        if (hash_counting_table_dec(their_hashes, ours->hash)) {
            // We're processing a chunk common to ours and theirs
            if (ours->start != previous_common->end) {
                // There's a gap, we skipped over some chunks unique to ours
                tail = block_new(
                    tail, previous_common->end, ours->start,
                    their_last_common);
                if (head == NULL) {
                    head = tail;
                }
            }
            previous_common = ours;
            while (theirs->hash != previous_common->hash) {
                // I want to be able to decrement here so that the stuff in the
                // table is strictly what remains, however right now I don't
                // think we know how many times we've already decremented the
                // counters. We might need a peek or something for above.
                theirs = theirs->next;
            }
            their_last_common = theirs->end;
        }
        ours = ours->next;
    }
    const unsigned hunk_end = hash_counting_table_dec(
        their_hashes, ours->hash) ?  ours->start : ours->end;
    if (hunk_end != previous_common->end) {
        // Unique block at end
        tail = block_new(
            tail, previous_common->end, hunk_end, their_last_common);
        if (head == NULL) {
            head = tail;
        }
    }
    hash_counting_table_destroy(their_hashes);
    return head;
}

inline void block_free(block * head) {
    while (head != NULL) {
        block * prev = head;
        head = head->next;
        free(prev);
    }
}
