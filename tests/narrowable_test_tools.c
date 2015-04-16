#include "narrowable_test_tools.h"
#include <glib.h>

unsigned narrowable_fetcher(
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

void narrowable_seeker(void * source, unsigned pos) {
    narrowable_data * nd = source;
    g_assert_cmpuint(pos, <=, nd->from[nd->n_values - 1] + 1);
    nd->pos = pos;
}

void assert_hunk_eq(
        hunk const * const h, unsigned const a_start, unsigned const a_end,
        unsigned const b_start, unsigned const b_end) {
    g_assert_nonnull(h);
    g_assert_cmpuint(h->a.start, ==, a_start);
    g_assert_cmpuint(h->a.end, ==, a_end);
    g_assert_cmpuint(h->b.start, ==, b_start);
    g_assert_cmpuint(h->b.end, ==, b_end);
}
