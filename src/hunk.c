#include "hunk.h"
#include <stdlib.h>
#include "hash_counting_table.h"

static inline hash_counting_table create_hash_counting_table(chunks theirs) {
    hash_counting_table their_hashes = hash_counting_table_new();
    while (theirs != NULL) {
        hash_counting_table_inc(
            their_hashes, theirs->hash);
        theirs = theirs->next;
    }
    return their_hashes;
}

static inline void possibly_append_hunk(
        hunk ** const head, hunk ** const tail, unsigned const a_start,
        unsigned const a_end, unsigned const b_start, unsigned const b_end) {
    if ((a_start != a_end) || (b_start != b_end)) {
        // There's a gap, we skipped over some unique chunks
        hunk * new_hunk = malloc(sizeof(hunk));
        *new_hunk = (hunk) {
            .a = {.start = a_start, .end = a_end},
            .b = {.start = b_start, .end = b_end}};
        (*tail)->next = new_hunk;
        *tail = new_hunk;
        if (*head == NULL) {
            *head = *tail;
        }
    }
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
hunk * hunk_factory(restrict chunks ours, restrict chunks theirs) {
    if (ours == NULL) {
        return NULL;
    }

    hash_counting_table their_hashes = create_hash_counting_table(theirs);

    hunk * head = NULL, * tail = NULL;
    chunk zero = {.start = 1, .end = 0, .next = theirs};
    chunk const * previous_common = &zero;
    theirs = &zero;
    while (ours->next != NULL) {
        if (hash_counting_table_get(their_hashes, ours->hash)) {
            // We're processing a chunk common to ours and theirs
            unsigned pre_skip_start = theirs->next->start;
            while (theirs->next->hash != previous_common->hash) {
                hash_counting_table_dec(their_hashes, theirs->next->hash);
                theirs = theirs->next;
            }

            possibly_append_hunk(
                &head, &tail, ours->start, previous_common->end,
                pre_skip_start, theirs->next->start);

            previous_common = ours;
            hash_counting_table_dec(their_hashes, theirs->next->hash);
            theirs = theirs->next;
        }
        ours = ours->next;
    }
    const unsigned hunk_end = hash_counting_table_get(
        their_hashes, ours->hash) ?  ours->start : ours->end;

    unsigned b_start, b_end;
    if (theirs == NULL) {
        b_start = b_end = 0;
    } else {
        b_start = theirs->end;
        while (theirs->next != NULL) {
            theirs = theirs->next;
        }
        b_end = theirs->end;
    }

    possibly_append_hunk(
        &head, &tail, previous_common->end, hunk_end, b_start, b_end);

    hash_counting_table_destroy(their_hashes);
    return head;
}

void hunk_free(hunk * head) {
    while (head != NULL) {
        hunk * prev = head;
        head = head->next;
        free(prev);
    }
}
