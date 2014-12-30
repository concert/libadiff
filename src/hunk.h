#pragma once
#include "../include/diff_types.h"
#include "chunk.h"

/** \brief Taking two sets of chunks, produce hunks.
 */
hunk * diff_chunks(restrict chunks ours, restrict chunks theirs);

/** \brief Free a linked list of hunks.
 */
void hunk_free(hunk * head);
