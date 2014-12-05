#include "unittest_chunk.h"
#include "chunk.h"
#include <glib.h>

typedef struct {
    GRand * const g_rand;
    unsigned const first_length;
    unsigned const second_length;
    unsigned pos;
} fake_fetcher_data;


static unsigned fake_fetcher(void * source, unsigned n_items, char * buffer) {
    fake_fetcher_data * const ffd = source;
    guint32 * gu_buf = buffer;
    unsigned const initial_pos = ffd->pos;
    while (ffd->pos < ffd->first_length) {
        if ((ffd->pos - initial_pos) == n_items) {
            break;
        }
        gu_buf[ffd->pos - initial_pos] = g_rand_int(ffd->g_rand);
        ffd->pos++;
    }
    if (ffd->pos == ffd->first_length) {
        g_rand_set_seed(ffd->g_rand, 2391);
    }
    while (ffd->pos < (ffd->first_length + ffd->second_length)) {
        if ((ffd->pos - initial_pos) == n_items) {
            break;
        }
        gu_buf[ffd->pos - initial_pos] = g_rand_int(ffd->g_rand);
        ffd->pos++;
    }
    return ffd->pos - initial_pos;
}

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
    df.pos = 0;
    chunks b = split_data(sizeof(guint32), fake_fetcher, &df);
    chunks last_a = a;
    while (last_a->next != NULL) {last_a = last_a->next;}
    chunks last_b = b;
    while (last_b->next != NULL) {last_b = last_b->next;}
    g_assert_cmphex(last_a->hash, ==, last_b->hash);
    g_assert_cmpuint(last_a->start, ==, last_b->start);
    g_assert_cmpuint(last_a->end, ==, last_b->end);
    chunk_free(a);
    chunk_free(b);
    g_rand_free(df.g_rand);
}

void add_chunk_tests() {
    g_test_add_func("/chunk/random", test_with_random_data);
}
