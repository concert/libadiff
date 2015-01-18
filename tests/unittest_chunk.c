#include "unittest_chunk.h"
#include "chunk.h"
#include "fake_fetcher.h"
#include <glib.h>

/*! Tests our chunking of two streams of data that start differently but end
 * with the same content. We assert that the last content-defined chunk of each
 * stream should be identical, given that we have enough random data in the
 * streams to have produced enough chunks, and that the data lengths of the two
 * streams are identical.
 */
static void test_with_random_data() {
    fake_fetcher_data df = {
        .g_rand = g_rand_new_with_seed(121), .first_length = 400,
        .second_length = 10000};
    chunks a = split_data(sizeof(guint32), fake_fetcher, &df, 1, 20000);
    g_rand_set_seed(df.g_rand, 212);
    df.first_length = 600;
    df.pos = 0;
    chunks b = split_data(sizeof(guint32), fake_fetcher, &df, 1, 20000);
    g_assert_cmphex(a->hash, !=, b->hash);
    g_assert_cmpuint(a->start, ==, 0);
    g_assert_cmpuint(b->start, ==, 0);
    g_assert_cmpuint(a->end, !=, b->end);
    chunks last_a = a;
    while (last_a->next != NULL) {last_a = last_a->next;}
    chunks last_b = b;
    while (last_b->next != NULL) {last_b = last_b->next;}
    g_assert_cmphex(last_a->hash, ==, last_b->hash);
    g_assert_cmpuint(last_a->start + 200, ==, last_b->start);
    g_assert_cmpuint(last_a->end + 200, ==, last_b->end);
    g_assert_cmpuint(last_a->end, ==, 10401);
    chunk_free(a);
    chunk_free(b);
    g_rand_free(df.g_rand);
}

static unsigned immediate_split_fetcher(
        void * source, unsigned n_items, char * buffer) {
    unsigned * const n_remaining = source;
    unsigned n = (*n_remaining > n_items) ? n_items : *n_remaining;
    for (unsigned i = 0; i < n; i++) {
        buffer[i] = 0;
    }
    *n_remaining -= n;
    return n;
}

static void test_minimum_chunk_length() {
    unsigned total_length = 2;
    chunks c = split_data(1, immediate_split_fetcher, &total_length, 1, 50);
    g_assert_nonnull(c->next);
    g_assert_cmpuint(c->end, ==, 1);
    chunk_free(c);
    total_length = 2;
    c = split_data(1, immediate_split_fetcher, &total_length, 2, 50);
    g_assert_null(c->next);
    chunk_free(c);
}

static void test_maximum_chunk_length() {
    unsigned total_length = 6;
    chunks c = split_data(1, immediate_split_fetcher, &total_length, total_length, 3);
    g_assert_nonnull(c->next);
    g_assert_cmpuint(c->end, ==, 4);
    chunk_free(c);
}

void add_chunk_tests() {
    g_test_add_func("/chunk/random", test_with_random_data);
    g_test_add_func("/chunk/min_length", test_minimum_chunk_length);
    g_test_add_func("/chunk/max_length", test_maximum_chunk_length);
}
