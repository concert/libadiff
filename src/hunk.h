#pragma once
#include "../include/diff_types.h"
#include "chunk.h"

hunk * diff_chunks(restrict chunks ours, restrict chunks theirs);

void hunk_free(hunk * head);
