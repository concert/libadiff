#include "chunk.h"
#include "../include/rabin.h"
#include <stdlib.h>
#include <stdio.h>

chunk * chunk_new(
        chunk * const prev, unsigned const start, unsigned const end,
        hash const h) {
    chunk *new_chunk = malloc(sizeof(chunk));
    *new_chunk = (chunk) {.start = start, .end = end, .hash = h};
    if (prev != NULL) {
        prev->next = new_chunk;
    }
    return new_chunk;
}

static const unsigned buf_size = 16384;

// 2**32 (truncated) + 2**7 + 2**3 + 2**2 + 2**0
hash const irreducible_polynomial = 141;

/*! Breaks data into chunks by splitting based on content.
 *
 * We read data into a buffer using the provided data_fetcher, then we use a
 * rolling hash over that data to split based on the content (we split when the
 * lower n bits of the hash is zero). That means that our splitting is
 * resistant to shifts in the data from insertions and deletions, which is very
 * useful for comparison with other data.
 */
chunks const split_data(
        unsigned const sample_size, data_fetcher const df,
        void * const source) {
    char buf[buf_size];
    chunks head = NULL, tail = NULL;
    unsigned samples_read, start_pos = 0, total_samples_read = 0;
    hash chunk_hash;
    hash_data hd = hash_data_init(irreducible_polynomial);
    unsigned const window_buffer_size = sample_size * 16;
    unsigned char window_buffer[window_buffer_size];
    window_data wd = window_data_init(&hd, window_buffer, window_buffer_size);
    do {
        samples_read = df(source, buf_size / sample_size, buf);
        unsigned byte_index = 0;
        for (
                unsigned max_byte_index = sample_size - 1;
                max_byte_index < (sample_size * samples_read);
                max_byte_index += sample_size) {
            for (; byte_index < max_byte_index; byte_index++) {
                hash_data_update(&hd, buf[byte_index]);
                window_data_update(&wd, buf[byte_index]);
            }
            chunk_hash = hash_data_update(&hd, buf[byte_index]);
            if (!(window_data_update(&wd, buf[byte_index]) & 0xFF)) {
                tail = chunk_new(
                    tail, start_pos, total_samples_read, chunk_hash);
                start_pos = total_samples_read;
                if (head == NULL) {
                    head = tail;
                }
                hash_data_reset(&hd);
                window_data_reset(&wd);
            }
            byte_index++;
            total_samples_read++;
        }
    } while (samples_read);
    if (total_samples_read > start_pos) {
        tail = chunk_new(tail, start_pos, total_samples_read + 1, chunk_hash);
        if (head == NULL) {
            head = tail;
        }
    }
    return head;
}

void chunk_free(chunk * head) {
    while (head != NULL) {
        chunk * prev = head;
        head = head->next;
        free(prev);
    }
}
