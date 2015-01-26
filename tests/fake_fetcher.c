#include "fake_fetcher.h"

unsigned fake_fetcher(void * source, char * buffer, unsigned n_items) {
    fake_fetcher_data * const ffd = source;
    guint32 * gu_buf = (guint32 *) buffer;
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
