#pragma once

#include "src/hash.h"

/** \brief Opaque structure holding state for hashing.
 */
typedef struct {
    hash h;
    hash irreducible_polynomial;
    hash table[256];
} hash_data;

/** \brief Initialise the hashing state with the given irreducible polynomial.
 */
hash_data hash_data_init(hash const irreducible_polynomial);

/** \brief Return the hash to a state where no data has been hashed.
 */
void hash_data_reset(hash_data * const hd);

/** \brief Hash a byte of data.
 * \return The new hash.
 */
hash hash_data_update(hash_data * const hd, unsigned char const next);


/** \brief Opaque structure holding state for windowed hashing.
 */
typedef struct {
    union {
        hash_data;
        hash_data hd;
    };
    unsigned window_size;
    unsigned char * undo_buf;
    unsigned buf_pos;
    hash undo_table[256];
} window_data;

/** \brief Initialise the windowed hashing state.
 * \param[in] hd Hash data structure to use as the basis for the
 * windowed hash (i.e. to use for its irreducible polynomial).
 * \param[in] window_buffer Buffer (at least window_size bytes in size) to use
 * for storing the data currently in the window.
 * \param[in] window_size Number of bytes in the window.
 */
window_data window_data_init(
    hash_data const * const hd, unsigned char * const window_buffer,
    unsigned const window_size);

/** \brief Return the windowed hash to a state where no data has been hashed.
 */
void window_data_reset(window_data * const wd);

/** \brief Hash a byte of data.
 * \return The hash of the new window.
 */
hash window_data_update(window_data * const wd, unsigned char const next);
