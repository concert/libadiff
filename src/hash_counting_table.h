#pragma once
#include "hash.h"
#include <glib.h>

typedef GHashTable * const hash_counting_table;

hash_counting_table hash_counting_table_new();

void hash_counting_table_inc(hash_counting_table tab, const hash key);

unsigned hash_counting_table_dec(hash_counting_table tab, const hash key);

void hash_counting_table_destroy(hash_counting_table tab);
