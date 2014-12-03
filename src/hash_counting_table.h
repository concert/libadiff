#pragma once
#include "hash.h"
#include <glib.h>

typedef GHashTable * const hash_counting_table;

hash_counting_table hash_counting_table_new();

unsigned hash_counting_table_inc(hash_counting_table tab, hash const key);

unsigned hash_counting_table_get(
    hash_counting_table const tab, hash const key);

unsigned hash_counting_table_dec(hash_counting_table tab, hash const key);

void hash_counting_table_destroy(hash_counting_table tab);
