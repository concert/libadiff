#pragma once

#include <inttypes.h>

/** \brief type for hashes.
 * 32 bit size was chosen because it should provide a sufficiently detailed
 * hash for most data whilst being possible to squash into a pointer for all
 * platforms glib supports.
 */
typedef uint32_t hash;
