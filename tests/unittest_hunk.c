#include "unittest_hunk.h"
#include <glib.h>
#include "hunk.h"

static chunk c2 = {.start = 2, .end = 3, .hash = 3};
static chunk c1 = {.start = 1, .end = 2, .hash = 2, .next = &c2};
static chunk c0 = {.start = 0, .end = 1, .hash = 1, .next = &c1};

static void test_both_null() {
    g_assert_null(hunk_factory(NULL, NULL));
}

static void test_identical_files() {
    g_assert_null(hunk_factory(&c0, &c0));
}

/*
static void full_contiguous_block_helper(chunks other) {
    blocks b = hunk_factory(&c0, other);
    g_assert_cmpuint(b->start, ==, 0);
    g_assert_cmpuint(b->end, ==, 3);
    g_assert_cmpuint(b->counterpart_start, ==, 0);
    g_assert_null(b->next);
    block_free(b);
}

static void test_theirs_null() {
    full_contiguous_block_helper(NULL);
}

static void test_nothing_unique() {
    g_assert_null(hunk_factory(&c0, &c0));
}

static void test_all_unique() {
    chunk other = {.start = 0, .end = 4, .hash = 4};
    full_contiguous_block_helper(&other);
}

static void test_insert_at_start() {
    blocks b = hunk_factory(&c0, &c1);
    g_assert_cmpuint(b->start, ==, 0);
    g_assert_cmpuint(b->end, ==, 1);
    g_assert_cmpuint(b->counterpart_start, ==, 0);
    g_assert_null(b->next);
    block_free(b);
}

static void test_change_at_start() {
    chunk other = {.start = 0, .end = 1, .hash = 4, .next = &c1};
    blocks b = hunk_factory(&c0, &other);
    g_assert_cmpuint(b->start, ==, 0);
    g_assert_cmpuint(b->end, ==, 1);
    g_assert_cmpuint(b->counterpart_start, ==, 0);
    g_assert_null(b->next);
    block_free(b);
}

static void test_change_at_end() {
    chunk o2 = {.start = 2, .end = 3, .hash = 4};
    chunk o1 = {.start = 1, .end = 2, .hash = 2, .next = &o2};
    chunk o0 = {.start = 0, .end = 1, .hash = 1, .next = &o1};
    blocks b = hunk_factory(&c0, &o0);
    g_assert_cmpuint(b->start, ==, 2);
    g_assert_cmpuint(b->end, ==, 3);
    g_assert_cmpuint(b->counterpart_start, ==, 2);
    g_assert_null(b->next);
    block_free(b);
}

static void test_change_in_middle() {
    chunk o2 = {.start = 2, .end = 3, .hash = 3};
    chunk o1 = {.start = 1, .end = 2, .hash = 4, .next = &o2};
    chunk o0 = {.start = 0, .end = 1, .hash = 1, .next = &o1};
    blocks b = hunk_factory(&c0, &o0);
    g_assert_cmpuint(b->start, ==, 1);
    g_assert_cmpuint(b->end, ==, 2);
    g_assert_cmpuint(b->counterpart_start, ==, 1);
    g_assert_null(b->next);
    block_free(b);
}

static void test_multiple_blocks_differ() {
    chunk o7 = {.start = 7, .end = 8, .hash = 3};
    chunk o6 = {.start = 6, .end = 7, .hash = 2, .next = &o7};
    chunk o5 = {.start = 5, .end = 6, .hash = 6, .next = &o6};
    chunk o4 = {.start = 4, .end = 5, .hash = 5, .next = &o5};
    chunk o3 = {.start = 3, .end = 4, .hash = 4, .next = &o4};
    chunk o2 = {.start = 2, .end = 3, .hash = 3, .next = &o3};
    chunk o1 = {.start = 1, .end = 2, .hash = 2, .next = &o2};
    chunk o0 = {.start = 0, .end = 1, .hash = 1, .next = &o1};

    chunk t7 = {.start = 6, .end = 7, .hash = 3};
    chunk t6 = {.start = 5, .end = 6, .hash = 2, .next = &t7};
    chunk t5 = {.start = 4, .end = 5, .hash = 7, .next = &t6};
    chunk t4 = {.start = 3, .end = 4, .hash = 7, .next = &t5};
    chunk t3 = {.start = 2, .end = 3, .hash = 4, .next = &t4};
    chunk t2 = {.start = 1, .end = 2, .hash = 3, .next = &t3};
    chunk t1 = {.start = 0, .end = 1, .hash = 2, .next = &t2};

    blocks b = hunk_factory(&o0, &t1);
    g_assert_cmpuint(b->start, ==, 0);
    g_assert_cmpuint(b->end, ==, 1);
    g_assert_cmpuint(b->counterpart_start, ==, 0);
    g_assert_nonnull(b->next);

    blocks b2 = b->next;
    g_assert_cmpuint(b2->start, ==, 4);
    g_assert_cmpuint(b2->end, ==, 6);
    g_assert_cmpuint(b->counterpart_start, ==, 3);
    g_assert_null(b2->next);

    block_free(b);
}
*/

/*! Test the situation where we have multiple changes in a repetitve stream of
 * data, which results in duplicate "anchor" chunks:
 * A - - - [x]~ ~ ~ - - [x]~ ~
 * B - - - [x]~ ~   - - [x]~ ~ ~
*/
/*
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

    blocks b = hunk_factory(&o0, &t0);
    g_assert_cmpint(b->start, ==, 3);
    g_assert_cmpint(b->end, ==, 6);
    g_assert_cmpint(b->counterpart_start, ==, 3);
    g_assert_nonnull(b->next);

    blocks b2 = b->next;
    g_assert_cmpint(b->start, ==, 8);
    g_assert_cmpint(b->end, ==, 10);
    g_assert_cmpint(b->counterpart_start, ==, 7);
    g_assert_null(b->next);
    block_free(b);
}
*/

void add_hunk_tests() {
    g_test_add_func("/hunk/both_null", test_both_null);
    g_test_add_func("/hunk/identical_files", test_identical_files);
    /*
    g_test_add_func("/block/nothing_unique", test_nothing_unique);
    g_test_add_func("/block/all_unique", test_all_unique);
    g_test_add_func("/block/insert_at_start", test_insert_at_start);
    g_test_add_func("/block/change_at_start", test_change_at_start);
    g_test_add_func("/block/change_at_end", test_change_at_end);
    g_test_add_func("/block/change_in_middle", test_change_in_middle);
    g_test_add_func("/block/multiple_blocks_differ", test_change_in_middle);
    g_test_add_func(
        "/block/consecutive_duplicate_anchors",
        test_consecutive_duplicate_anchors);
    */
}
