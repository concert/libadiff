#pragma once

#include "diff_types.h"

typedef unsigned (*data_fetcher)(void * source, unsigned n_items, char * buffer);

diff_hunk * bdiff(data_fetcher df, void * a, void * b);
