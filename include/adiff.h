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

/** \brief Codes for errors that may be encountered wihlst patching.
 */
typedef enum {
    APATCH_OK = 0,
    APATCH_ERR_OPEN_A,
    APATCH_ERR_OPEN_B,
    APATCH_ERR_OPEN_OUTPUT,
} apatch_return_code;

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

/** \brief Generate a patched file using the given patch and source data.
 * \param[in] hunks The diff data to use when generating the new file.
 * \param[in] a_path A path to the original source file A.
 * \param[in] b_path A path to the original source file B.
 * \param[in] out_path Where the new file should be written.
 * \return A code describing the success or otherwise of the patch.
 */
apatch_return_code apatch(
    hunk * hunks, char const * const a_path, char const * const b_path,
    char const * const out_path);

/** \brief Free a diff (as returned by adiff).
 */
void diff_free(diff * d);
