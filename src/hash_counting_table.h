#pragma once
#include "hash.h"
#include <glib.h>

/** \brief Used to "look ahead" to see if a hash is present.
 * We use this to reduce a list search O(n) to a hash lookup O(1).
 * This structure would more commonly be known as a multiset.
 */
typedef GHashTable * const hash_counting_table;

/** \brief Create and initialise a hash_counting_table.
 */
hash_counting_table hash_counting_table_new();

/** \brief Increment the counter for a given hash.
 * \param[inout] tab the table to modify.
 * \param[in] key the hash to increment.
 */
void hash_counting_table_inc(hash_counting_table tab, hash const key);

/** \brief Get the counter for a given hash.
 * \param[in] tab the table to look in.
 * \param[in] key the hash to look for.
 * \return the counter (or 0 if not found).
 */
unsigned hash_counting_table_get(
    hash_counting_table const tab, hash const key);

/** \brief Decrement the counter for a given hash.
 * \param[inout] tab the table to modify.
 * \param[in] key the hash to increment.
 */
void hash_counting_table_dec(hash_counting_table tab, hash const key);

/** \brief Free a hash_counting_table.
 * \param[inout] tab the table to free.
 */
void hash_counting_table_destroy(hash_counting_table tab);
