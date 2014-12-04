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

static unsigned fetcher(void * source, unsigned n_items, char * buffer) {
    lsf_wrapped const * const src = source;
    return sf_readf_double(
        src->file, (double *) buffer,
        n_items / (sizeof(double) * src->info.channels));
}

static diff cmp(const lsf_wrapped a, const lsf_wrapped b) {
    adiff_return_code ret_code = info_cmp(a, b);
    if (ret_code == ADIFF_OK)
        return (diff) {
            .code = ret_code,
            .hunks = bdiff(
                sizeof(double) * a.info.channels, fetcher,
                (void *) &a, (void *) &b)};
    return (diff) {.code = ret_code};
}

diff adiff(const_str path_a, const_str path_b) {
    lsf_wrapped const a = sndfile_open(path_a);
    if (a.file == NULL)
        return (diff) {.code = ADIFF_ERR_OPEN_A};
    lsf_wrapped const b = sndfile_open(path_b);
    if (a.file == NULL)
        return (diff) {.code = ADIFF_ERR_OPEN_B};
    diff result = cmp(a, b);
    sf_close(a.file);
    sf_close(b.file);
    return result;
}

void diff_free(diff * d) {
    hunk_free(d->hunks);
}
