#pragma once
#include "../include/bdiff.h"
#include "hash.h"

typedef struct chunk {
    union {
        struct view;
        view v;
    };
    hash hash;
    struct chunk * next;
} chunk;

typedef chunk * chunks;

chunk * chunk_new(
        chunk * const prev, unsigned const start, unsigned const end,
        hash const h);

chunks const split_data(
    unsigned const sample_size, data_fetcher const df, void * const source);

void chunk_free(chunk * head);
