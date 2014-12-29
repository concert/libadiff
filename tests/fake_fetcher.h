#include <glib.h>

typedef struct {
    GRand * const g_rand;
    unsigned first_length;
    unsigned const second_length;
    unsigned pos;
} fake_fetcher_data;

unsigned fake_fetcher(void * source, unsigned n_items, char * buffer);
