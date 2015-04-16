#include <glib.h>
#include "../include/bdiff.h"
#include "narrowable_test_tools.h"

static void narrowable_tools() {
    Build_narrowable_data(nd, 3, Arr(9, 19, 24), Arr(0, 1, 2));
    unsigned ints[25];
    g_assert_cmpuint(25, ==, narrowable_fetcher(&nd, (char *) ints, 25));
    g_assert_cmpuint(ints[0], ==, 0);
    g_assert_cmpuint(ints[9], ==, 0);
    g_assert_cmpuint(ints[10], ==, 1);
    g_assert_cmpuint(ints[19], ==, 1);
    g_assert_cmpuint(ints[20], ==, 2);
    g_assert_cmpuint(ints[24], ==, 2);
    g_assert_cmpuint(0, ==, narrowable_fetcher(&nd, (char *) ints, 25));
}

static void narrowing() {
    Build_narrowable_data(nda, 3, Arr(9, 19, 24), Arr(0, 1, 2));
    Build_narrowable_data(ndb, 3, Arr(9, 19, 24), Arr(0, 3, 2));
    hunk rough_hunks = (hunk) {
        .a = {.start = 7, .end = 23}, .b = {.start = 7, .end = 23}};
    hunk * precise_hunks = bdiff_narrow(
        &rough_hunks, sizeof(unsigned), narrowable_seeker, narrowable_fetcher,
        &nda, &ndb);
    assert_hunk_eq(precise_hunks, 10, 20, 10, 20);
    g_assert_null(precise_hunks->next);
    hunk_free(precise_hunks);
}

static void narrowing_differing_sizes() {
    Build_narrowable_data(nda, 3, Arr(9, 19, 24), Arr(0, 1, 2));
    Build_narrowable_data(ndb, 3, Arr(9, 29, 34), Arr(0, 3, 2));
    hunk rough_hunks = (hunk) {
        .a = {.start = 6, .end = 22}, .b = {.start = 6, .end = 32}};
    hunk * precise_hunks = bdiff_narrow(
        &rough_hunks, sizeof(unsigned), narrowable_seeker, narrowable_fetcher,
        &nda, &ndb);
    assert_hunk_eq(precise_hunks, 10, 20, 10, 30);
    g_assert_null(precise_hunks->next);
    hunk_free(precise_hunks);
}

static void narrowing_insertion() {
    Build_narrowable_data(nda, 3, Arr(9, 19, 24), Arr(0, 1, 0));
    Build_narrowable_data(ndb, 1, Arr(14), Arr(0));
    hunk rough_hunks = (hunk) {
        .a = {.start = 10, .end = 20}, .b = {.start = 10, .end = 10}};
    hunk * precise_hunks = bdiff_narrow(
        &rough_hunks, sizeof(unsigned), narrowable_seeker, narrowable_fetcher,
        &nda, &ndb);
    assert_hunk_eq(precise_hunks, 10, 20, 10, 10);
    g_assert_null(precise_hunks->next);
    hunk_free(precise_hunks);
}

static void insertion_at_end() {
    Build_narrowable_data(nda, 2, Arr(14, 19), Arr(0, 1));
    Build_narrowable_data(ndb, 1, Arr(14), Arr(0));
    hunk rough_hunks = (hunk) {
        .a = {.start = 15, .end = 20}, .b = {.start = 15, .end = 15}};
    hunk * precise_hunks = bdiff_narrow(
        &rough_hunks, sizeof(unsigned), narrowable_seeker, narrowable_fetcher,
        &nda, &ndb);
    assert_hunk_eq(precise_hunks, 15, 20, 15, 15);
    g_assert_null(precise_hunks->next);
    hunk_free(precise_hunks);
}

static void narrowing_change_over_start() {
    Build_narrowable_data(nda, 2, Arr(15, 30), Arr(2, 1));
    Build_narrowable_data(ndb, 2, Arr(17, 32), Arr(3, 1));
    hunk rough_hunks = (hunk) {
        .a = {.start = 0, .end = 20}, .b = {.start = 0, .end = 22}};
    hunk * precise_hunks = bdiff_narrow(
        &rough_hunks, sizeof(unsigned), narrowable_seeker, narrowable_fetcher,
        &nda, &ndb);
    assert_hunk_eq(precise_hunks, 0, 16, 0, 18);
    g_assert_null(precise_hunks->next);
    hunk_free(precise_hunks);
}

static void narrowing_change_is_start() {
    Build_narrowable_data(nda, 1, Arr(29), Arr(1));
    Build_narrowable_data(ndb, 2, Arr(9, 39), Arr(0, 1));
    hunk rough_hunks = (hunk) {
        .a = {.start = 0, .end = 25}, .b = {.start = 0, .end = 35}};
    hunk * precise_hunks = bdiff_narrow(
        &rough_hunks, sizeof(unsigned), narrowable_seeker, narrowable_fetcher,
        &nda, &ndb);
    assert_hunk_eq(precise_hunks, 0, 0, 0, 10);
    g_assert_null(precise_hunks->next);
    hunk_free(precise_hunks);
}

static void narrowing_change_over_end() {
    Build_narrowable_data(nda, 2, Arr(15, 30), Arr(0, 1));
    Build_narrowable_data(ndb, 2, Arr(15, 37), Arr(0, 2));
    hunk rough_hunks = (hunk) {
        .a = {.start = 12, .end = 31}, .b = {.start = 12, .end = 38}};
    hunk * precise_hunks = bdiff_narrow(
        &rough_hunks, sizeof(unsigned), narrowable_seeker, narrowable_fetcher,
        &nda, &ndb);
    assert_hunk_eq(precise_hunks, 16, 31, 16, 38);
    g_assert_null(precise_hunks->next);
    hunk_free(precise_hunks);
}

static void narrowing_change_is_end() {
    Build_narrowable_data(nda, 1, Arr(29), Arr(0));
    Build_narrowable_data(ndb, 1, Arr(37), Arr(0));
    hunk rough_hunks = (hunk) {
        .a = {.start = 12, .end = 30}, .b = {.start = 12, .end = 38}};
    hunk * precise_hunks = bdiff_narrow(
        &rough_hunks, sizeof(unsigned), narrowable_seeker, narrowable_fetcher,
        &nda, &ndb);
    assert_hunk_eq(precise_hunks, 30, 30, 30, 38);
    g_assert_null(precise_hunks->next);
    hunk_free(precise_hunks);
    rough_hunks = (hunk) {
        .a = {.start = 12, .end = 38}, .b = {.start = 12, .end = 30}};
    precise_hunks = bdiff_narrow(
        &rough_hunks, sizeof(unsigned), narrowable_seeker, narrowable_fetcher,
        &ndb, &nda);
    assert_hunk_eq(precise_hunks, 30, 38, 30, 30);
    g_assert_null(precise_hunks->next);
    hunk_free(precise_hunks);
}

static void narrowing_multihunk() {
    Build_narrowable_data(nda, 5, Arr(9, 19, 29, 39, 49), Arr(0, 1, 0, 1, 0));
    Build_narrowable_data(ndb, 1, Arr(49), Arr(0));
    hunk second_hunk = (hunk) {
        .a = {.start = 26, .end = 45}, .b = {.start = 26, .end = 45}};
    hunk first_hunk = (hunk) {
        .a = {.start = 8, .end = 22}, .b = {.start = 8, .end = 22},
        .next = &second_hunk};
    hunk * precise_hunks = bdiff_narrow(
        &first_hunk, sizeof(unsigned), narrowable_seeker, narrowable_fetcher,
        &nda, &ndb);
    assert_hunk_eq(precise_hunks, 10, 20, 10, 20);
    assert_hunk_eq(precise_hunks->next, 30, 40, 30, 40);
    g_assert_null(precise_hunks->next->next);
    hunk_free(precise_hunks);
}

static void narrowing_long_hunk() {
    Build_narrowable_data(nda, 3, Arr(15000, 65000, 70000), Arr(0, 1, 0));
    Build_narrowable_data(ndb, 1, Arr(70000), Arr(0));
    hunk rough_hunks = (hunk) {
        .a = {.start = 7324, .end = 69237},
        .b = {.start = 7324, .end = 69237}};
    hunk * precise_hunks = bdiff_narrow(
        &rough_hunks, sizeof(unsigned), narrowable_seeker, narrowable_fetcher,
        &nda, &ndb);
    assert_hunk_eq(precise_hunks, 15001, 65001, 15001, 65001);
    g_assert_null(precise_hunks->next);
    hunk_free(precise_hunks);
}

void add_narrowing_test_funcs() {
    g_test_add_func("/bdiff/narrow_tools", narrowable_tools);
    g_test_add_func("/bdiff/narrow", narrowing);
    g_test_add_func("/bdiff/narrow_sizes_differ", narrowing_differing_sizes);
    g_test_add_func("/bdiff/narrow_insertion", narrowing_insertion);
    g_test_add_func("/bdiff/insertion_at_end", insertion_at_end);
    g_test_add_func("/bdiff/narrow_over_start", narrowing_change_over_start);
    g_test_add_func("/bdiff/narrow_is_start", narrowing_change_is_start);
    g_test_add_func("/bdiff/narrow_over_end", narrowing_change_over_end);
    g_test_add_func("/bdiff/narrow_is_end", narrowing_change_is_end);
    g_test_add_func("/bdiff/narrow_multihunk", narrowing_multihunk);
    g_test_add_func("/bdiff/narrow_long", narrowing_long_hunk);
}
