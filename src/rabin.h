#pragma once

//unsigned const starting_hash = 1;

unsigned hash(unsigned h, char next);

typedef struct {
    unsigned hash;
    unsigned undo_buf;
} window;

window windowed_hash(window w, char next);
