#include "unistd.h"
#include <glib.h>
#include <sndfile.h>
#include "../include/adiff.h"

typedef struct {
    char * const temp_dir;
    char * const missing;
    char * const alt_sample_rate;
    char * const short0;
    char * const short_stereo0;
    char * const float0;
} adiff_fixture;

static void create_sndfile(char const * const path, SF_INFO info) {
    SNDFILE * f = sf_open(path, SFM_WRITE, &info);
    // Some data should really get written here
    sf_close(f);
}

static adiff_fixture create_fixture() {
    GError * err;
    char * temp_dir = g_dir_make_tmp("test_adiff_XXXXXX", &err);
    adiff_fixture fixture = {
        .temp_dir = temp_dir,
        .missing = g_build_filename(temp_dir, "missing", NULL),
        .alt_sample_rate = g_build_filename(temp_dir, "48khz", NULL),
        .short0 = g_build_filename(temp_dir, "short0", NULL),
        .short_stereo0 = g_build_filename(temp_dir, "short_stereo0", NULL),
        .float0 = g_build_filename(temp_dir, "float0", NULL),
        };
    create_sndfile(fixture.alt_sample_rate, (SF_INFO) {
        .channels = 1, .samplerate = 48000,
        .format = SF_FORMAT_WAV | SF_FORMAT_PCM_16});
    create_sndfile(fixture.short0, (SF_INFO) {
        .channels = 1, .samplerate = 44100,
        .format = SF_FORMAT_WAV | SF_FORMAT_PCM_16});
    create_sndfile(fixture.short_stereo0, (SF_INFO) {
        .channels = 2, .samplerate = 44100,
        .format = SF_FORMAT_WAV | SF_FORMAT_PCM_16});
    create_sndfile(fixture.float0, (SF_INFO) {
        .channels = 1, .samplerate = 44100,
        .format = SF_FORMAT_WAV | SF_FORMAT_FLOAT});
    return fixture;
}

static void cleanup_fixture(adiff_fixture f) {
    rmdir(f.temp_dir);
    g_free(f.temp_dir);
    g_free(f.missing);
    g_free(f.alt_sample_rate);
    g_free(f.short0);
    g_free(f.short_stereo0);
}

static void test_files_missing(gconstpointer ud) {
    adiff_fixture const * const f = ud;
    diff d = adiff(f->missing, f->short0);
    g_assert_cmpint(d.code, ==, ADIFF_ERR_OPEN_A);
    d = adiff(f->short0, f->missing);
    g_assert_cmpint(d.code, ==, ADIFF_ERR_OPEN_B);
}

static void test_sample_rate_mismatch(gconstpointer ud) {
    adiff_fixture const * const f = ud;
    diff d = adiff(f->alt_sample_rate, f->short0);
    g_assert_cmpint(d.code, ==, ADIFF_ERR_SAMPLE_RATE);
}

static void test_channels_mismatch(gconstpointer ud) {
    adiff_fixture const * const f = ud;
    diff d = adiff(f->short_stereo0, f->short0);
    g_assert_cmpint(d.code, ==, ADIFF_ERR_CHANNELS);
}

static void test_sample_format_mismatch(gconstpointer ud) {
    adiff_fixture const * const f = ud;
    diff d = adiff(f->float0, f->short0);
    g_assert_cmpint(d.code, ==, ADIFF_ERR_SAMPLE_FORMAT);
}

/*
static void test_short(adiff_fixture const f) {
}

static void test_float(adiff_fixture const f) {
}
*/

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    adiff_fixture fixture = create_fixture();
    g_test_add_data_func("/adiff/missing", &fixture, test_files_missing);
    g_test_add_data_func(
        "/adiff/sample_rate_mismatch", &fixture, test_sample_rate_mismatch);
    g_test_add_data_func(
        "/adiff/channels_mismatch", &fixture, test_channels_mismatch);
    g_test_add_data_func(
        "/adiff/sample_format_mismatch", &fixture, test_sample_format_mismatch);
    int const run_result = g_test_run();
    cleanup_fixture(fixture);
    return run_result;
}
