#pragma once
#include "hash.h"
#include <glib.h>

typedef GHashTable * const hash_multiset;

hash_multiset hash_multiset_new();

void hash_multiset_insert(hash_multiset set, const hash key);

hash hash_multiset_pop(hash_multiset set, const hash key);

void hash_multiset_destroy(hash_multiset set);
