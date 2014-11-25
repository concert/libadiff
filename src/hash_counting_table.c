#include "hash_counting_table.h"

hash_multiset hash_multiset_new() {
    return g_hash_table_new(
        &g_direct_hash,
        &g_direct_equal);
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
