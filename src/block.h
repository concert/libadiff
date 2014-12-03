#pragma once
#include "chunk.h"

/*! A block of data is very similar to a chunk of data in our abstraction. That
 * is, it is a simple view onto the data and a small amount of metadata.
 *
 * Conceptually our blocks are formed from multiple chunks of data that are
 * unique to only one of the data streams we are comparing - they are "bigger"
 * units. Instead of a hash, the metadata we store is used as a reference to
 * tie the position of this unique data to a position in the comparison stream
 * of data.
 */
typedef struct block {
    union {
        struct view;
        view v;
    };
    unsigned counterpart_start;
    struct block * next;
} block;

typedef block * blocks;

hunk * hunk_factory(restrict chunks ours, restrict chunks theirs);

extern void block_free(block * head);
