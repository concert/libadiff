#include <glib.h>

typedef struct fake_fetcher_data {
    GRand * g_rand;
    unsigned first_length;
    unsigned second_length;
    unsigned pos;
} fake_fetcher_data;

unsigned fake_fetcher(void * source, unsigned n_items, char * buffer);
