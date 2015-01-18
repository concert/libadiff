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
static hash const irreducible_polynomial = 141;

static inline hash hash_sample(
        hash_data * const hd, window_data * const wd,
        unsigned const sample_size, char const * const buf) {
    for (unsigned short b = 0; b < sample_size; b++) {
        hash_data_update(hd, buf[b]);
        window_data_update(wd, buf[b]);
    }
    return wd->h;
}

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
        void * const source, unsigned const min_length) {
    char buf[buf_size];
    chunks head = NULL, tail = NULL;
    unsigned samples_read, start_pos = 0, total_samples_read = 0;
    hash_data hd = hash_data_init(irreducible_polynomial);
    unsigned const window_buffer_size = sample_size * 16;
    unsigned char window_buffer[window_buffer_size];
    window_data wd = window_data_init(&hd, window_buffer, window_buffer_size);
    do {
        samples_read = df(source, buf_size / sample_size, buf);
        for (unsigned sample = 0; sample < samples_read; sample++) {
            hash const h = hash_sample(
                &hd, &wd, sample_size, buf + (sample * sample_size));
            if ((total_samples_read - start_pos) >= min_length && !(h & 0xFF)) {
                tail = chunk_new(
                    tail, start_pos, total_samples_read, hd.h);
                start_pos = total_samples_read;
                if (head == NULL) {
                    head = tail;
                }
                hash_data_reset(&hd);
                window_data_reset(&wd);
            }
            total_samples_read++;
        }
    } while (samples_read);
    if (total_samples_read > start_pos) {
        tail = chunk_new(tail, start_pos, total_samples_read + 1, hd.h);
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
