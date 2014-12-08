#pragma once

#include <inttypes.h>

typedef uint32_t hash;


typedef struct hash_data {
    hash h;
    hash irreducible_polynomial;
    hash table[256];
} hash_data;

hash_data hash_data_init(hash const irreducible_polynomial);

void hash_data_reset(hash_data * const hd);

hash hash_data_update(hash_data * const hd, unsigned char const next);


typedef struct {
    union {
        struct hash_data;
        hash_data hd;
    };
    unsigned window_size;
    unsigned char * undo_buf;
    unsigned buf_pos;
    hash undo_table[256];
} window_data;

window_data window_data_init(
    hash_data const * const hd, unsigned char * const window_buffer,
    unsigned const window_size);

void window_data_reset(window_data * const wd);

hash window_data_update(window_data * const wd, unsigned char const next);
