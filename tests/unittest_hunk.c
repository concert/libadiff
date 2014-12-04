#include "unittest_hunk.h"
#include <glib.h>
#include "hunk.h"

static chunk c2 = {.start = 2, .end = 3, .hash = 3};
static chunk c1 = {.start = 1, .end = 2, .hash = 2, .next = &c2};
static chunk c0 = {.start = 0, .end = 1, .hash = 1, .next = &c1};

static void test_both_null() {
    g_assert_null(diff_chunks(NULL, NULL));
}

static void test_identical_files() {
    g_assert_null(diff_chunks(&c0, &c0));
}

static void assertion_helper(
        hunk const * const h, unsigned const a_start, unsigned const a_end,
        unsigned const b_start, unsigned const b_end) {
    g_assert_nonnull(h);
    g_assert_cmpuint(h->a.start, ==, a_start);
    g_assert_cmpuint(h->a.end, ==, a_end);
    g_assert_cmpuint(h->b.start, ==, b_start);
    g_assert_cmpuint(h->b.end, ==, b_end);
}

static void test_a_null() {
    hunk * h = diff_chunks(NULL, &c0);
    assertion_helper(h, 0, 0, 0, 3);
    g_assert_null(h->next);
    hunk_free(h);
}

static void test_b_null() {
    hunk * h = diff_chunks(&c0, NULL);
    assertion_helper(h, 0, 3, 0, 0);
    g_assert_null(h->next);
    hunk_free(h);
}

static void test_insert_at_start_a() {
    static chunk t2 = {.start = 1, .end = 2, .hash = 3};
    static chunk t1 = {.start = 0, .end = 1, .hash = 2, .next = &t2};
    hunk * h = diff_chunks(&c0, &t1);
    assertion_helper(h, 0, 1, 0, 0);
    g_assert_null(h->next);
    hunk_free(h);
}

static void test_insert_at_start_b() {
    static chunk t2 = {.start = 1, .end = 2, .hash = 3};
    static chunk t1 = {.start = 0, .end = 1, .hash = 2, .next = &t2};
    hunk * h = diff_chunks(&t1, &c0);
    assertion_helper(h, 0, 0, 0, 1);
    g_assert_null(h->next);
    hunk_free(h);
}

static void test_insert_at_end_a() {
    static chunk t3 = {.start = 3, .end = 4, .hash = 4};
    static chunk t2 = {.start = 2, .end = 3, .hash = 3, .next = &t3};
    static chunk t1 = {.start = 1, .end = 2, .hash = 2, .next = &t2};
    static chunk t0 = {.start = 0, .end = 1, .hash = 1, .next = &t1};
    hunk * h = diff_chunks(&t0, &c0);
    assertion_helper(h, 3, 4, 3, 3);
    g_assert_null(h->next);
    hunk_free(h);
}

static void test_insert_at_end_b() {
    static chunk t3 = {.start = 3, .end = 4, .hash = 4};
    static chunk t2 = {.start = 2, .end = 3, .hash = 3, .next = &t3};
    static chunk t1 = {.start = 1, .end = 2, .hash = 2, .next = &t2};
    static chunk t0 = {.start = 0, .end = 1, .hash = 1, .next = &t1};
    hunk * h = diff_chunks(&c0, &t0);
    assertion_helper(h, 3, 3, 3, 4);
    g_assert_null(h->next);
    hunk_free(h);
}

static void test_change_at_start() {
    chunk other = {.start = 0, .end = 1, .hash = 4, .next = &c1};
    hunk * h = diff_chunks(&c0, &other);
    assertion_helper(h, 0, 1, 0, 1);
    g_assert_null(h->next);
    hunk_free(h);
}

static void test_change_at_end() {
    chunk o3 = {.start = 3, .end = 4, .hash = 5};
    chunk o2 = {.start = 2, .end = 3, .hash = 4, .next = &o3};
    chunk o1 = {.start = 1, .end = 2, .hash = 2, .next = &o2};
    chunk o0 = {.start = 0, .end = 1, .hash = 1, .next = &o1};
    hunk * h = diff_chunks(&c0, &o0);
    assertion_helper(h, 2, 3, 2, 4);
    g_assert_null(h->next);
    hunk_free(h);
}

static void test_change_in_middle() {
    chunk o2 = {.start = 2, .end = 3, .hash = 3};
    chunk o1 = {.start = 1, .end = 2, .hash = 4, .next = &o2};
    chunk o0 = {.start = 0, .end = 1, .hash = 1, .next = &o1};
    hunk * h = diff_chunks(&c0, &o0);
    assertion_helper(h, 1, 2, 1, 2);
    g_assert_null(h->next);
    hunk_free(h);
}

static void test_multiple_chunks_differ() {
    chunk o7 = {.start = 10, .end = 13, .hash = 3};
    chunk o6 = {.start = 9, .end = 10, .hash = 2, .next = &o7};
    chunk o5 = {.start = 7, .end = 9, .hash = 6, .next = &o6};
    chunk o4 = {.start = 6, .end = 7, .hash = 5, .next = &o5};
    chunk o3 = {.start = 4, .end = 6, .hash = 4, .next = &o4};
    chunk o2 = {.start = 3, .end = 4, .hash = 3, .next = &o3};
    chunk o1 = {.start = 1, .end = 3, .hash = 2, .next = &o2};
    chunk o0 = {.start = 0, .end = 1, .hash = 1, .next = &o1};

    chunk t7 = {.start = 11, .end = 14, .hash = 3};
    chunk t6 = {.start = 10, .end = 11, .hash = 2, .next = &t7};
    chunk t5 = {.start = 8, .end = 10, .hash = 7, .next = &t6};
    chunk t4 = {.start = 5, .end = 8, .hash = 7, .next = &t5};
    chunk t3 = {.start = 3, .end = 5, .hash = 4, .next = &t4};
    chunk t2 = {.start = 2, .end = 3, .hash = 3, .next = &t3};
    chunk t1 = {.start = 0, .end = 2, .hash = 2, .next = &t2};

    hunk * h = diff_chunks(&o0, &t1);
    assertion_helper(h, 0, 1, 0, 0);
    g_assert_nonnull(h->next);

    hunk * h2 = h->next;
    assertion_helper(h2, 6, 9, 5, 10);
    g_assert_null(h2->next);

    hunk_free(h);
}

/*! Test the situation where we have multiple changes in a repetitve stream of
 * data, which results in duplicate "anchor" chunks:
 * A - - - [x]~ ~ ~ - - [x]~ ~
 * B - - - [x]~ ~   - - [x]~ ~ ~
*/
static void test_consecutive_duplicate_anchors() {
    chunk o9 = {.start = 9, .end = 10, .hash = 3};  // ~
    chunk o8 = {.start = 8, .end = 9, .hash = 3, .next = &o9};  // ~
    chunk o7 = {.start = 7, .end = 8, .hash = 1, .next = &o8};  // - [x]
    chunk o6 = {.start = 6, .end = 7, .hash = 1, .next = &o7};  // -
    chunk o5 = {.start = 5, .end = 6, .hash = 2, .next = &o6};  // ~
    chunk o4 = {.start = 4, .end = 5, .hash = 2, .next = &o5};  // ~
    chunk o3 = {.start = 3, .end = 4, .hash = 2, .next = &o4};  // ~
    chunk o2 = {.start = 2, .end = 3, .hash = 1, .next = &o3};  // - [x]
    chunk o1 = {.start = 1, .end = 2, .hash = 1, .next = &o2};  // -
    chunk o0 = {.start = 0, .end = 1, .hash = 1, .next = &o1};  // -

    chunk t9 = {.start = 9, .end = 10, .hash = 5};  // ~
    chunk t8 = {.start = 8, .end = 9, .hash = 5, .next = &t9};  // ~
    chunk t7 = {.start = 7, .end = 8, .hash = 5, .next = &t8};  // ~
    chunk t6 = {.start = 6, .end = 7, .hash = 1, .next = &t7};  // - [x]
    chunk t5 = {.start = 5, .end = 6, .hash = 1, .next = &t6};  // -
    chunk t4 = {.start = 4, .end = 5, .hash = 4, .next = &t5};  // ~
    chunk t3 = {.start = 3, .end = 4, .hash = 4, .next = &t4};  // ~
    chunk t2 = {.start = 2, .end = 3, .hash = 1, .next = &t3};  // - [x]
    chunk t1 = {.start = 1, .end = 2, .hash = 1, .next = &t2};  // -
    chunk t0 = {.start = 0, .end = 1, .hash = 1, .next = &t1};  // -

    hunk * h = diff_chunks(&o0, &t0);
    assertion_helper(h, 3, 6, 3, 5);
    g_assert_nonnull(h->next);

    hunk * h2 = h->next;
    assertion_helper(h2, 8, 10, 7, 10);
    g_assert_null(h2->next);

    hunk_free(h);
}

void add_hunk_tests() {
    g_test_add_func("/hunk/both_null", test_both_null);
    g_test_add_func("/hunk/identical_files", test_identical_files);
    g_test_add_func("/hunk/a_null", test_a_null);
    g_test_add_func("/hunk/b_null", test_b_null);
    g_test_add_func("/hunk/insert_at_start_a", test_insert_at_start_a);
    g_test_add_func("/hunk/insert_at_start_b", test_insert_at_start_b);
    g_test_add_func("/hunk/insert_at_end_a", test_insert_at_end_a);
    g_test_add_func("/hunk/insert_at_end_b", test_insert_at_end_b);
    g_test_add_func("/hunk/change_at_start", test_change_at_start);
    g_test_add_func("/hunk/change_at_end", test_change_at_end);
    g_test_add_func("/hunk/change_in_middle", test_change_in_middle);
    g_test_add_func(
        "/hunk/multiple_chunks_differ", test_multiple_chunks_differ);
    g_test_add_func(
        "/hunk/consecutive_duplicate_anchors",
        test_consecutive_duplicate_anchors);
}
