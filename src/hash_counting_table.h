#pragma once
#include "hash.h"
#include <glib.h>

typedef GHashTable * const hash_counting_table;

hash_counting_table hash_counting_table_new();

void hash_counting_table_insert(hash_counting_table set, const hash key);

hash hash_counting_table_pop(hash_counting_table set, const hash key);

void hash_counting_table_destroy(hash_counting_table set);
