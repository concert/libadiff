#include "rabin.h"
#include <stdio.h>
#include <math.h>

const unsigned hash_len = 9;
// irreducible_polynomial = pow(2, 9) + pow(2, 1) + pow(2, 0);
const unsigned irreducible_polynomial = 515;

const unsigned window_size = 12;
// unsigned one_over = pow(2, 3) + pow(2, 2);
const unsigned one_over = 12;

static inline unsigned sub(unsigned a, unsigned b) {
    return a ^ b;
}

static unsigned f(unsigned i) {
    if (i >= pow(2, 9)) {
        return sub(i, irreducible_polynomial);
    }
    return i;
}

unsigned hash(unsigned h, char next) {
    for (unsigned p = 8; p > 0; p--) {
        unsigned mask = 0x1 << (p - 1);
        mask = mask & next;
        if (mask != 0) {
            h = (h << 1) | 1;
        } else {
            h = h << 1;
        }
        h = f(h);
    }
}

window windowed_hash(window w, char next) {
    unsigned undo = w.undo_buf & 0xFF000;
    for (unsigned p = 8; p > 0; p--) {
        unsigned mask = 1 << (p - 1 + window_size);
        if (undo & mask) {
            undo ^= one_over;
        }
        undo = undo << 1;
    }
    undo &= 0x1FF;
    w.undo_buf = (w.undo_buf << 8) | next;
    w.undo_buf &= 0xFFFFF;
    w.hash = w.hash ^ undo;
    w.hash = hash(w.hash, next);
    return w;
}
