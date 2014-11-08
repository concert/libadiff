#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "diff.h"

typedef unsigned long hash;

typedef struct chunk {
    union {
        struct view;
        view v;
    };
    hash hash;
    struct chunk * next;
} chunk;

typedef chunk * chunks;

chunk * chunk_new_malloc(
        chunk * const prev, unsigned const start, unsigned const end,
        hash const h) {
    chunk *new_chunk = malloc(sizeof(chunk));
    *new_chunk = (chunk) {.start = start, .end = end, .hash = h};
    if (prev != NULL) {
        prev->next = new_chunk;
    }
    return new_chunk;
}

void chunk_free(chunk * head) {
    while (head != NULL) {
        chunk * prev = head;
        head = head->next;
        free(prev);
    }
}

chunk * chunk_new(
        char const * const source, chunk * const prev, const unsigned start,
        const unsigned end) {
    char substr[end - start + 1];
    snprintf(substr, end - start + 1, "%s", source + start);
    return chunk_new_malloc(prev, start, end, g_str_hash(substr));
}

typedef GHashTable * const hash_multiset;

hash_multiset hash_multiset_new() {
    return g_hash_table_new_full(
        &g_direct_hash,
        &g_direct_equal,
        /*key destroy*/ NULL,
        /*value destroy*/ NULL);
}

void hash_multiset_insert(hash_multiset set, const hash key) {
    gpointer ptr = GUINT_TO_POINTER(key);
    gpointer h = g_hash_table_lookup(set, ptr);
    //  A failed lookup comes back with NULL (0)
    g_hash_table_insert(set, ptr, h+1);
}

hash hash_multiset_pop(hash_multiset set, const hash key) {
    gpointer ptr = GUINT_TO_POINTER(key);
    hash h = GPOINTER_TO_UINT(g_hash_table_lookup(set, ptr));
    if (h == 1) {
        g_hash_table_remove(set, ptr);
    } else if (h > 1) {
        g_hash_table_insert(set, ptr, GUINT_TO_POINTER(h-1));
    }
    return GPOINTER_TO_UINT(h);
}

void hash_multiset_destroy(hash_multiset set) {
    g_hash_table_destroy(set);
}

// The hash of a block is the hash of its preceeding chunk common to both
// streams used as an anchor to align the streams.
typedef chunk block;

typedef block * blocks;

block * block_new(
        block * prev, chunk const * const previous_common,
        const unsigned end) {
    return chunk_new_malloc(
        prev, previous_common->end, end, previous_common->hash);
}

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

diff_hunk * diff_hunk_new(
        diff_hunk * const prev, view const * const a, view const * const b) {
    diff_hunk * const new_hunk = malloc(sizeof(diff_hunk));
    *new_hunk = (diff_hunk) {.a = a, .b = b};
    if (prev != NULL) {
        prev->next = new_hunk;
    }
    return new_hunk;
}

void diff_hunk_free(diff_hunk * head) {
    while (head != NULL) {
        diff_hunk * prev = head;
        head = head->next;
        free(prev);
    }
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

/*
diff file_diff(const string a, const string b) {
    // Open files
    // Make views
    // return diff_new(view_a, view_b)
}
*/
