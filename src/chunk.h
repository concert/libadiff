#pragma once
#include "include/diff_types.h"
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

void chunk_free(chunk * head);
