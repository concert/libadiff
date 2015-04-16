#pragma once
#include "hunk.h"

typedef struct {
    unsigned pos;
    unsigned const n_values;
    unsigned const * const from;
    unsigned const * const value;
} narrowable_data;

unsigned narrowable_fetcher(
        void * source, char * buffer, unsigned n_items);

void narrowable_seeker(void * source, unsigned pos);

void assert_hunk_eq(
        hunk const * const h, unsigned const a_start, unsigned const a_end,
        unsigned const b_start, unsigned const b_end);

// Allows arrays to sneak by the preprocessor
#define Arr(...) __VA_ARGS__

#define Build_narrowable_data(name, n_vals, untils, values) \
    unsigned const until_##name[n_vals] = {untils}; \
    unsigned const values_##name[n_vals] = {values}; \
    narrowable_data name = (narrowable_data) { \
        .n_values = n_vals, .from = until_##name, .value = values_##name};
