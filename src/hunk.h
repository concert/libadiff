#pragma once
#include "../include/diff_types.h"
#include "chunk.h"

hunk * hunk_factory(restrict chunks ours, restrict chunks theirs);

void hunk_free(hunk * head);
