#include "../include/rabin.h"

const unsigned hash_len = 9;
// irreducible_polynomial = pow(2, 9) + pow(2, 1) + pow(2, 0);
const unsigned irreducible_polynomial = 515;

const unsigned window_size = 12;

// f(pow(t,l))
// Performs polynomial division (currently fixed to by the irreducible
// polynomial) returning the remainder
static unsigned f_pow_t_l(unsigned l) {
    if (l < hash_len) {  // Quotient = 0
        return 1 << l;
    }
    unsigned quotient_power = l + 1 - hash_len;
    hash partial_solution = 0;
    //hash const mask = 1 << hash_len - 1;  might be faster, needs testing
    hash quotient_coefficient = 1;
    do {
        partial_solution <<= 1;
        partial_solution &= 0x1FF;  // 9 bit hash hack
        if (quotient_coefficient) {
            partial_solution ^= irreducible_polynomial -  512;
        }
        // Set conditions for the next iteration
        quotient_coefficient = partial_solution >> (hash_len - 1);
        //quotient_coefficient = mask & partial_solution;
    } while (--quotient_power);
    return partial_solution;
}

static unsigned f(unsigned i) {
    if (i >= (1 << hash_len)) {
        return i ^ irreducible_polynomial;
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

void window_data_reset(window_data * const wd) {
    hash_data_reset(&wd->hd);
    wd->undo_buf = wd->hd.h;
}

window_data window_data_init() {
    window_data wd = {
        .hd = hash_data_init()};
    window_data_reset(&wd);
    return wd;
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
