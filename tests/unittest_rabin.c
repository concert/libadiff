#include <glib.h>
#include "rabin.h"

// Test that first byte comes out directly in the hash
static void test_pre_remainder_hash() {
    g_assert_cmphex(hash(0, 0xFF), ==, 0xFF);
}

// Test that the rolling hash settles
static void test_rolling_settles() {
    window w;
    for (unsigned i = 0; i < 5; i++) {
        w = windowed_hash(w, 0xFF);
    }
    unsigned h = w.hash;
    w = windowed_hash(w, 0xFF);
    g_assert_cmphex(h, ==, w.hash);
}

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/hash/pre_remainder", test_pre_remainder_hash);
    g_test_add_func("/windowed_hash/settles", test_rolling_settles);
    return g_test_run();
}
