#include "block.h"

block * block_new(
        block * prev, chunk const * const previous_common,
        const unsigned end) {
    return chunk_new_malloc(
        prev, previous_common->end, end, previous_common->hash);
}
