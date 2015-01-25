#pragma once
#include "../include/bdiff.h"
#include "hash.h"

/** \brief Shift resistant block.
 *
 * Linked list of views with hashes.
 */
typedef struct chunk {
    union {
        view;
        view v;
    };
    hash hash;
    struct chunk * next;
} chunk;

typedef chunk * chunks;

/** \brief Create a new chunk on the heap.
 *
 * \param[out] prev pointer to the chunk to append the new one to.
 * \param[in] start the starting index of the chunk.
 * \param[in] end the end index of the chunk (exclusive).
 * \param[in] h the hash of the chunk.
 * \return a pointer to the new chunk.
 */
chunk * chunk_new(
        chunk * const prev, unsigned const start, unsigned const end,
        hash const h);

/** \brief Split the data given by the data_fetcher into shift resistant blocks.
 *
 * \param[in] sample_size the size of a sample returned by the data fetcher.
 * \param[in] df a data_fetcher function.
 * \param[in] source pointer to the data to give to the specified data_fetcher.
 * \return the head of a linked list of chunks.
 */
chunks const split_data(
    unsigned const sample_size, data_fetcher const df, void * const source,
    unsigned const min_length, unsigned const max_length);

/** \brief Free a linked list of chunks.
 *
 * \param[out] head pointer to the start of the list.
 */
void chunk_free(chunk * head);
