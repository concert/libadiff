#include "unistd.h"
#include <glib.h>
#include "../include/adiff.h"

typedef struct {
    char * const temp_dir;
    char * const missing;
    char * const short0;
} adiff_fixture;

static adiff_fixture create_fixture() {
    GError * err;
    char * temp_dir = g_dir_make_tmp("test_adiff_XXXXXX", &err);
    adiff_fixture fixture = {
        .temp_dir = temp_dir,
        .missing = g_build_filename(temp_dir, "missing", NULL),
        .short0 = g_build_filename(temp_dir, "short0", NULL)};
    return fixture;
}

static void cleanup_fixture(adiff_fixture f) {
    rmdir(f.temp_dir);
    g_free(f.temp_dir);
    g_free(f.missing);
    g_free(f.short0);
}

static void test_files_missing(gconstpointer ud) {
    adiff_fixture const * const f = ud;
    diff d = adiff(f->missing, f->short0);
    g_assert_cmpint(d.code, ==, ADIFF_ERR_OPEN_A);
}

/*
static void test_sample_rate_mismatch(adiff_fixture const f) {
}

static void test_sample_format_mismatch(adiff_fixture const f) {
}

static void test_short(adiff_fixture const f) {
}

static void test_float(adiff_fixture const f) {
}
*/

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    adiff_fixture fixture = create_fixture();
    g_test_add_data_func("/adiff/float", &fixture, test_files_missing);
    int const run_result = g_test_run();
    cleanup_fixture(fixture);
    return run_result;
}
