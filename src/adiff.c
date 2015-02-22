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

static unsigned short_fetcher(void * source, char * buffer, unsigned n_items) {
  return sf_readf_short((SNDFILE * const) source, (short *) buffer, n_items);
}

static unsigned int_fetcher(void * source, char * buffer, unsigned n_items) {
  return sf_readf_int((SNDFILE * const) source, (int *) buffer, n_items);
}

static unsigned float_fetcher(void * source, char * buffer, unsigned n_items) {
  return sf_readf_float((SNDFILE * const) source, (float *) buffer, n_items);
}

static unsigned double_fetcher(void * source, char * buffer, unsigned n_items) {
  return sf_readf_double((SNDFILE * const) source, (double *) buffer, n_items);
}

typedef struct {
    data_fetcher const fetcher;
    const size_t sample_size;
} fetcher_info;

static fetcher_info get_fetcher(lsf_wrapped const f) {
    switch (f.info.format & SF_FORMAT_SUBMASK) {
        case SF_FORMAT_PCM_S8:
        case SF_FORMAT_PCM_U8:
        case SF_FORMAT_PCM_16:
            return (fetcher_info) {
	        .fetcher = short_fetcher, .sample_size = sizeof(short)};
        case SF_FORMAT_PCM_24:
        case SF_FORMAT_PCM_32:
            return (fetcher_info) {
	        .fetcher = int_fetcher, .sample_size = sizeof(int)};
        case SF_FORMAT_FLOAT:
            return (fetcher_info) {
                .fetcher = float_fetcher, .sample_size = sizeof(float)};
        case SF_FORMAT_DOUBLE:
        default:
            return (fetcher_info) {
                .fetcher = double_fetcher, .sample_size = sizeof(double)};
    }
}

static void seeker(void * const source, unsigned const pos) {
    // Should be error checked (-1 rval):
    sf_seek((SNDFILE * const) source, pos, SEEK_SET);
}

static diff cmp(const lsf_wrapped a, const lsf_wrapped b) {
    adiff_return_code ret_code = info_cmp(a, b);
    if (ret_code == ADIFF_OK) {
        fetcher_info const fi = get_fetcher(a);
        return (diff) {
            .code = ret_code,
            .hunks = bdiff(
                fi.sample_size * a.info.channels, seeker, fi.fetcher,
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

typedef unsigned (*data_writer)(
    SNDFILE * const, char const * buffer, unsigned const n_items);

typedef struct {
    data_writer const writer;
    size_t const sample_size;
} writer_info;

static writer_info get_writer(lsf_wrapped const f) {
    switch (f.info.format & SF_FORMAT_SUBMASK) {
        case SF_FORMAT_PCM_S8:
        case SF_FORMAT_PCM_U8:
        case SF_FORMAT_PCM_16:
            return (writer_info) {
                .writer = (data_writer) sf_writef_short,
                .sample_size = sizeof(short)};
        case SF_FORMAT_PCM_24:
        case SF_FORMAT_PCM_32:
            return (writer_info) {
                .writer = (data_writer) sf_writef_int,
                .sample_size = sizeof(int)};
        case SF_FORMAT_FLOAT:
            return (writer_info) {
                .writer = (data_writer) sf_writef_float,
                .sample_size = sizeof(float)};
        case SF_FORMAT_DOUBLE:
        default:
            return (writer_info) {
                .writer = (data_writer) sf_writef_double,
                .sample_size = sizeof(double)};
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
        const unsigned n_read = fi.fetcher(in.file, buffer, n_items);
        ei.writer(out.file, buffer, n_read);  // Possible write failure
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
