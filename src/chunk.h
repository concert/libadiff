#pragma once
#include "include/diff.h"
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

chunk * chunk_new_malloc(
        chunk * const prev, unsigned const start, unsigned const end,
        hash const h);

void chunk_free(chunk * head);

chunk * chunk_new(
        char const * const source, chunk * const prev, const unsigned start,
        const unsigned end);
