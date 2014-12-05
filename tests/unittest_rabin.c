#include <glib.h>
#include <stdio.h>
#include "rabin.h"

// 2**32 (truncated) + 2**7 + 2**3 + 2**2 + 2**0
hash const irreducible_polynomial = 141;

// Test that first byte comes out directly in the hash
static void test_pre_remainder_hash() {
    hash_data hd = hash_data_init(irreducible_polynomial);
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
    hash_data hda = hash_data_init(irreducible_polynomial);
    hda.h = 0;
    #define n_bytes 5
    char const arr_a[n_bytes] = {0xF2, 0x34, 0x11, 0xF4, 0x9B};
    char const arr_b[n_bytes] = {0xFB, 0xCD, 0xAD, 0x42, 0xE2};
    for (unsigned i=0; i < n_bytes; i++) {
        hash_data_update(&hda, arr_a[i] ^ arr_b[i]);
    }
    const hash hash_of_sum = hda.h;
    hda.h = 0;
    hash_data hdb = hash_data_init(irreducible_polynomial);
    hdb.h = 0;
    const hash sum_of_hash =
        long_hash_helper(&hda, arr_a, n_bytes) ^
        long_hash_helper(&hdb, arr_b, n_bytes);
    #undef n_bytes
    g_assert_cmphex(hash_of_sum, ==, sum_of_hash);
}

static void test_rolling_settles() {
    hash_data hd = hash_data_init(irreducible_polynomial);
    #define window_size 16
    unsigned char buffer[window_size];
    window_data w = window_data_init(&hd, buffer, window_size);
    unsigned i;
    for (i = 0; i < window_size - 2; i++) {
        window_data_update(&w, 0xFF);
    }
    unsigned h0 = window_data_update(&w, 0xFF);
    unsigned h1 = window_data_update(&w, 0xFF);
    g_assert_cmphex(h0, !=, h1);
    h0 = h1;
    h1 = window_data_update(&w, 0xFF);
    g_assert_cmphex(h0, ==, h1);
    unsigned const ff_hash = h1;

    h0 = h1;
    h1 = window_data_update(&w, 0xAB);
    g_assert_cmphex(h0, !=, h1);
    for (i = 0; i < window_size - 3; i++) {
        window_data_update(&w, 0xAB);
    }
    h0 = window_data_update(&w, 0xAB);
    h1 = window_data_update(&w, 0xAB);
    g_assert_cmphex(h0, !=, h1);
    h0 = h1;
    h1 = window_data_update(&w, 0xAB);
    g_assert_cmphex(h0, ==, h1);
    g_assert_cmphex(h1, !=, ff_hash);

    for (i = 0; i < window_size; i++) {
        h0 = window_data_update(&w, 0xFF);
    }
    g_assert_cmphex(h0, ==, ff_hash);
    #undef window_size
}

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/hash/pre_remainder", test_pre_remainder_hash);
    g_test_add_func("/hash/distributive", test_hash_distributive);
    g_test_add_func("/windowed_hash/settles", test_rolling_settles);
    return g_test_run();
}
