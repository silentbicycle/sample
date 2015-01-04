#ifndef SAMPLE_H
#define SAMPLE_H

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <assert.h>
#include <stdbool.h>

#include <errno.h>
#include <sys/time.h>

typedef enum {
    M_UNDEF,                    /* undefined */
    M_COUNT,                    /* get N samples */
    M_PERC,                     /* get N% of input */
} filter_mode;

enum {
    SAMPLE_DEF_BUF_SIZE = 2,
};

typedef struct {
    unsigned int seed;

    /* Input stream(s) */
    FILE *cur_file;
    const char **fnames;
    size_t fn_count;
    size_t fn_index;

    filter_mode mode;
    union {
        struct {
            size_t samples;
        } count;
        struct {
            size_t pair_count;
            struct out_pair *pairs;
        } percent;
    } u;

    size_t buf_size;
    char *buf;
} config;

struct out_pair {
    double perc;                /* cumulative probability, <= 1.0 */
    FILE *out;
};

/* Get the next line from the input source(s), or NULL on EOS. */
typedef const char *(line_iter_cb)(config *cfg, size_t *length);

int sample_count(config *cfg, line_iter_cb *line_iter);
int sample_percent(config *cfg, line_iter_cb *line_iter);

#endif
