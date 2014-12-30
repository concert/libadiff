#include <stdio.h>
#include <stdlib.h>
#include "../include/adiff.h"

static hunk * load_diff_file(char const * path) {
    FILE * diff_file = fopen(path, "r");
    if (diff_file == NULL) {
        fprintf(stderr, "Unable to open diff file: %s\n", path);
        return NULL;
    }
    unsigned a_start, a_end, b_start, b_end;
    hunk * head = NULL, * tail = NULL;
    while (
            fscanf(
                diff_file, "%u %u %u %u\n",
                &a_start, &a_end, &b_start, &b_end)
            == 4) {
        append_hunk(&head, &tail, a_start, a_end, b_start, b_end);
    }
    return head;
}

int main(int argc, char ** argv) {
    if (argc != 5) {
        printf("Usage: %s diff_file a_file b_file output_file\n", argv[0]);
        return -2;
    }
    hunk * const h = load_diff_file(argv[1]);
    if (h == NULL) {
        fprintf(stderr, "Failed to load diff file\n");
        return -1;
    }
    apatch_return_code const code = apatch(h, argv[2], argv[3], argv[4]);
    switch (code) {
        case APATCH_OK:
            printf("Patch applied\n");
            break;
        case APATCH_ERR_OPEN_A:
            fprintf(stderr, "Failed to open (read) %s\n", argv[2]);
            break;
        case APATCH_ERR_OPEN_B:
            fprintf(stderr, "Failed to open (read) %s\n", argv[3]);
            break;
        case APATCH_ERR_OPEN_OUTPUT:
            fprintf(stderr, "Failed to open (write) %s\n", argv[4]);
            break;
    }
    hunk_free(h);
    return code;
}
