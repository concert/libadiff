#include <glib.h>
#include "unittest_hash_counting_table.h"

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    add_hash_counting_table_tests();
    return g_test_run();
}
