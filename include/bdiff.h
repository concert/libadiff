#pragma once

#include "diff_types.h"

/** \brief A function to be supplied by the library user for getting the data to diff.
 */
typedef unsigned (*data_fetcher)(
    void * source, char * buffer, unsigned n_items);

/** \brief Perform a binary diff.
 *
 * \param[in] sample_size The size (in bytes) of an item in the data type being
 * diffed.
 * \param[in] df Function to use to get the data from a and b.
 * \param[in] a Given as the source parameter to df for the a side.
 * \param[in] b Given as the source parameter to df for the b side.
 * \return A linked list of hunks representing the changes made between the two data sources.
 */
hunk * const bdiff(
    unsigned const sample_size, data_fetcher const df, void * const a,
    void * const b);
