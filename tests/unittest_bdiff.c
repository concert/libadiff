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

typedef struct {
    unsigned pos;
    unsigned const n_values;
    unsigned const * const from;
    unsigned const * const value;
} narrowable_data;

static unsigned narrowable_fetcher(
        void * source, char * buffer, unsigned n_items) {
    narrowable_data * nd = source;
    unsigned * buf = (unsigned *) buffer;
    unsigned j = 0, n_output = 0;
    for (unsigned i = 0; i < n_items; i++) {
        while (nd->from[j] < nd->pos) {
            if (++j == nd->n_values) {
                break;
            }
        }
        if (j == nd->n_values) {
            break;
        }
        buf[i] = nd->value[j];
        nd->pos++;
        n_output++;
    }
    return n_output;
}

static void narrowable_seeker(void * source, unsigned pos) {
    narrowable_data * nd = source;
    nd->pos = pos;
}

static void narrowable_tools() {
    #define n_vals 3
    unsigned const until[n_vals] = {9, 19, 24};
    unsigned const values[n_vals] = {0, 1, 2};
    narrowable_data nd = (narrowable_data) {
        .n_values = n_vals, .from = until, .value = values};
    #undef n_vals
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
    #define n_values_a 3
    unsigned const until_a[n_values_a] = {9, 19, 24};
    unsigned const values_a[n_values_a] = {0, 1, 2};
    narrowable_data nda = (narrowable_data) {
        .n_values = n_values_a, .from = until_a, .value = values_a};
    #undef n_values_a
    #define n_values_b 3
    unsigned const until_b[n_values_b] = {9, 19, 24};
    unsigned const values_b[n_values_b] = {0, 3, 2};
    narrowable_data ndb = (narrowable_data) {
        .n_values = n_values_b, .from = until_b, .value = values_b};
    #undef n_values_b
    hunk * rough_head = NULL, * rough_tail = NULL;
    append_hunk(&rough_head, &rough_tail, 7, 23, 7, 23);
    hunk * precise_hunks = bdiff_narrow(
        rough_head, sizeof(unsigned), narrowable_seeker, narrowable_fetcher,
        &nda, &ndb);
    g_assert_nonnull(precise_hunks);
    g_assert_cmpuint(precise_hunks->a.start, ==, 10);
    g_assert_cmpuint(precise_hunks->a.end, ==, 20);
    g_assert_cmpuint(precise_hunks->b.start, ==, 10);
    g_assert_cmpuint(precise_hunks->b.end, ==, 20);
    g_assert_null(precise_hunks->next);
    hunk_free(rough_head);
    hunk_free(precise_hunks);
}

static void narrowing_differing_sizes() {
    #define n_vals 3
    unsigned const until_a[n_vals] = {9, 19, 24};
    unsigned const values_a[n_vals] = {0, 1, 2};
    narrowable_data nda = (narrowable_data) {
        .n_values = n_vals, .from = until_a, .value = values_a};
    unsigned const until_b[n_vals] = {9, 29, 34};
    unsigned const values_b[n_vals] = {0, 3, 2};
    narrowable_data ndb = (narrowable_data) {
        .n_values = n_vals, .from = until_b, .value = values_b};
    #undef n_vals
    hunk * rough_head = NULL, * rough_tail = NULL;
    append_hunk(&rough_head, &rough_tail, 6, 22, 6, 32);
    hunk * precise_hunks = bdiff_narrow(
        rough_head, sizeof(unsigned), narrowable_seeker, narrowable_fetcher,
        &nda, &ndb);
    g_assert_nonnull(precise_hunks);
    g_assert_cmpuint(precise_hunks->a.start, ==, 10);
    g_assert_cmpuint(precise_hunks->a.end, ==, 20);
    g_assert_cmpuint(precise_hunks->b.start, ==, 10);
    g_assert_cmpuint(precise_hunks->b.end, ==, 30);
    g_assert_null(precise_hunks->next);
    hunk_free(rough_head);
    hunk_free(precise_hunks);
}

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    add_chunk_tests();
    add_hash_counting_table_tests();
    add_hunk_tests();
    g_test_add_func("/bdiff/end_to_end", end_to_end_test);
    g_test_add_func("/bdiff/narrow_tools", narrowable_tools);
    g_test_add_func("/bdiff/narrow", narrowing);
    g_test_add_func("/bdiff/narrow_sizes_differ", narrowing_differing_sizes);
    return g_test_run();
}
