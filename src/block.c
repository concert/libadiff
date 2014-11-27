#include "block.h"
#include "hash_counting_table.h"

static block * block_new(
        block * prev, chunk const * const previous_common,
        const unsigned end) {
    return chunk_new(
        prev, previous_common->end, end, previous_common->hash);
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

    hash_counting_table their_hashes = hash_counting_table_new();
    while (theirs != NULL) {
        hash_counting_table_inc(
            their_hashes, theirs->hash);
        theirs = theirs->next;
    }

    block * head = NULL, * tail = NULL;
    chunk const zero = {.start = 0, .end = 0};
    chunk const * previous_common = &zero;
    while (ours->next != NULL) {
        if (hash_counting_table_dec(their_hashes, ours->hash)) {
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
    const unsigned hunk_end = hash_counting_table_dec(
        their_hashes, ours->hash) ?  ours->start : ours->end;
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
