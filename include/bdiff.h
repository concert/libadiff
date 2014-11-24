#pragma once

#include "diff_types.h"

typedef unsigned (*data_fetcher)(void * source, unsigned n_items, char * buffer);

diff_hunk * const bdiff(data_fetcher const df, void * const a, void * const b);
