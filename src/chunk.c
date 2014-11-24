#include "chunk.h"
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

void chunk_free(chunk * head) {
    while (head != NULL) {
        chunk * prev = head;
        head = head->next;
        free(prev);
    }
}
