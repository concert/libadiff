#include <glib.h>
#include <stdio.h>

typedef char* buffer;

typedef unsigned long hash;

typedef struct view {
    buffer source;  // raw data
    unsigned start;  // start index in raw data
    unsigned end;  // end index in raw data
    buffer data;  // source + start
    unsigned length;  // end - start
} view;

view view_new(const buffer source, const unsigned start, const unsigned end) {
    return (view) {
        .source = source,
        .start = start,
        .end = end,
        .data = source + start,
        .length = end - start};
}

typedef struct chunk {
//    const union {
//        const struct view;
        view v;
//    };
    hash hash;
} chunk;

chunk chunk_new(const buffer source, const unsigned start, const unsigned end) {
    char substr[end - start + 1];
    snprintf(substr, end - start + 1, "%s", source + start);
    //printf("substr: %u %u %s\n", start, end, substr);
    return (chunk) {
        .v = view_new(source, start, end),
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

void hash_set_insert(hash_set set, hash hash) {
    g_hash_table_insert(set, hash, hash);
}

gboolean hash_set_contains(hash_set set, hash hash) {
    return g_hash_table_lookup_extended(set, hash, NULL, NULL);
}

void hash_set_destroy(hash_set set) {
    g_hash_table_destroy(set);
}

chunks unique_blocks(restrict chunks our_chunks, restrict chunks others_chunks) {
    typedef chunk const * const const_chunk;
    hash some_hash = ((const_chunk) others_chunks->data)->hash;
    hash_set others_hashes = hash_set_new();
    while (others_chunks != NULL) {
        hash_set_insert(others_hashes, ((const_chunk) others_chunks->data)->hash);
        others_chunks = g_slist_next(others_chunks);
    }

    chunks unique_chunks = NULL;
    while (our_chunks != NULL) {
        if (!hash_set_contains(others_hashes, ((const_chunk) our_chunks->data)->hash)) {
            unique_chunks = g_slist_prepend(unique_chunks, our_chunks->data);
        }
        our_chunks = g_slist_next(our_chunks);
    }
    hash_set_destroy(others_hashes);
    return g_slist_reverse(unique_chunks);
}

int main() {
    buffer const a = "foobar";
    chunks a_chunks = NULL;
    chunk a0 = chunk_new(a, 0, 3);
    chunk a1 = chunk_new(a, 3, 6);
    a_chunks = g_slist_prepend(a_chunks, &a1);
    a_chunks = g_slist_prepend(a_chunks, &a0);
    buffer const b = "foobarbaz";
    chunks b_chunks = NULL;
    chunk b0 = chunk_new(b, 0, 3);
    chunk b1 = chunk_new(b, 3, 6);
    chunk b2 = chunk_new(b, 6, 9);
    b_chunks = g_slist_prepend(b_chunks, &b2);
    b_chunks = g_slist_prepend(b_chunks, &b1);
    b_chunks = g_slist_prepend(b_chunks, &b0);

    chunks unique = unique_blocks(b_chunks, a_chunks);
    printf("unique bit: %s\n", ((chunk *) unique->data)->v.data);
}

typedef struct view block;

typedef struct {
    unsigned start;
    block a;
    block b;
} diff_hunk;

typedef diff_hunk* diff;

diff diff_new(const view a, const view b) {
}

/*
diff file_diff(const string a, const string b) {
    // Open files
    // Make views
    // return diff_new(view_a, view_b)
}
*/
