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
        if ((a != NULL) && (b != NULL) && (a->hash == b->hash)) {
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
                tail = hunk_new(tail, &(a->v), NULL);
                b_offset += a->end - a->start;
                a = a->next;
            } else {
                //  b is next insertion in our virtual stream of diffs
                tail = hunk_new(tail, NULL, &(b->v));
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
