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

void hunk_free(hunk * head);
