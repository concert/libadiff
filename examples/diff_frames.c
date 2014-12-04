#include <stdio.h>
#include "../include/adiff.h"

int main(int argc, char ** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: diff_frames file_a file_b\n");
        return -1;
    }
    diff d = adiff(argv[1], argv[2]);
    switch (d.code) {
        case ADIFF_ERR_OPEN_A:
            fprintf(stderr, "Failed to open: %s\n", argv[1]);
            break;
        case ADIFF_ERR_OPEN_B:
            fprintf(stderr, "Failed to open: %s\n", argv[2]);
            break;
        case ADIFF_ERR_CHANNELS:
            fprintf(stderr, "Files have different numbers of channels\n");
            break;
        case ADIFF_ERR_SAMPLE_RATE:
            fprintf(stderr, "Files have different sample rates\n");
            break;
        case ADIFF_ERR_SAMPLE_FORMAT:
            fprintf(stderr, "Files have different sample formats\n");
            break;
        case ADIFF_OK:
            if (d.hunks == NULL) {
                fprintf(stderr, "No changes found");
            } else {
                fprintf(stderr, "OK");
            }
    }
    diff_free(&d);
    return d.code;
}
