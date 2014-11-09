#include "chunk.h"
#include <stdlib.h>

chunk * chunk_new_malloc(
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

chunk * chunk_new(
        char const * const source, chunk * const prev, const unsigned start,
        const unsigned end) {
    char substr[end - start + 1];
    snprintf(substr, end - start + 1, "%s", source + start);
    return chunk_new_malloc(prev, start, end, g_str_hash(substr));
}
