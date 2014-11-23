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

// f(t^l)
static unsigned f_pow_t_l(unsigned l) {
    unsigned v_shift = l + 1 - hash_len;
    hash div_window = 0;
    unsigned mask = pow(2, hash_len - 1);
    unsigned reduce = 1;
    while (v_shift--) {
        div_window <<= 1;
        if (reduce) {
            div_window ^= (irreducible_polynomial -  512);
        }
        reduce = mask & div_window;
    }
    return div_window & 0x1FF;
}

static unsigned f(unsigned i) {
    if (i >= pow(2, 9)) {
        return sub(i, irreducible_polynomial);
    }
    return i;
}

hash_data hash_data_init() {
    hash_data hd = {};
    hash_data_reset(&hd);
    return hd;
}

void hash_data_reset(hash_data * const hd) {
    hd->h = 1;
}

static void bin_print(hash h) {
    printf("(");
    for (unsigned p = 9; p > 0; p--) {
        if ((0x1 << (p - 1)) & h) {
            printf("1, ");
        } else {
            printf("0, ");
        }
    }
    printf(")\n");
}

hash hash_data_update(hash_data * const hd, unsigned char const next) {
    for (unsigned p = 8; p > 0; p--) {
        hd->h <<= 1;
        unsigned const mask = 0x1 << (p - 1);
        if (mask & next) {
            hd->h |= 1;
        }
        if (hd->h & 0x200) {
            hd->h ^= irreducible_polynomial;
        }
    }
    return hd->h;
}

hash windowed_hash(window_data * const w, unsigned char const next) {
    unsigned undo = 0;
    for (unsigned p = 8; p > 0; p--) {
        unsigned mask = 1 << (p - 1 + window_size);
        if (w->undo_buf & mask) {
            unsigned us = f_pow_t_l(p - 1 + window_size);
            undo ^= us;
        }
    }
    w->undo_buf = (w->undo_buf << 8) | next;
    w->h = w->h ^ undo;
    hash_data_update(&w->hd, next);
    return w->h;
}
