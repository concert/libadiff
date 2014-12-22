#pragma once

typedef struct view {
    unsigned start;  // start index in raw data
    unsigned end;  // end index in raw data
} view;

typedef struct hunk {
    struct hunk * next;
    view a;
    view b;
} hunk;

void append_hunk(
        hunk ** const head, hunk ** const tail, unsigned const a_start,
        unsigned const a_end, unsigned const b_start, unsigned const b_end);

void hunk_free(hunk * head);
