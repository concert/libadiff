#pragma once

#include "diff_types.h"

/** \brief A function to be supplied by the library user for getting the data to diff.
 */
typedef unsigned (*data_fetcher)(
    void * source, char * buffer, unsigned n_items);

hunk * const bdiff_rough(
    unsigned const sample_size, data_fetcher const df, void * const a,
    void * const b);

/** \brief A function to be supplied by the library user for seeking to a point
 * in the data to diff.
 */
typedef void (*data_seeker)(void * source, unsigned pos);

hunk * const bdiff_narrow(
    hunk * rough_hunks, unsigned const sample_size, data_seeker const ds,
    data_fetcher const df, void * const a, void * const b);

/** \brief Perform a binary diff.
 *
 * \param[in] sample_size The size (in bytes) of an item in the data type being
 * diffed.
 * \param[in] ds Function to use to seek to a point in the data from a and b.
 * \param[in] df Function to use to get the data from a and b.
 * \param[in] a Given as the source parameter to df for the a side.
 * \param[in] b Given as the source parameter to df for the b side.
 * \return A linked list of hunks representing the changes made between the two data sources.
 */
hunk * const bdiff(
    unsigned const sample_size, data_seeker const ds, data_fetcher const df,
    void * const a, void * const b);
