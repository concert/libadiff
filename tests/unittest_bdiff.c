#include <glib.h>
#include "unittest_hash_counting_table.h"
#include "unittest_hunk.h"
#include "unittest_chunk.h"
#include "unittest_narrowing.h"
#include "fake_fetcher.h"
#include "../include/bdiff.h"
#include "narrowable_test_tools.h"

static void bdiff_rough_test() {
    fake_fetcher_data dfa = {
        .g_rand = g_rand_new_with_seed(212), .first_length = 600,
        .second_length = 10000};
    fake_fetcher_data dfb = {
        .g_rand = g_rand_new_with_seed(121), .first_length = 400,
        .second_length = 9000};
    hunk * const h = bdiff_rough(sizeof(guint32), fake_fetcher, &dfa, &dfb);
    g_assert_nonnull(h);
    g_assert_cmpuint(h->a.start, ==, 0);
    g_assert_cmpuint(h->b.start, ==, 0);
    g_assert_cmpuint(h->a.end, >=, 600);
    g_assert_cmpuint(h->b.end, >=, 400);
    hunk const * const h2 = h->next;
    g_assert_nonnull(h2);
    g_assert_cmpuint(h2->a.start, <=, 9400 + 200);
    g_assert_cmpuint(h2->b.start + 200, ==, h2->a.start);
    g_assert_cmpuint(h2->a.end, ==, 10600);
    g_assert_cmpuint(h2->b.end, ==, 9400);
    g_assert_null(h2->next);
    hunk_free(h);
    g_rand_free(dfa.g_rand);
    g_rand_free(dfb.g_rand);
}

static void bdiff_combined_change() {
    Build_narrowable_data(nda, 3, Arr(150, 650, 700), Arr(0, 1, 0));
    Build_narrowable_data(ndb, 3, Arr(150, 650, 700), Arr(0, 2, 0));
    hunk * hunks = bdiff(
        sizeof(unsigned), narrowable_seeker, narrowable_fetcher, &nda, &ndb);
    assert_hunk_eq(hunks, 151, 651, 151, 651);
    g_assert_null(hunks->next);
    hunk_free(hunks);
}

static void bdiff_combined_insertion() {
    Build_narrowable_data(nda, 3, Arr(150, 650, 700), Arr(0, 1, 2));
    Build_narrowable_data(ndb, 2, Arr(150, 200), Arr(0, 2));
    hunk * hunks = bdiff(
        sizeof(unsigned), narrowable_seeker, narrowable_fetcher, &nda, &ndb);
    assert_hunk_eq(hunks, 151, 651, 151, 151);
    g_assert_null(hunks->next);
    hunk_free(hunks);
}

static void bdiff_combined_insertion_same_either_side() {
    Build_narrowable_data(nda, 3, Arr(150, 650, 700), Arr(0, 1, 0));
    Build_narrowable_data(ndb, 1, Arr(200), Arr(0));
    hunk * hunks = bdiff(
        sizeof(unsigned), narrowable_seeker, narrowable_fetcher, &nda, &ndb);
    assert_hunk_eq(hunks, 151, 651, 151, 151);
    g_assert_null(hunks->next);
    hunk_free(hunks);
    nda.pos = ndb.pos = 0;
    hunks = bdiff(
        sizeof(unsigned), narrowable_seeker, narrowable_fetcher, &ndb, &nda);
    assert_hunk_eq(hunks, 151, 151, 151, 651);
    g_assert_null(hunks->next);
    hunk_free(hunks);
}

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    add_chunk_tests();
    add_hash_counting_table_tests();
    add_hunk_tests();
    g_test_add_func("/bdiff/rough", bdiff_rough_test);
    add_narrowing_test_funcs();
    g_test_add_func("/bdiff/combined_change", bdiff_combined_change);
    g_test_add_func("/bdiff/combined_insert", bdiff_combined_insertion);
    g_test_add_func(
        "/bdiff/combined_highly_repetitive",
        bdiff_combined_insertion_same_either_side);
    return g_test_run();
}
