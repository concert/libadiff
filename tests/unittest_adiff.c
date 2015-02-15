#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <glib.h>
#include <sndfile.h>
#include "../include/adiff.h"
#include "../include/bdiff.h"  // data_fetcher
#include "fake_fetcher.h"

typedef struct {
    union {
        fake_fetcher_data;
        fake_fetcher_data ffd;
    };
    unsigned seed;
} file_content_desc;

typedef struct {
    char * const temp_dir;
    char * const missing;
    char * const alt_sample_rate;
    char * const short0;
    file_content_desc fcd0;
    char * const short1;
    file_content_desc fcd1;
    char * const short_stereo0;
    char * const int0;
    char * const int1;
    char * const float0;
    char * const float1;
    char * const double0;
    char * const double1;
} adiff_fixture;

static void create_sndfile(
        char const * const path, SF_INFO info,
        file_content_desc * const fcd) {
    SNDFILE * f = sf_open(path, SFM_WRITE, &info);
    if (fcd != NULL) {
        fcd->pos = 0;
        fcd->g_rand = g_rand_new_with_seed(fcd->seed);
        int buffer[1024];
        unsigned n_read = fake_fetcher(&fcd->ffd, (char*) buffer, 1024);
        while (n_read) {
            sf_writef_int(f, buffer, n_read);
            n_read = fake_fetcher(&fcd->ffd, (char*) buffer, 1024);
        }
        g_rand_free(fcd->g_rand);
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
        .fcd0 = (file_content_desc) {
            .seed = 92, .first_length = 500, .second_length = 19000},
        .short1 = g_build_filename(temp_dir, "short1", NULL),
        .fcd1 = (file_content_desc) {
            .seed = 346, .first_length = 700, .second_length = 17000},
        .int0 = g_build_filename(temp_dir, "int0", NULL),
        .int1 = g_build_filename(temp_dir, "int1", NULL),
        .short_stereo0 = g_build_filename(temp_dir, "short_stereo0", NULL),
        .float0 = g_build_filename(temp_dir, "float0", NULL),
        .float1 = g_build_filename(temp_dir, "float1", NULL),
        .double0 = g_build_filename(temp_dir, "double0", NULL),
        .double1 = g_build_filename(temp_dir, "double1", NULL),
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
        &fixture.fcd0);
    create_sndfile(
        fixture.short1,
        (SF_INFO) {
            .channels = 1, .samplerate = 44100,
            .format = SF_FORMAT_WAV | SF_FORMAT_PCM_16},
        &fixture.fcd1);
    create_sndfile(
        fixture.int0,
        (SF_INFO) {
            .channels = 1, .samplerate = 44100,
            .format = SF_FORMAT_WAV | SF_FORMAT_PCM_32},
        &fixture.fcd0);
    create_sndfile(
        fixture.int1,
        (SF_INFO) {
            .channels = 1, .samplerate = 44100,
            .format = SF_FORMAT_WAV | SF_FORMAT_PCM_32},
        &fixture.fcd1);
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
        &fixture.fcd0);
    create_sndfile(
        fixture.float1,
        (SF_INFO) {
            .channels = 1, .samplerate = 44100,
            .format = SF_FORMAT_WAV | SF_FORMAT_FLOAT},
        &fixture.fcd1);
    create_sndfile(
        fixture.double0,
        (SF_INFO) {
            .channels = 1, .samplerate = 44100,
            .format = SF_FORMAT_WAV | SF_FORMAT_DOUBLE},
        &fixture.fcd0);
    create_sndfile(
        fixture.double1,
        (SF_INFO) {
            .channels = 1, .samplerate = 44100,
            .format = SF_FORMAT_WAV | SF_FORMAT_DOUBLE},
        &fixture.fcd1);
    return fixture;
}

static void cleanup_fixture(adiff_fixture f) {
    #define rm_free(element) \
        g_assert_cmpint(remove(f.element), ==, 0); \
        g_free(f.element);
    rm_free(short0)
    rm_free(short1)
    rm_free(int0)
    rm_free(int1)
    rm_free(short_stereo0)
    rm_free(alt_sample_rate)
    rm_free(float0)
    rm_free(float1)
    rm_free(double0)
    rm_free(double1)
    rm_free(temp_dir)
    #undef rm_free
    g_free(f.missing);
}

static void test_adiff_files_missing(gconstpointer ud) {
    adiff_fixture const * const f = ud;
    diff d = adiff(f->missing, f->short0);
    g_assert_cmpint(d.code, ==, ADIFF_ERR_OPEN_A);
    d = adiff(f->short0, f->missing);
    g_assert_cmpint(d.code, ==, ADIFF_ERR_OPEN_B);
}

static void test_apatch_file_open_errors(gconstpointer ud) {
    adiff_fixture const * const f = ud;
    char * patch_outfile = g_build_filename(f->temp_dir, "patch_result", NULL);
    apatch_return_code code = apatch(
        NULL, f->missing, f->short0, patch_outfile);
    g_assert_cmpint(code, ==, APATCH_ERR_OPEN_A);
    code = apatch(NULL, f->short0, f->missing, patch_outfile);
    g_assert_cmpint(code, ==, APATCH_ERR_OPEN_B);
    char * ned_outfile = g_build_filename(
        f->temp_dir, "not_a_dir", "patch_result", NULL);
    code = apatch(NULL, f->short0, f->short1, ned_outfile);
    g_assert_cmpint(code, ==, APATCH_ERR_OPEN_OUTPUT);
    g_free(ned_outfile);
    g_assert_cmpint(remove(patch_outfile), ==, -1);
    g_free(patch_outfile);
}

static void test_adiff_sample_rate_mismatch(gconstpointer ud) {
    adiff_fixture const * const f = ud;
    diff d = adiff(f->alt_sample_rate, f->short0);
    g_assert_cmpint(d.code, ==, ADIFF_ERR_SAMPLE_RATE);
}

static void test_adiff_channels_mismatch(gconstpointer ud) {
    adiff_fixture const * const f = ud;
    diff d = adiff(f->short_stereo0, f->short0);
    g_assert_cmpint(d.code, ==, ADIFF_ERR_CHANNELS);
}

static void test_adiff_sample_format_mismatch(gconstpointer ud) {
    adiff_fixture const * const f = ud;
    diff d = adiff(f->float0, f->short0);
    g_assert_cmpint(d.code, ==, ADIFF_ERR_SAMPLE_FORMAT);
}

static void diff_assertions(
        diff const * const d, file_content_desc const * const a,
        file_content_desc const * const b) {
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
    g_assert_cmpuint(h2->a.end, ==, a->pos + 1);
    g_assert_cmpuint(h2->b.end, ==, b->pos + 1);
    g_assert_null(h2->next);
}

static gchar * riff_id(gchar const * const content) {
    static gchar identifier[5] = {};
    memcpy(identifier, content, 4);
    return identifier;
}

static void assert_riff_equivalent_possibly_newer(
        gchar const * a_contents, gchar const * b_contents, gsize length) {
    g_assert_cmpuint(length, >=, 4);
    g_assert_cmpstr(riff_id(a_contents), ==, "RIFF");
    g_assert_cmpstr(riff_id(b_contents), ==, "RIFF");
    #define Advance(n) length -= n; a_contents += n; b_contents += n;
    Advance(4)
    g_assert_cmpuint(length, >=, 4);
    // The following will go wrong on big endian architectures:
    #define to_uint(buf) *((uint32_t *) buf)
    uint32_t a_size = to_uint(a_contents);
    uint32_t b_size = to_uint(b_contents);
    g_assert_cmpuint(a_size, ==, length - 4);
    g_assert_cmpuint(b_size, ==, length - 4);
    Advance(sizeof(uint32_t))
    g_assert_cmpuint(length, >=, 4);
    g_assert_cmpstr(riff_id(a_contents), ==, "WAVE");
    g_assert_cmpstr(riff_id(b_contents), ==, "WAVE");
    Advance(4)
    while (length) {
        g_assert_cmpuint(length, >=, 4 + sizeof(uint32_t));
        gchar const * const a_id = riff_id(a_contents);
        gchar const * const b_id = riff_id(b_contents);
        g_assert_cmpstr(a_id, ==, b_id);
        Advance(4)
        a_size = to_uint(a_contents);
        b_size = to_uint(b_contents);
        g_assert_cmpuint(a_size, ==, b_size);
        g_assert_cmpuint(a_size, <=, length - 4);
        Advance(4)
        if (!strcmp(a_id, "PEAK")) {
            // PEAK subchunk
            g_assert_cmpuint(a_size, >=, 2 * sizeof(uint32_t));
            // Version:
            g_assert_cmpuint(to_uint(a_contents), ==, 1);
            g_assert_cmpuint(to_uint(b_contents), ==, 1);
            Advance(sizeof(uint32_t))
            // Timestamp:
            g_assert_cmpuint(to_uint(a_contents), <=, to_uint(b_contents));
            Advance(sizeof(uint32_t))
            a_size -= 2 * sizeof(uint32_t);
        }
        // Binary diff the rest
        for (uint32_t i = 0; i < a_size; i++) {
            if (a_contents[i] != b_contents[i]) {
                printf("Byte %u of %u (chunk %s) differed\n", i, a_size, a_id);
                g_test_fail();
            }
        }
        Advance(a_size)
    }
    #undef to_uint
    #undef Advance
}

static void files_identical(char const * const a, char const * const b) {
    // Not that memory efficient, it uses twice the size of the files, but this
    // is a unit test and the files are pretty small (O(kb))
    gchar * a_contents = NULL;
    gsize a_length = 0;
    GError * error = NULL;
    g_assert(g_file_get_contents(a, &a_contents, &a_length, &error));
    g_assert_no_error(error);
    g_assert_cmpuint(a_length, >, 0);
    gchar * b_contents = NULL;
    gsize b_length = 0;
    g_assert(g_file_get_contents(b, &b_contents, &b_length, &error));
    g_assert_no_error(error);
    g_assert_cmpuint(a_length, ==, b_length);
    assert_riff_equivalent_possibly_newer(a_contents, b_contents, a_length);
    g_free(a_contents);
    g_free(b_contents);
}

static void test_patch(
        hunk const * const h, char const * const temp_dir,
        char const * const a, char const * const b) {
    char * patch_outfile = g_build_filename(temp_dir, "patch_result", NULL);
    g_assert_cmpint(APATCH_OK, ==, apatch(h, a, b, patch_outfile));
    diff d = adiff(b, patch_outfile);
    g_assert_cmpint(d.code, ==, ADIFF_OK);
    g_assert_null(d.hunks);
    diff_free(&d);
    files_identical(b, patch_outfile);
    remove(patch_outfile);
    g_free(patch_outfile);
}

static void positive_test(
        char const * const a, char const * const b,
        adiff_fixture const * const f) {
    diff d = adiff(a, b);
    diff_assertions(&d, &f->fcd0, &f->fcd1);
    test_patch(d.hunks, f->temp_dir, a, b);
    diff_free(&d);
}

#define pos_test(type) \
    static void test_##type(gconstpointer ud) { \
        adiff_fixture const * const f = ud; \
        positive_test(f->type##0, f->type##1, f); \
    }

pos_test(short)
pos_test(int)
pos_test(float)
pos_test(double)

#undef pos_test

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    adiff_fixture fixture = create_fixture();
    g_test_add_data_func("/adiff/missing", &fixture, test_adiff_files_missing);
    g_test_add_data_func(
        "/adiff/sample_rate_mismatch", &fixture,
        test_adiff_sample_rate_mismatch);
    g_test_add_data_func(
        "/adiff/channels_mismatch", &fixture, test_adiff_channels_mismatch);
    g_test_add_data_func(
        "/adiff/sample_format_mismatch", &fixture,
        test_adiff_sample_format_mismatch);
    g_test_add_data_func(
        "/adiff/short", &fixture, test_short);
    g_test_add_data_func(
        "/adiff/int", &fixture, test_int);
    g_test_add_data_func(
        "/adiff/float", &fixture, test_float);
    g_test_add_data_func(
        "/adiff/double", &fixture, test_double);
    g_test_add_data_func(
        "/apatch/open_errors", &fixture, test_apatch_file_open_errors);
    int const run_result = g_test_run();
    cleanup_fixture(fixture);
    return run_result;
}
