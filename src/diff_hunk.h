#pragma once
#include "../include/diff.h"

diff_hunk * diff_hunk_new(
        diff_hunk * const prev, view const * const a, view const * const b);

void diff_hunk_free(diff_hunk * head);
