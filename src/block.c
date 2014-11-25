#include "block.h"
#include "hash_counting_table.h"

static block * block_new(
        block * prev, chunk const * const previous_common,
        const unsigned end) {
    return chunk_new(
        prev, previous_common->end, end, previous_common->hash);
}

blocks unique_blocks(restrict chunks ours, restrict chunks theirs) {
    if (ours == NULL) {
        return NULL;
    }

    hash_counting_table their_hashes = hash_counting_table_new();
    while (theirs != NULL) {
        hash_counting_table_insert(
            their_hashes, theirs->hash);
        theirs = theirs->next;
    }

    block * head = NULL, * tail = NULL;
    chunk const zero = {.start = 0, .end = 0};
    chunk const * previous_common = &zero;
    while (ours->next != NULL) {
        if (hash_counting_table_pop(their_hashes, ours->hash)) {
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
    const unsigned hunk_end = hash_counting_table_pop(their_hashes, ours->hash) ?
        ours->start : ours->end;
    if (hunk_end != previous_common->end) {
        // Unique block at end
        tail = block_new(tail, previous_common, hunk_end);
        if (head == NULL) {
            head = tail;
        }
    }
    hash_counting_table_destroy(their_hashes);
    return head;
}
