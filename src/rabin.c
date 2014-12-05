#include "../include/rabin.h"

const unsigned hash_len = sizeof(hash) * 8;

// f(pow(t,l))
// Performs polynomial division (currently fixed to by the irreducible
// polynomial) returning the remainder
static hash f_pow_t_l(hash irreducible_polynomial, unsigned l) {
    if (l < hash_len) {  // Quotient = 0
        return 1 << l;
    }
    unsigned quotient_power = l + 1 - hash_len;
    hash partial_solution = 0;
    //hash const mask = 1 << hash_len - 1;  might be faster, needs testing
    hash quotient_coefficient = 1;
    do {
        partial_solution <<= 1;
        if (quotient_coefficient) {
            partial_solution ^= irreducible_polynomial;
        }
        // Set conditions for the next iteration
        quotient_coefficient = partial_solution >> (hash_len - 1);
        //quotient_coefficient = mask & partial_solution;
    } while (--quotient_power);
    return partial_solution;
}

static void populate_table(
        hash * const table, hash const irreducible_polynomial,
        unsigned const pos) {
    table[0] = 0x0;  // No bits is always 0
    for (unsigned i = 0; i < 8; i++) {
        hash const msb = f_pow_t_l(irreducible_polynomial, i + pos);
        unsigned const msb_table_index = 0x1 << i;
        // Add the new polynomial to those already in the table
        for (unsigned j = 0; j < msb_table_index; j++) {
            table[msb_table_index + j] = msb ^ table[j];
        }
    }
}

hash_data hash_data_init(hash const irreducible_polynomial) {
    hash_data hd = {.irreducible_polynomial = irreducible_polynomial};
    populate_table(hd.table, irreducible_polynomial, hash_len);
    hash_data_reset(&hd);
    return hd;
}

void hash_data_reset(hash_data * const hd) {
    hd->h = 1;
}

hash hash_data_update(hash_data * const hd, unsigned char const next) {
    hash const overflow = hd->table[hd->h >> (hash_len - 8)];
    hd->h <<= 8;
    hd->h |= next;
    hd->h ^= overflow;
    return hd->h;
}

void window_data_reset(window_data * const wd) {
    hash_data_reset(&wd->hd);
    for (unsigned i = 0; i < wd->window_size - 1; i++) {
        wd->undo_buf[i] = 0;
    }
    wd->undo_buf[wd->window_size - 1] = wd->hd.h;
    wd->buf_pos = 0;
}

window_data window_data_init(
        hash_data const * const h, unsigned char * const window_buffer,
        unsigned const window_size) {
    window_data wd = {
        .hd = *h, .window_size = window_size, .undo_buf = window_buffer};
    window_data_reset(&wd);
    return wd;
}

hash window_data_update(window_data * const w, unsigned char const next) {
    hash undo = 0;
    for (unsigned p = 8; p > 0; p--) {
        unsigned char mask = 0x1 << (p - 1);
        if (w->undo_buf[w->buf_pos] & mask) {
            undo ^= f_pow_t_l(
                w->irreducible_polynomial,
                p - 1 + ((w->window_size - 1) * 8));
        }
    }
    w->undo_buf[w->buf_pos] = next;
    if (++w->buf_pos == w->window_size) {
        w->buf_pos = 0;
    }
    w->h = w->h ^ undo;
    hash_data_update(&w->hd, next);
    return w->h;
}
