#include "hunk.h"
#include <stdlib.h>
#include "hash_counting_table.h"

static inline hash_counting_table create_hash_counting_table(chunks c) {
    hash_counting_table c_hashes = hash_counting_table_new();
    while (c != NULL) {
        hash_counting_table_inc(
            c_hashes, c->hash);
        c = c->next;
    }
    return c_hashes;
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
        if (*tail != NULL) {
            (*tail)->next = new_hunk;
        }
        *tail = new_hunk;
        if (*head == NULL) {
            *head = *tail;
        }
    }
}

hunk * hunk_factory(chunks a, chunks b) {
    hash_counting_table b_hashes = create_hash_counting_table(b);

    hunk * head = NULL, * tail = NULL;
    unsigned hunk_start_a = 0, hunk_start_b = 0;
    // Initial zero-length chunks to avoid nasty first iteration logic:
    chunk zero_a = {.start = 0, .end = 0, .next = a};
    chunk zero_b = {.start = 0, .end = 0, .next = b};
    a = &zero_a;
    b = &zero_b;
    for (; a->next != NULL; a = a->next) {
        if (hash_counting_table_get(b_hashes, a->next->hash)) {
            // We're processing a chunk common to a and b
            while (b->next->hash != a->next->hash) {
                b = b->next;
                hash_counting_table_dec(b_hashes, b->hash);
            }

            possibly_append_hunk(
                &head, &tail, hunk_start_a, a->next->start,
                hunk_start_b, b->next->start);

            hunk_start_a = a->next->end;
            hunk_start_b = b->next->end;

            hash_counting_table_dec(b_hashes, b->next->hash);
            b = b->next;
        }
    }
    while (b->next != NULL) {
        b = b->next;
    }

    possibly_append_hunk(
        &head, &tail, hunk_start_a, a->end, hunk_start_b, b->end);

    hash_counting_table_destroy(b_hashes);
    return head;
}

void hunk_free(hunk * head) {
    while (head != NULL) {
        hunk * prev = head;
        head = head->next;
        free(prev);
    }
}
