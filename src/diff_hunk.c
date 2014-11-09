#include "diff_hunk.h"

diff_hunk * diff_hunk_new(
        diff_hunk * const prev, view const * const a, view const * const b) {
    diff_hunk * const new_hunk = malloc(sizeof(diff_hunk));
    *new_hunk = (diff_hunk) {.a = a, .b = b};
    if (prev != NULL) {
        prev->next = new_hunk;
    }
    return new_hunk;
}

void diff_hunk_free(diff_hunk * head) {
    while (head != NULL) {
        diff_hunk * prev = head;
        head = head->next;
        free(prev);
    }
}
