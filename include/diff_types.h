#pragma once

typedef struct view {
    unsigned start;  // start index in raw data
    unsigned end;  // end index in raw data
} view;

typedef struct diff_hunk {
    struct diff_hunk * next;
    view a;
    view b;
} diff_hunk;

void diff_hunk_free(diff_hunk * dh);
