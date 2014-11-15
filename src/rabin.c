#include <stdio.h>
#include <math.h>

const unsigned hash_len = 9;
// irreducible_polynomial = pow(2, 9) + pow(2, 1) + pow(2, 0);
const unsigned irreducible_polynomial = 515;

const unsigned window_size = 12;
// unsigned one_over = pow(2, 3) + pow(2, 2);
const unsigned one_over = 12;

static inline unsigned sub(unsigned a, unsigned b) {
    return a ^ b;
}

unsigned f(unsigned i) {
    if (i >= pow(2, 9)) {
        return sub(i, irreducible_polynomial);
    }
    return i;
}

int main() {
    unsigned h = 0;
    for (unsigned i; i <= 30; i++) {
        if ((i >= window_size) && (i != 15 + 12)) {
            h = sub(h, one_over);
        }
        if (i == 15) {
            h = h << 1;
        } else {
            h = (h << 1) | 1;
        }
        h = f(h);
        printf("pos: %u hash: %u\n", i, h);
    }
}
