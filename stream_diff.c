#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <sndfile.h>

typedef struct {
    SNDFILE * const file;
    SF_INFO const info;
} lsf_wrapped;

typedef char const * const const_str;

lsf_wrapped sndfile_open(const_str path) {
    SF_INFO info = {};
    return (lsf_wrapped) {
        .file = sf_open(path, SFM_READ, &info),
        .info = info
    };
}

const_str info_cmp(const lsf_wrapped a, const lsf_wrapped b) {
    if (a.info.channels != b.info.channels)
        return "Channels differ";
    if (a.info.samplerate != b.info.samplerate)
        return "Sample rates differ";
    if (a.info.frames != b.info.frames)
        return "Lengths differ";
    if (
            (a.info.format & SF_FORMAT_SUBMASK) !=
            (b.info.format & SF_FORMAT_SUBMASK)) {
        return "Sample formats differ";
    }
    return NULL;
}

const_str data_cmp(const lsf_wrapped a, const lsf_wrapped b) {
    #define buffer_size 65536 // Size of comparison buffers (in samples)
    double buffer_a[buffer_size], buffer_b[buffer_size];
    // Calculate the framecount that will fit in our data buffers
    sf_count_t const n_buffer_frames = buffer_size / a.info.channels;
    #undef buffer_size

    sf_count_t read_a, read_b, offset = 0;
    while ((read_a = sf_readf_double(a.file, buffer_a, n_buffer_frames)) > 0) {
        read_b = sf_readf_double(b.file, buffer_b, read_a);
        for (sf_count_t i = 0; i < read_a * a.info.channels; i++) {
            if (buffer_a[i] != buffer_b[i]) {
                printf(
                    "Files differ after %f seconds.\n",
                    ((offset + (i / a.info.channels))/(float) a.info.samplerate));
                return "Contents differ";
            }
        }
        offset += read_a;
    }
    return NULL;
}

const_str cmp(const lsf_wrapped a, const lsf_wrapped b) {
    char const * result = info_cmp(a, b);
    if (result == NULL)
        result = data_cmp(a, b);
    return result;
}

const_str differences(const_str path_a, const_str path_b) {
    lsf_wrapped const a = sndfile_open(path_a);
    if (a.file == NULL)
        return "Failed to open a";
    lsf_wrapped const b = sndfile_open(path_b);
    if (a.file == NULL)
        return "Failed to open b";
    const_str result = cmp(a, b);
    sf_close(a.file);
    sf_close(b.file);
    return result;
}

int main (const int argc, const_str argv[]) {
    if (argc != 3) {
        printf("Args are 2 files to compare.\n");
        return 3;
    }

    const_str a_path = argv[1], b_path = argv[2];

    if (strcmp(a_path, b_path) == 0) {
        printf("Comparing file to itself.\n");
        return 2;
    }

    const_str result = differences(a_path, b_path);

    if (result == NULL) {
        printf("Streams identical\n");
        return 0;
    } else {
        printf("Streams differed: %s\n", result);
        return 1;
    }
}
