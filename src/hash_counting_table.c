#include "hash_counting_table.h"

hash_counting_table hash_counting_table_new() {
    return g_hash_table_new(
        &g_direct_hash,
        &g_direct_equal);
}

void hash_counting_table_inc(hash_counting_table tab, const hash key) {
    gpointer ptr = GUINT_TO_POINTER(key);
    //  A failed lookup comes back with NULL (0):
    gpointer count = g_hash_table_lookup(tab, ptr);
    g_hash_table_insert(tab, ptr, count+1);
}

unsigned hash_counting_table_dec(hash_counting_table tab, const hash key) {
    gpointer ptr = GUINT_TO_POINTER(key);
    unsigned count = GPOINTER_TO_UINT(g_hash_table_lookup(tab, ptr));
    if (count == 1) {
        g_hash_table_remove(tab, ptr);
    } else if (count > 1) {
        g_hash_table_insert(tab, ptr, GUINT_TO_POINTER(count-1));
    }
    return GPOINTER_TO_UINT(count);
}

void hash_counting_table_destroy(hash_counting_table tab) {
    g_hash_table_destroy(tab);
}
