#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include "diff.h"

typedef unsigned long hash;

typedef struct chunk {
    union {
        struct view;
        view v;
    };
    hash hash;
} chunk;

chunk chunk_new(const buffer source, const unsigned start, const unsigned end) {
    char substr[end - start + 1];
    snprintf(substr, end - start + 1, "%s", source + start);
    //printf("substr: %u %u %s\n", start, end, substr);
    return (chunk) {
        .source = source,
        .start = start,
        .end = end,
        .data = source + start,
        .length = end - start,
        .hash = g_str_hash(substr)};
}

typedef GSList * chunks;

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

// Watch out mallocy
block * block_new(
        const buffer source, chunk const * const previous_common,
        const unsigned end) {
    const unsigned start = previous_common->end;
    block * const new_block = malloc(sizeof(block));
    *new_block = (block) {
        .source = source,
        .start = start,
        .end = end,
        .data = source + start,
        .length = end - start,
        .hash = previous_common->hash};
    return new_block;
}

typedef GSList * blocks;

blocks unique_blocks(restrict chunks ours, restrict chunks theirs) {
    hash_multiset their_hashes = hash_multiset_new();
    while (theirs != NULL) {
        hash_multiset_insert(
            their_hashes, ((chunk const * const) theirs->data)->hash);
        theirs = g_slist_next(theirs);
    }

    blocks unique_blocks = NULL;
    chunk zero = chunk_new(NULL, 0, 0);
    chunk const * previous_common = &zero;
    chunk const * chunk;
    while (ours != NULL) {
        chunk = ours->data;
        if (hash_multiset_pop(their_hashes, chunk->hash)) {
            // We're processing a chunk common to ours and theirs
            if (chunk->start != previous_common->end) {
                // There's a gap, we skipped over some chunks unique to ours
                unique_blocks = g_slist_prepend(unique_blocks, block_new(
                    chunk->source, previous_common, chunk->start));
            }
            previous_common = chunk;
        }
        ours = g_slist_next(ours);
    }
    if (chunk->start != previous_common->start) {
        // Unique block at end
        unique_blocks = g_slist_prepend(unique_blocks, block_new(
            chunk->source, previous_common, chunk->start));
    }
    hash_multiset_destroy(their_hashes);
    return g_slist_reverse(unique_blocks);
}

diff_hunk * diff_hunk_new(
        unsigned start, view const * const a, view const * const b) {
    diff_hunk * const hunk = malloc(sizeof(diff_hunk));
    *hunk = (diff_hunk) {.start = start, .a = a, .b = b};
    return hunk;
}

typedef GSList * hunks;

hunks pair_blocks(blocks a, blocks b) {
    block const * a_block, * b_block;
    hash a_anchor, b_anchor;
    #define pop_block(a_or_b) \
        a_or_b##_block = (a_or_b != NULL) ? a_or_b->data : NULL;\
        a_or_b##_anchor = (a_or_b##_block != NULL) ? a_or_b##_block->hash : 0;\
        a_or_b = g_slist_next(a_or_b);
    pop_block(a)
    pop_block(b)
    hunks result = NULL;
    unsigned a_offset = 0, b_offset = 0;
    unsigned a_start = 0, b_start = 0;  //  Is this right?
    while ((a_block != NULL) || (b_block != NULL)) {
        if (a_block != NULL) {
            a_start = a_block->start + a_offset;
        }
        if (b_block != NULL) {
            b_start = b_block->start + b_offset;
        }
        if (a_anchor == b_anchor) {
            //  TODO: Assert the starts are equal
            result = g_slist_prepend(result, diff_hunk_new(
                a_start, &(a_block->v), &(b_block->v)));
            unsigned const hunk_len = (a_block->length > b_block->length) ?
                a_block->length : b_block->length;
            a_offset += hunk_len - a_block->length;
            b_offset += hunk_len - b_block->length;
            pop_block(a)
            pop_block(b)
        } else {
            if ((b_block == NULL) || (a_start > b_start)) {
                //  a_block is next insertion in our virtual stream of diffs
                result = g_slist_prepend(result, diff_hunk_new(
                    a_start, &(a_block->v), NULL));
                b_offset += a_block->length;
                pop_block(a)
            } else {
                //  b_block is next insertion in our virtual stream of diffs
                result = g_slist_prepend(result, diff_hunk_new(
                    a_start, NULL, &(b_block->v)));
                a_offset += b_block->length;
                pop_block(b)
            }
        }
    }
    #undef pop_block
    return result;
}

int main() {
    buffer const a = "foobar";
    chunks a_chunks = NULL;
    chunk a0 = chunk_new(a, 0, 3);
    chunk a1 = chunk_new(a, 3, 6);
    a_chunks = g_slist_prepend(a_chunks, &a1);
    a_chunks = g_slist_prepend(a_chunks, &a0);
    buffer const b = "foobarbar";
    chunks b_chunks = NULL;
    chunk b0 = chunk_new(b, 0, 3);
    chunk b1 = chunk_new(b, 3, 6);
    chunk b2 = chunk_new(b, 6, 9);
    b_chunks = g_slist_prepend(b_chunks, &b2);
    b_chunks = g_slist_prepend(b_chunks, &b1);
    b_chunks = g_slist_prepend(b_chunks, &b0);

    blocks unique_a = unique_blocks(a_chunks, b_chunks);
    blocks unique_b = unique_blocks(b_chunks, a_chunks);
    hunks h = pair_blocks(unique_a, unique_b);
    diff_hunk * dh = h->data;
    printf("hs: %u bs: %u be: %u\n", dh->start, dh->b->start, dh->b->end);
    g_slist_free_full(unique_a, free);
    g_slist_free_full(unique_b, free);
}

/*
diff file_diff(const string a, const string b) {
    // Open files
    // Make views
    // return diff_new(view_a, view_b)
}
*/
