#include <unistd.h>
#include <glib.h>
#include <sndfile.h>
#include "../include/adiff.h"
#include "../include/bdiff.h"  // data_fetcher
#include "fake_fetcher.h"

typedef struct {
    char * const temp_dir;
    char * const missing;
    char * const alt_sample_rate;
    char * const short0;
    fake_fetcher_data short0_ffd;
    char * const short1;
    fake_fetcher_data short1_ffd;
    char * const short_stereo0;
    char * const float0;
    fake_fetcher_data float0_ffd;
    char * const float1;
    fake_fetcher_data float1_ffd;
} adiff_fixture;

static void create_sndfile(
        char const * const path, SF_INFO info,
        fake_fetcher_data * const ffd) {
    SNDFILE * f = sf_open(path, SFM_WRITE, &info);
    if (ffd != NULL) {
        int buffer[1024];
        unsigned n_read = fake_fetcher(ffd, 1024, (char*) buffer);
        while (n_read) {
            sf_writef_int(f, buffer, n_read);
            n_read = fake_fetcher(ffd, 1024, (char*) buffer);
        }
    }
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
        .short0_ffd = (fake_fetcher_data) {
            .g_rand = g_rand_new_with_seed(92), .first_length = 500,
            .second_length = 19000},
        .short1 = g_build_filename(temp_dir, "short1", NULL),
        .short1_ffd = (fake_fetcher_data) {
            .g_rand = g_rand_new_with_seed(346), .first_length = 700,
            .second_length = 17000},
        .short_stereo0 = g_build_filename(temp_dir, "short_stereo0", NULL),
        .float0 = g_build_filename(temp_dir, "float0", NULL),
        .float0_ffd = (fake_fetcher_data) {
            .g_rand = g_rand_new_with_seed(92), .first_length = 500,
            .second_length = 19000},
        .float1 = g_build_filename(temp_dir, "float1", NULL),
        .float1_ffd = (fake_fetcher_data) {
            .g_rand = g_rand_new_with_seed(346), .first_length = 700,
            .second_length = 17000},
        };
    create_sndfile(
        fixture.alt_sample_rate,
        (SF_INFO) {
            .channels = 1, .samplerate = 48000,
            .format = SF_FORMAT_WAV | SF_FORMAT_PCM_16},
        NULL);
    create_sndfile(
        fixture.short0,
        (SF_INFO) {
            .channels = 1, .samplerate = 44100,
            .format = SF_FORMAT_WAV | SF_FORMAT_PCM_16},
        &fixture.short0_ffd);
    g_rand_free(fixture.short0_ffd.g_rand);
    create_sndfile(
        fixture.short1,
        (SF_INFO) {
            .channels = 1, .samplerate = 44100,
            .format = SF_FORMAT_WAV | SF_FORMAT_PCM_16},
        &fixture.short1_ffd);
    g_rand_free(fixture.short1_ffd.g_rand);
    create_sndfile(
        fixture.short_stereo0,
        (SF_INFO) {
            .channels = 2, .samplerate = 44100,
            .format = SF_FORMAT_WAV | SF_FORMAT_PCM_16},
        NULL);
    create_sndfile(
        fixture.float0,
        (SF_INFO) {
            .channels = 1, .samplerate = 44100,
            .format = SF_FORMAT_WAV | SF_FORMAT_FLOAT},
        &fixture.float0_ffd);
    g_rand_free(fixture.float0_ffd.g_rand);
    create_sndfile(
        fixture.float1,
        (SF_INFO) {
            .channels = 1, .samplerate = 44100,
            .format = SF_FORMAT_WAV | SF_FORMAT_FLOAT},
        &fixture.float1_ffd);
    g_rand_free(fixture.float1_ffd.g_rand);
    return fixture;
}

static void cleanup_fixture(adiff_fixture f) {
    #define rm_free(element) \
        remove(f.element); \
        g_free(f.element);
    rm_free(short0)
    rm_free(short1)
    rm_free(short_stereo0)
    rm_free(alt_sample_rate)
    rm_free(float0)
    rm_free(float1)
    rm_free(temp_dir)
    #undef rm_free
    g_free(f.missing);
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

static void diff_assertions(
        diff const * const d, fake_fetcher_data const * const a,
        fake_fetcher_data const * const b) {
    g_assert_cmpint(d->code, ==, ADIFF_OK);
    hunk const * const h = d->hunks;
    g_assert_nonnull(h);
    g_assert_cmpuint(h->a.start, ==, 0);
    g_assert_cmpuint(h->b.start, ==, 0);
    g_assert_cmpuint(h->a.end, >=, a->first_length + 1);
    g_assert_cmpuint(h->b.end, >=, b->first_length + 1);
    hunk const * const h2 = h->next;
    g_assert_nonnull(h2);
    g_assert_cmpuint(h2->b.start, <=, b->pos);
    g_assert_cmpuint(h2->a.start + 200, ==, h2->b.start);
    // Is the 400 something to do with each random number being 2 frames
    g_assert_cmpuint(h2->a.end, ==, a->pos + 1);
    g_assert_cmpuint(h2->b.end, ==, b->pos + 1);
    g_assert_null(h2->next);
}

static void test_short(gconstpointer ud) {
    adiff_fixture const * const f = ud;
    diff d = adiff(f->short0, f->short1);
    diff_assertions(&d, &f->short0_ffd, &f->short1_ffd);
    diff_free(&d);
}

static void test_float(gconstpointer ud) {
    adiff_fixture const * const f = ud;
    diff d = adiff(f->float0, f->float1);
    diff_assertions(&d, &f->float0_ffd, &f->float1_ffd);
    diff_free(&d);
}

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
    g_test_add_data_func(
        "/adiff/short", &fixture, test_short);
    g_test_add_data_func(
        "/adiff/float", &fixture, test_float);
    int const run_result = g_test_run();
    cleanup_fixture(fixture);
    return run_result;
}
