#include "hash_counting_table.h"

hash_counting_table hash_counting_table_new() {
    return g_hash_table_new(
        &g_direct_hash,
        &g_direct_equal);
}

void hash_counting_table_inc(hash_counting_table tab, const hash key) {
    gpointer ptr = GUINT_TO_POINTER(key);
    gpointer h = g_hash_table_lookup(tab, ptr);
    //  A failed lookup comes back with NULL (0)
    g_hash_table_insert(tab, ptr, h+1);
}

hash hash_counting_table_dec(hash_counting_table tab, const hash key) {
    gpointer ptr = GUINT_TO_POINTER(key);
    hash h = GPOINTER_TO_UINT(g_hash_table_lookup(tab, ptr));
    if (h == 1) {
        g_hash_table_remove(tab, ptr);
    } else if (h > 1) {
        g_hash_table_insert(tab, ptr, GUINT_TO_POINTER(h-1));
    }
    return GPOINTER_TO_UINT(h);
}

void hash_counting_table_destroy(hash_counting_table tab) {
    g_hash_table_destroy(tab);
}
