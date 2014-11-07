#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

typedef char* buffer;

typedef unsigned long hash;

typedef struct chunk {
    buffer source;  // raw data
    unsigned start;  // start index in raw data
    unsigned end;  // end index in raw data
    buffer data;  // source + start
    unsigned length;  // end - start
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

typedef GHashTable * const hash_set;

hash_set hash_set_new() {
    return g_hash_table_new_full(
        &g_direct_hash,
        &g_direct_equal,
        /*key destroy*/ NULL,
        /*value destroy*/ NULL);
}

void hash_set_insert(hash_set set, const hash key) {
    hash h = g_hash_table_lookup(set, key);
    //  A failed lookup comes back with NULL (0)
    g_hash_table_insert(set, key, h+1);
}

gboolean hash_set_contains(hash_set set, hash hash) {
    return g_hash_table_lookup_extended(set, hash, NULL, NULL);
}

hash hash_pop(hash_set set, const hash key) {
    hash h = g_hash_table_lookup(set, key);
    if (h == 1) {
        g_hash_table_remove(set, key);
    } else if (h > 1) {
        g_hash_table_insert(set, key, h-1);
    }
    return h;
}

void hash_set_destroy(hash_set set) {
    g_hash_table_destroy(set);
}

typedef chunk block;
// The hash of a block is the hash of its preceeding chunk common to both
// streams used as an anchor to align the streams.


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
    typedef chunk const * const const_chunk;
    hash_set their_hashes = hash_set_new();
    while (theirs != NULL) {
        hash_set_insert(their_hashes, ((const_chunk) theirs->data)->hash);
        theirs = g_slist_next(theirs);
    }

    blocks unique_blocks = NULL;
    chunk zero = chunk_new(NULL, 0, 0);
    chunk const * previous_common = &zero;
    chunk const * chunk;
    while (ours != NULL) {
        chunk = ours->data;
        if (hash_pop(their_hashes, chunk->hash)) {
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
    hash_set_destroy(their_hashes);
    return g_slist_reverse(unique_blocks);
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

    chunks unique = unique_blocks(b_chunks, a_chunks);
    if (unique != NULL) {
        chunk * c = unique->data;
        printf("unique bit: %s\n", c->data);
    } else
        printf("no unique stuff\n");
}

typedef struct {
    unsigned start;
    block a;
    block b;
} diff_hunk;

typedef diff_hunk* diff;

/*
diff file_diff(const string a, const string b) {
    // Open files
    // Make views
    // return diff_new(view_a, view_b)
}
*/
