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

extern window_data const clear_window;

hash windowed_hash(window_data * const w, unsigned char const next);
