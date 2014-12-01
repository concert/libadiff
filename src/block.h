#pragma once
#include "chunk.h"

/*! A block data is very similar to a chunk of data in our abstraction. That
 * is, it is a simple view onto the data with an additional hash property.
 *
 * Conceptually, our blocks are formed from multiple chunks of data that are
 * unique to only one of the data streams we are comparing - they are "bigger"
 * units.
 *
 * Unlike chunks, the hash of a block is not the hash of the contents,
 * but rather the hash of the preceeding "anchor" chunk that is common to both
 * of the streams being compared. This hash is used as a reference point so we
 * can align the streams and compare which set of unique data in one stream can
 * be rightly compared to which in the other.
 */
typedef chunk block;

typedef block * blocks;

blocks unique_blocks(restrict chunks ours, restrict chunks theirs);

extern void block_free(block * head);
