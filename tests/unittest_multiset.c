#include <glib.h>
#include "hash_multiset.h"

typedef struct {
    GHashTable * hm;
} hm_fixture;

void hm_fixture_setup(hm_fixture *hmf, gconstpointer test_data) {
    hmf->hm = hash_multiset_new();
    hash_multiset_insert(hmf->hm, 1);
    hash_multiset_insert(hmf->hm, 2);
    hash_multiset_insert(hmf->hm, 2);
}

void hm_fixture_teardown(hm_fixture *hmf, gconstpointer test_data) {
    hash_multiset_destroy(hmf->hm);
}

void test_pop(hm_fixture *hmf, gconstpointer ignored) {
    g_assert_cmpuint(hash_multiset_pop(hmf->hm, 99), ==, 0);
    g_assert_cmpuint(hash_multiset_pop(hmf->hm, 1), ==, 1);
    g_assert_cmpuint(hash_multiset_pop(hmf->hm, 1), ==, 0);
    g_assert_cmpuint(hash_multiset_pop(hmf->hm, 2), ==, 2);
    g_assert_cmpuint(hash_multiset_pop(hmf->hm, 2), ==, 1);
}

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    g_test_add(
        "/hash_multiset/test_pop", hm_fixture, NULL,
        hm_fixture_setup, test_pop, hm_fixture_teardown);
    return g_test_run();
}
