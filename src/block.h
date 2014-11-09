#pragma once
#include "chunk.h"

// The hash of a block is the hash of its preceeding chunk common to both
// streams used as an anchor to align the streams.
typedef chunk block;

block * block_new(
        block * prev, chunk const * const previous_common,
        const unsigned end);

typedef block * blocks;
