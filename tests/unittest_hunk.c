#include "unittest_hunk.h"
#include <glib.h>
#include "hunk.h"

static void assertion_helper(
        hunk const * const h, unsigned const a_start, unsigned const a_end,
        unsigned const b_start, unsigned const b_end) {
    g_assert_nonnull(h);
    g_assert_cmpuint(h->a.start, ==, a_start);
    g_assert_cmpuint(h->a.end, ==, a_end);
    g_assert_cmpuint(h->b.start, ==, b_start);
    g_assert_cmpuint(h->b.end, ==, b_end);
}

static void test_insertion_at_start() {
    block b = {.start = 0, .end = 1, .other_start = 0};

    hunk * h = pair_blocks(NULL, &b);
    assertion_helper(h, 0, 0, 0, 1);
    g_assert_null(h->next);
    hunk_free(h);

    h = pair_blocks(&b, NULL);
    assertion_helper(h, 0, 1, 0, 0);
    g_assert_null(h->next);
    hunk_free(h);
}

static void test_insertion_at_end() {
    block b = {.start = 100, .end = 200, .other_start = 100};

    hunk * h = pair_blocks(&b, NULL);
    assertion_helper(h, 100, 200, 100, 100);
    g_assert_null(h->next);
    hunk_free(h);
}

static void test_change_at_start() {
    block b0 = {.start = 0, .end = 50, .other_start = 0};
    block b1 = {.start = 0, .end = 60, .other_start = 0};

    hunk * h = pair_blocks(&b0, &b1);
    assertion_helper(h, 0, 50, 0, 60);
    g_assert_null(h->next);
    hunk_free(h);
}

void add_hunk_tests() {
    g_test_add_func("/hunk/insertion_at_start", test_insertion_at_start);
    g_test_add_func("/hunk/insertion_at_end", test_insertion_at_end);
    g_test_add_func("/hunk/change_at_start", test_change_at_start);
}
