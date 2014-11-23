#pragma once

typedef unsigned hash;


typedef struct hash_data {
    hash h;
} hash_data;

hash_data hash_data_init();

void hash_data_reset(hash_data * const hd);

hash hash_data_update(hash_data * const hd, unsigned char const next);


typedef struct {
    union {
        struct hash_data;
        hash_data hd;
    };
    unsigned undo_buf;
} window_data;

window_data window_data_init();

void window_data_reset(window_data * const wd);

hash window_data_update(window_data * const wd, unsigned char const next);
