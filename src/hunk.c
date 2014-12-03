#include "hunk.h"
#include <stdlib.h>
#include <assert.h>

static hunk * hunk_new(
        hunk * const prev, view const * const a, view const * const b) {
    hunk * const new_hunk = malloc(sizeof(hunk));
    *new_hunk = (hunk) {.a = *a, .b = *b};
    if (prev != NULL) {
        prev->next = new_hunk;
    }
    return new_hunk;
}

/*! Takes two lists of unique blocks of data and pairs them to highlight
 * changes.
 *
 * The final part of producing a diff between two streams is taking the data
 * unique to one ("A") and the data unique to the other ("B") and tying them
 * together into a meaningful comparison. We effectively want to know whether
 * either file has been modified by insertion, deletion or alteration.
 *
 * This boils down to comparing where each of the unique data blocks are
 * located in the file, and potentially paring them up. If a block is only in
 * A, then A has an insertion (or B has a deletion) and visa versa. If there
 * are changes in file A and B at a common reference point, then we have an
 * alteration between the files.
 *
 * The slightly tricky part is the fact that we need to maintain a common
 * reference as to where we are in some combination of both streams. We'll use
 * an illustration to clarify:
 *
 *    0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17
 *    +  +  +  +  -  -  -  #  #  #  -  -  +  +  +  -  -  -                (raw)
 * A
 *   [w]+  +  +  +  -  -  -[x]#  #  #  -  -[y]+  +  +  -  -  -[z]     (aligned)
 *      0  1  2  3  4  5  6   7  8  9  10 11  12 13 14 15 16 17  18 19
 *   [w]            -  -  -[x]#  #     -  -[y]         -  -  -[z]+  + (aligned)
 * B
 *    -  -  -  #  #  -  -  -  -  -  +  +                                  (raw)
 *    0  1  2  3  4  5  6  7  8  9  10 11
 *
 * The raw data for the two streams are shown at the very top and bottom. We've
 * used "-" to show data that common to both streams, "+" to show data that's
 * present only in a single stream (insertion), and "#" to show data that's
 * different between streams (insertion + deletion in the same block of data).
 * We include the indicies of each arbitrary unit of data to aid later
 * discussion.
 *
 * In the middle, we can see how we want to compare the two streams: We want to
 * line up the common bits so we can compare changes easily. To do this, we've
 * introduced some labels [w] - [z]. Each of these represents a chunk of data
 * that is present in both streams that we can use as a reference point for
 * comparing the different bits. In essence we "anchor" every change to a
 * common part of the file, so that we know, for example, the that there are
 * insertions only in A at [w], but we have a combination of insertions and
 * deleteions to get between A and B at [x]. This is why, when we found the
 * unique blocks of data from each stream, we stored a hash of it's preceeding
 * common chunk for reference.
 *
 * The indices in the middle relate to a kind of "virtual stream" that
 * contains all of the data from both data sources, and it gives as a way to
 * translate between the locations of a piece of data in A and a piece of data
 * in B. As we process the two streams of unique blocks into our final diff, we
 * maintain a_offset and b_offset, which are the differences between the real
 * data indecies into the data streams for A and B, and this "virtual stream"
 * that exists in the middle. That way, we can say categorically "does this
 * insertion in A occur before this other insertion in B?", allowing us to get
 * our insetions in the right order in our final diff output.
 *
 * You can see in the diagram that looking naively at the streams it could
 * appear like the insertion in B at [z] occurs *before* the insertion in A at
 * [y], because B's index of 10 < A's index of 12. However, the middle picture
 * after we've converted with the offsets (a=0, b=8 at this point) shows B's
 * insertion at [z] to be significantly later.
 */
hunk * pair_blocks(blocks a, blocks b) {
    hunk * head = NULL, * tail = NULL;
    unsigned a_offset = 0, b_offset = 0;
    unsigned a_start = 0, b_start = 0;
    while ((a != NULL) || (b != NULL)) {
        if (a != NULL) {
            a_start = a->start + a_offset;
        }
        if (b != NULL) {
            b_start = b->start + b_offset;
        }
        if ((a != NULL) && (b != NULL) && (a->start == b->other_start)) {
            // a and b both have different data after the common anchor (hash)
            assert(a_start == b_start);
            tail = hunk_new(tail, &(a->v), &(b->v));
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
                view v = {.start = a->other_start, .end = a->other_start};
                tail = hunk_new(tail, &(a->v), &v);
                b_offset += a->end - a->start;
                a = a->next;
            } else {
                //  b is next insertion in our virtual stream of diffs
                view v = {.start = b->other_start, .end = b->other_start};
                tail = hunk_new(tail, &v, &(b->v));
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

void hunk_free(hunk * head) {
    while (head != NULL) {
        hunk * prev = head;
        head = head->next;
        free(prev);
    }
}
