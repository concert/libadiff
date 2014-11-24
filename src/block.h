#pragma once
#include "chunk.h"

// The hash of a block is the hash of its preceeding chunk common to both
// streams used as an anchor to align the streams.
typedef chunk block;

typedef block * blocks;

blocks unique_blocks(restrict chunks ours, restrict chunks theirs);
