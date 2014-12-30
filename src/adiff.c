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

static unsigned int_fetcher(void * source, unsigned n_items, char * buffer) {
    lsf_wrapped const * const src = source;
    return sf_readf_double(src->file, (double *) buffer, n_items);
}

static unsigned float_fetcher(void * source, unsigned n_items, char * buffer) {
    lsf_wrapped const * const src = source;
    return sf_readf_float(src->file, (float *) buffer, n_items);
}

static diff cmp(const lsf_wrapped a, const lsf_wrapped b) {
    adiff_return_code ret_code = info_cmp(a, b);
    if (ret_code == ADIFF_OK) {
        if (
                ((a.info.format & SF_FORMAT_SUBMASK) == SF_FORMAT_FLOAT) |
                ((a.info.format & SF_FORMAT_SUBMASK) == SF_FORMAT_DOUBLE)) {
            return (diff) {
                .code = ret_code,
                .hunks = bdiff(
                    sizeof(float) * a.info.channels, float_fetcher,
                    (void *) &a, (void *) &b)};
        } else {
            return (diff) {
                .code = ret_code,
                .hunks = bdiff(
                    sizeof(short) * a.info.channels, int_fetcher,
                    (void *) &a, (void *) &b)};
        }
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

static void copy_data(
        lsf_wrapped const in, lsf_wrapped const out, unsigned start,
        unsigned end) {
    end--;
    sf_seek(in.file, start, SEEK_SET);  // Should be error checked (-1 rval)
    short buffer[4096];
    unsigned n_items = 4096 / in.info.channels;
    while (start < end) {
        if ((end - start) < n_items)
            n_items = (end - start) + 1;
        const unsigned n_read = sf_readf_short(in.file, buffer, n_items);
        sf_writef_short(out.file, buffer, n_read);  // Possible write failure
        start += n_read;
    }
}

static apatch_return_code apply_patch(
        hunk * h, lsf_wrapped const a, lsf_wrapped const b,
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
        hunk * hunks, const_str path_a, const_str path_b, const_str out_path) {
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
