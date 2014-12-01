#include "unittest_hash_counting_table.h"
#include <glib.h>
#include "hash_counting_table.h"

typedef struct {
    GHashTable * hct;
} hm_fixture;

static void hm_fixture_setup(hm_fixture *hmf, gconstpointer test_data) {
    hmf->hct = hash_counting_table_new();
    hash_counting_table_inc(hmf->hct, 1);
    hash_counting_table_inc(hmf->hct, 2);
    hash_counting_table_inc(hmf->hct, 2);
}

static void hm_fixture_teardown(hm_fixture *hmf, gconstpointer test_data) {
    hash_counting_table_destroy(hmf->hct);
}

static void test_inc(hm_fixture *hmf, gconstpointer ignored) {
    g_assert_cmpuint(hash_counting_table_inc(hmf->hct, 3), ==, 0);
    g_assert_cmpuint(hash_counting_table_inc(hmf->hct, 3), ==, 1);
    g_assert_cmpuint(hash_counting_table_inc(hmf->hct, 2), ==, 2);
}

static void test_dec(hm_fixture *hmf, gconstpointer ignored) {
    g_assert_cmpuint(hash_counting_table_dec(hmf->hct, 99), ==, 0);
    g_assert_cmpuint(hash_counting_table_dec(hmf->hct, 1), ==, 1);
    g_assert_cmpuint(hash_counting_table_dec(hmf->hct, 1), ==, 0);
    g_assert_cmpuint(hash_counting_table_dec(hmf->hct, 2), ==, 2);
    g_assert_cmpuint(hash_counting_table_dec(hmf->hct, 2), ==, 1);
}

void add_hash_counting_table_tests() {
    g_test_add(
        "/hash_counting_table/test_inc", hm_fixture, NULL,
        hm_fixture_setup, test_inc, hm_fixture_teardown);
    g_test_add(
        "/hash_counting_table/test_dec", hm_fixture, NULL,
        hm_fixture_setup, test_dec, hm_fixture_teardown);
}
