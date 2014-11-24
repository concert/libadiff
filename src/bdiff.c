#include "../include/bdiff.h"
#include "../include/rabin.h"
#include "chunk.h"
#include "block.h"
#include "diff_hunk.h"
#include <stdio.h>

static const unsigned buf_size = 4096;

static chunks const split_data(data_fetcher const df, void * const a) {
    char buf[buf_size];
    unsigned n_read;
    chunks head, tail;
    head = tail = NULL;
    unsigned start_pos = 0, total_read = 0;
    hash_data hd = hash_data_init();
    window_data wd = window_data_init();
    unsigned chunk_hash;
    do {
        n_read = df(a, buf_size, buf);
        for (unsigned i = 0; i < n_read; i++) {
            chunk_hash = hash_data_update(&hd, buf[i]);
            if (!(window_data_update(&wd, buf[i]) & 0xFF)) {
                tail = chunk_new(tail, start_pos, total_read, chunk_hash);
                start_pos = total_read;
                if (head == NULL) {
                    head = tail;
                }
                hash_data_reset(&hd);
                window_data_reset(&wd);
            }
            total_read++;
        }
    } while (n_read);
    if (total_read > start_pos) {
        tail = chunk_new(tail, start_pos, total_read, chunk_hash);
        if (head == NULL) {
            head = tail;
        }
    }
    return head;
}

diff_hunk * const bdiff(data_fetcher const df, void * const a, void * const b) {
    chunks a_chunks = split_data(df, a);
    chunks b_chunks = split_data(df, b);
    blocks unique_a = unique_blocks(a_chunks, b_chunks);
    blocks unique_b = unique_blocks(b_chunks, a_chunks);
    chunk_free(a_chunks);
    chunk_free(b_chunks);
    diff_hunk * const dh = pair_blocks(unique_a, unique_b);
    chunk_free(unique_a);
    chunk_free(unique_b);
    return dh;
}

typedef struct {
    char * data;
    unsigned length;
    unsigned pos;
} test_data;

unsigned test_reader(void * source, unsigned n_items, char * buffer) {
    test_data * td = source;
    unsigned stop = td->pos + n_items;
    if (stop > td->length) {
        stop = td->length;
    }
    stop -= td->pos;
    for (unsigned i=0; i < stop; i++) {
        buffer[i] = td->data[i + td->pos];
    }
    td->pos += stop;
    return stop;
}

int main() {
    test_data a = {.data = "foobarbaz", .length = 9};
    test_data b = {.data = "foobarbam", .length = 9};
    diff_hunk * const dh = bdiff(test_reader, (void *) &a, (void *) &b);
    printf("bs: %u be: %u\n", dh->b.start, dh->b.end);
    diff_hunk_free(dh);
}
