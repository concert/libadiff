#pragma once

typedef char* buffer;

typedef struct view {
    unsigned start;  // start index in raw data
    unsigned end;  // end index in raw data
} view;

typedef struct diff_hunk {
    struct diff_hunk * next;
    view const * a;
    view const * b;
} diff_hunk;
