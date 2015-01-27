#include <glib.h>

typedef struct {
    GRand * g_rand;
    unsigned first_length;
    unsigned second_length;
    unsigned pos;
} fake_fetcher_data;

unsigned fake_fetcher(void * source, char * buffer, unsigned n_items);
