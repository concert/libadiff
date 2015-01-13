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
    chunks a = split_data(sizeof(guint32), fake_fetcher, &df);
    g_rand_set_seed(df.g_rand, 212);
    df.first_length = 600;
    df.pos = 0;
    chunks b = split_data(sizeof(guint32), fake_fetcher, &df);
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

void add_chunk_tests() {
    g_test_add_func("/chunk/random", test_with_random_data);
}
