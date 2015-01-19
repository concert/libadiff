#include "../include/adiff.h"
#include "../include/bdiff.h"
#include <sndfile.h>

typedef struct {
    SNDFILE * const file;
    SF_INFO const info;
} lsf_wrapped;

typedef char const * const const_str;

static lsf_wrapped sndfile_open(const_str path) {
    SF_INFO info = {};
    return (lsf_wrapped) {
        .file = sf_open(path, SFM_READ, &info),
        .info = info
    };
}

static lsf_wrapped sndfile_new(const_str path, SF_INFO info) {
    return (lsf_wrapped) {
        .file = sf_open(path, SFM_WRITE, &info),
        .info = info
    };
}

static adiff_return_code info_cmp(const lsf_wrapped a, const lsf_wrapped b) {
    if (a.info.channels != b.info.channels)
        return ADIFF_ERR_CHANNELS;
    if (a.info.samplerate != b.info.samplerate)
        return ADIFF_ERR_SAMPLE_RATE;
    if (
            (a.info.format & SF_FORMAT_SUBMASK) !=
            (b.info.format & SF_FORMAT_SUBMASK)) {
        return ADIFF_ERR_SAMPLE_FORMAT;
    }
    return ADIFF_OK;
}

static unsigned short_fetcher(void * source, unsigned n_items, char * buffer) {
    return sf_readf_short((SNDFILE * const) source, (short *) buffer, n_items);
}

static unsigned float_fetcher(void * source, unsigned n_items, char * buffer) {
    return sf_readf_float((SNDFILE * const) source, (float *) buffer, n_items);
}

typedef struct {
    data_fetcher const fetcher;
    const size_t sample_size;
} fetcher_info;

static fetcher_info get_fetcher(lsf_wrapped const f) {
    if (
            ((f.info.format & SF_FORMAT_SUBMASK) == SF_FORMAT_FLOAT) |
            ((f.info.format & SF_FORMAT_SUBMASK) == SF_FORMAT_DOUBLE)) {
        return (fetcher_info) {
            .fetcher = float_fetcher, .sample_size = sizeof(float)};
    } else {
        return (fetcher_info) {
            .fetcher = short_fetcher, .sample_size = sizeof(short)};
    }
}

static diff cmp(const lsf_wrapped a, const lsf_wrapped b) {
    adiff_return_code ret_code = info_cmp(a, b);
    if (ret_code == ADIFF_OK) {
        fetcher_info const fi = get_fetcher(a);
        return (diff) {
            .code = ret_code,
            .hunks = bdiff(
                fi.sample_size * a.info.channels, fi.fetcher,
                (void *) a.file, (void *) b.file)};
    }
    return (diff) {.code = ret_code};
}

diff adiff(const_str path_a, const_str path_b) {
    lsf_wrapped const a = sndfile_open(path_a);
    if (a.file == NULL) {
        return (diff) {.code = ADIFF_ERR_OPEN_A};
    }
    lsf_wrapped const b = sndfile_open(path_b);
    if (b.file == NULL) {
        sf_close(a.file);
        return (diff) {.code = ADIFF_ERR_OPEN_B};
    }
    diff result = cmp(a, b);
    sf_close(a.file);
    sf_close(b.file);
    return result;
}

static unsigned short_writer(
        SNDFILE * const src, unsigned const n_items, char const * buffer) {
    return sf_writef_short(src, (short *) buffer, n_items);
}

static unsigned float_writer(
        SNDFILE * const src, unsigned const n_items,
        char const * buffer) {
    return sf_writef_float(src, (float *) buffer, n_items);
}

typedef unsigned (*data_writer)(
    SNDFILE * const, unsigned const n_items, char const * buffer);

typedef struct {
    data_writer const writer;
    size_t const sample_size;
} writer_info;

static writer_info get_writer(lsf_wrapped const f) {
    if (
            ((f.info.format & SF_FORMAT_SUBMASK) == SF_FORMAT_FLOAT) |
            ((f.info.format & SF_FORMAT_SUBMASK) == SF_FORMAT_DOUBLE)) {
        return (writer_info) {
            .writer = float_writer, .sample_size = sizeof(float)};
    } else {
        return (writer_info) {
            .writer = short_writer, .sample_size = sizeof(short)};
    }
}

static void copy_data(
        lsf_wrapped const in, lsf_wrapped const out, unsigned start,
        unsigned end) {
    if (end == 0) {
        return;
    }
    end--;
    sf_seek(in.file, start, SEEK_SET);  // Should be error checked (-1 rval)
    fetcher_info const fi = get_fetcher(in);
    writer_info const ei = get_writer(in);
    char buffer[8192];
    unsigned n_items = 8192 / ei.sample_size / in.info.channels;
    while (start < end) {
        if ((end - start) < n_items)
            n_items = (end - start) + 1;
        const unsigned n_read = fi.fetcher(in.file, n_items, buffer);
        ei.writer(out.file, n_read, buffer);  // Possible write failure
        start += n_read;
    }
}

static apatch_return_code apply_patch(
        hunk const * h, lsf_wrapped const a, lsf_wrapped const b,
        lsf_wrapped const o) {
    unsigned prev_hunk_end = 0;
    for (; h != NULL; h = h->next) {
        copy_data(a, o, prev_hunk_end, h->a.start);
        copy_data(b, o, h->b.start, h->b.end);
        prev_hunk_end = h->a.end;
    }
    copy_data(a, o, prev_hunk_end, a.info.frames + 1);
    return APATCH_OK;
}

apatch_return_code apatch(
        hunk const * hunks, const_str path_a, const_str path_b,
        const_str out_path) {
    apatch_return_code retcode;
    lsf_wrapped const a = sndfile_open(path_a);
    if (a.file != NULL) {
        lsf_wrapped const b = sndfile_open(path_b);
        if (b.file != NULL) {
            lsf_wrapped const o = sndfile_new(out_path, a.info);
            if (o.file != NULL) {
                retcode = apply_patch(hunks, a, b, o);
                sf_close(o.file);
            } else {
                retcode = APATCH_ERR_OPEN_OUTPUT;
            }
            sf_close(b.file);
        } else {
            retcode = APATCH_ERR_OPEN_B;
        }
        sf_close(a.file);
    } else {
        retcode = APATCH_ERR_OPEN_A;
    }
    return retcode;
}

void diff_free(diff * d) {
    hunk_free(d->hunks);
}
