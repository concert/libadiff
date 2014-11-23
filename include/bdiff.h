#pragma once

typedef struct view {
    unsigned start;  // start index in raw data
    unsigned end;  // end index in raw data
} view;

typedef struct diff_hunk {
    struct diff_hunk * next;
    view const * a;
    view const * b;
} diff_hunk;

typedef unsigned (*data_fetcher)(void * source, unsigned n_items, char * buffer);

diff_hunk * diff(data_fetcher df, void * a, void * b);

void diff_hunk_free(diff_hunk * dh);
