#pragma once
#include "../include/diff_types.h"
#include "block.h"

hunk * pair_blocks(blocks a, blocks b);

void hunk_free(hunk * head);
