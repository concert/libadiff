#pragma once

#include "diff_types.h"

/** \brief Codes for errors that may be encountered whilst diffing.
 */
typedef enum {
    ADIFF_OK = 0,
    ADIFF_ERR_OPEN_A,
    ADIFF_ERR_OPEN_B,
    ADIFF_ERR_CHANNELS,
    ADIFF_ERR_SAMPLE_RATE,
    ADIFF_ERR_SAMPLE_FORMAT,
} adiff_return_code;

/** \brief Information about how two files differ (or why they couldn't be compared).
 */
typedef struct {
    adiff_return_code code;
    hunk * hunks;
} diff;

/** \brief Compare the files at the specified paths.
 * \param[in] a_path The original file for comparison.
 * \param[in] b_path The modified version of the file.
 * \return A diff of the two files.
 */
diff adiff(char const * const a_path, char const * const b_path);

/** \brief Free a diff (as returned by adiff).
 */
void diff_free(diff * d);
