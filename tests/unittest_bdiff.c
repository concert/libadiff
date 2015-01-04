#include <glib.h>
#include "unittest_hash_counting_table.h"
#include "unittest_hunk.h"
#include "unittest_chunk.h"
#include "fake_fetcher.h"
#include "../include/bdiff.h"

static void end_to_end_test() {
    fake_fetcher_data dfa = {
        .g_rand = g_rand_new_with_seed(212), .first_length = 600,
        .second_length = 10000};
    fake_fetcher_data dfb = {
        .g_rand = g_rand_new_with_seed(121), .first_length = 400,
        .second_length = 9000};
    hunk * const h = bdiff(sizeof(guint32), fake_fetcher, &dfa, &dfb);
    g_assert_nonnull(h);
    g_assert_cmpuint(h->a.start, ==, 0);
    g_assert_cmpuint(h->b.start, ==, 0);
    g_assert_cmpuint(h->a.end, >=, 601);
    g_assert_cmpuint(h->b.end, >=, 401);
    hunk const * const h2 = h->next;
    g_assert_nonnull(h2);
    g_assert_cmpuint(h2->a.start, <=, 9401 + 200);
    g_assert_cmpuint(h2->b.start + 200, ==, h2->a.start);
    g_assert_cmpuint(h2->a.end, ==, 10601);
    g_assert_cmpuint(h2->b.end, ==, 9401);
    g_assert_null(h2->next);
    hunk_free(h);
    g_rand_free(dfa.g_rand);
    g_rand_free(dfb.g_rand);
}

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    add_chunk_tests();
    add_hash_counting_table_tests();
    add_hunk_tests();
    g_test_add_func("/bdiff/end_to_end", end_to_end_test);
    return g_test_run();
}
