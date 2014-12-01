#include "unittest_block.h"
#include <glib.h>
#include "block.h"

static chunk c2 = {.start = 2, .end = 3, .hash = 3};
static chunk c1 = {.start = 1, .end = 2, .hash = 2, .next = &c2};
static chunk c0 = {.start = 0, .end = 1, .hash = 1, .next = &c1};

static void test_ours_null() {
    g_assert_null(unique_blocks(NULL, &c0));
}

static void full_contiguous_block_helper(blocks other) {
    blocks b = unique_blocks(&c0, other);
    g_assert_cmpuint(b->hash, ==, 0);
    g_assert_cmpuint(b->start, ==, 0);
    g_assert_cmpuint(b->end, ==, 3);
    g_assert_null(b->next);
    block_free(b);
}

static void test_theirs_null() {
    full_contiguous_block_helper(NULL);
}

static void test_nothing_unique() {
    g_assert_null(unique_blocks(&c0, &c0));
}

static void test_all_unique() {
    chunk other = {.start = 0, .end = 4, .hash = 4};
    full_contiguous_block_helper(&other);
}

static void test_insert_at_start() {
    blocks b = unique_blocks(&c0, &c1);
    g_assert_cmpuint(b->hash, ==, 0);
    g_assert_cmpuint(b->start, ==, 0);
    g_assert_cmpuint(b->end, ==, 1);
    g_assert_null(b->next);
    block_free(b);
}

static void test_change_at_start() {
    chunk other = {.start = 0, .end = 1, .hash = 4, .next = &c1};
    blocks b = unique_blocks(&c0, &other);
    g_assert_cmpuint(b->hash, ==, 0);
    g_assert_cmpuint(b->start, ==, 0);
    g_assert_cmpuint(b->end, ==, 1);
    g_assert_null(b->next);
    block_free(b);
}

static void test_change_at_end() {
    chunk o2 = {.start = 2, .end = 3, .hash = 4};
    chunk o1 = {.start = 1, .end = 2, .hash = 2, .next = &o2};
    chunk o0 = {.start = 0, .end = 1, .hash = 1, .next = &o1};
    blocks b = unique_blocks(&c0, &o0);
    g_assert_cmpuint(b->hash, ==, 2);
    g_assert_cmpuint(b->start, ==, 2);
    g_assert_cmpuint(b->end, ==, 3);
    g_assert_null(b->next);
    block_free(b);
}

static void test_change_in_middle() {
    chunk o2 = {.start = 2, .end = 3, .hash = 3};
    chunk o1 = {.start = 1, .end = 2, .hash = 4, .next = &o2};
    chunk o0 = {.start = 0, .end = 1, .hash = 1, .next = &o1};
    blocks b = unique_blocks(&c0, &o0);
    g_assert_cmpuint(b->hash, ==, 1);
    g_assert_cmpuint(b->start, ==, 1);
    g_assert_cmpuint(b->end, ==, 2);
    g_assert_null(b->next);
    block_free(b);
}

void add_block_tests() {
    g_test_add_func("/block/ours_null", test_ours_null);
    g_test_add_func("/block/theirs_null", test_theirs_null);
    g_test_add_func("/block/nothing_unique", test_nothing_unique);
    g_test_add_func("/block/all_unique", test_all_unique);
    g_test_add_func("/block/insert_at_start", test_insert_at_start);
    g_test_add_func("/block/change_at_start", test_change_at_start);
    g_test_add_func("/block/change_at_end", test_change_at_end);
    g_test_add_func("/block/change_in_middle", test_change_in_middle);
    // TODO: test multiple blocks of changes
}
