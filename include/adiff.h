#pragma once

#include "diff_types.h"

typedef enum {
    ADIFF_OK = 0,
    ADIFF_ERR_OPEN_A,
    ADIFF_ERR_OPEN_B,
    ADIFF_ERR_CHANNELS,
    ADIFF_ERR_SAMPLE_RATE,
    ADIFF_ERR_SAMPLE_FORMAT,
} adiff_return_code;

typedef enum {
    APATCH_OK = 0,
    APATCH_ERR_OPEN_A,
    APATCH_ERR_OPEN_B,
    APATCH_ERR_OPEN_OUTPUT,
} apatch_return_code;

typedef struct {
    adiff_return_code code;
    hunk * hunks;
} diff;

diff adiff(char const * const a_path, char const * const b_path);

apatch_return_code apatch(
    hunk * hunks, char const * const a_path, char const * const b_path,
    char const * const out_path);

void diff_free(diff * d);
