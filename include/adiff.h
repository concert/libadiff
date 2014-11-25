#pragma once

#include "diff_types.h"

enum adiff_return_code {
    ADIFF_OK = 0,
    ADIFF_ERR_CHANNELS,
    ADIFF_ERR_SAMPLE_RATE,
    ADIFF_ERR_SAMPLE_FORMAT,
    ADIFF_ERR_OPEN_A,
    ADIFF_ERR_OPEN_B
};

typedef struct {
    adiff_return_code code;
    hunk * hunks;
} diff;

diff adiff(char const * const a_path, char const * const b_path);

void diff_free(diff d);
