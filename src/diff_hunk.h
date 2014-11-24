#pragma once
#include "../include/diff_types.h"
#include "block.h"

diff_hunk * pair_blocks(blocks a, blocks b);

void diff_hunk_free(diff_hunk * head);
