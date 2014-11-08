#pragma once

typedef char* buffer;

typedef struct view {
    buffer source;  // raw data (does this need to be in every view?)
    unsigned end;  // end index in raw data
    buffer data;  // source + start
    // Feels like the following are the interface
    unsigned start;  // start index in raw data
    unsigned length;  // end - start
} view;

typedef struct {
    unsigned start;  // Does this belong in the interface?
    view const * a;
    view const * b;
} diff_hunk;

typedef diff_hunk* diff;  // Too simplistic?
