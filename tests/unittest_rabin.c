#include <glib.h>
#include <stdio.h>
#include "rabin.h"

// Test that first byte comes out directly in the hash
static void test_pre_remainder_hash() {
    hash_data hd = hash_data_init();
    g_assert_cmphex(hash_data_update(&hd, 0xAB), ==, 0x1AB);
}

static hash long_hash_helper(
        hash_data * const hd, char const * const bytes,
        unsigned const n_bytes) {
    for (unsigned i=0; i < n_bytes; i++) {
        hash_data_update(hd, bytes[i]);
    }
    return hd->h;
}

// Test f(a + b) = f(a) + f(b)
static void test_hash_distributive() {
    // Initialise hash to zero because otherwise it gets complicated to add the
    // streams
    hash_data hda = {};
    char const arr_a[2] = {0xF2, 0x34};
    char const arr_b[2] = {0xFB, 0xCD};
    for (unsigned i=0; i < 2; i++) {
        hash_data_update(&hda, arr_a[i] ^ arr_b[i]);
    }
    const hash hash_of_sum = hda.h;
    hda.h = 0;
    hash_data hdb = {};
    const hash sum_of_hash = long_hash_helper(&hda, arr_a, 2) ^ long_hash_helper(&hdb, arr_b, 2);
    g_assert_cmphex(hash_of_sum, ==, sum_of_hash);
}

// Test that the rolling hash settles
static void test_rolling_settles() {
    window_data w = window_data_init();
    for (unsigned i = 0; i < 4; i++) {
        windowed_hash(&w, 0xFF);
    }
    const unsigned h0 = windowed_hash(&w, 0xFF);
    const unsigned h1 = windowed_hash(&w, 0xFF);
    g_assert_cmphex(h0, ==, h1);
}

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/hash/pre_remainder", test_pre_remainder_hash);
    g_test_add_func("/hash/distributive", test_hash_distributive);
    g_test_add_func("/windowed_hash/settles", test_rolling_settles);
    return g_test_run();
}
