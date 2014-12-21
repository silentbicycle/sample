/* 
 * Copyright (c) 2011-14 Scott Vokes <vokes.s@gmail.com>
 *  
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "sample.h"

typedef struct {
    char *buf;
    size_t buf_sz;
    size_t length;
    size_t line_number;
} sample_info;

static int si_cmp(const void *a, const void *b);
static bool keep(sample_info *kept, const char *line, size_t len, size_t id);

#define DEF_BUF_SIZE 8          /* Start small to exercise buffer growing */

/* Randomly sample N values from a stream of unknown length, with
 * unknown probability. Knuth 3.4.2, algorithm R (Reservoir sampling). */
int sample_count(config *cfg, line_iter_cb *line_iter) {
    size_t samples = cfg->u.count.samples;
    sample_info *kept = calloc(samples, sizeof(*kept));
    if (kept == NULL) { err(1, "calloc"); }

    size_t i = 0;
    for (i = 0; i < samples; i++) {
        sample_info *si = &kept[i];
        si->buf = malloc(DEF_BUF_SIZE);
        if (si->buf == NULL) { err(1, "malloc"); }
        si->buf_sz = DEF_BUF_SIZE;
    }

    const char *line = NULL;
    size_t len = 0;

    /* Fill buffer with SAMPLES, then randomly replace them
     * with others with a samples/i chance. */

    for (i = 0; i < samples; i++) {
        line = line_iter(cfg, &len);
        if (line == NULL) { break; }
        keep(&kept[i], line, len, i);
    }

    line = line_iter(cfg, &len);

    while (line) {
        long rv = (random() % i);
        if (rv < samples) {
            keep(&kept[rv], line, len, i);
        }
        i++;
        line = line_iter(cfg, &len);
    }

    /* Output samples, in input order. */
    qsort(kept, samples, sizeof(sample_info), si_cmp);
    for (i = 0; i < samples; i++) {
        if (kept[i].length > 0) { printf("%s\n", kept[i].buf); }
    }

    /* free kept and buffers */
    for (i = 0; i < samples; i++) {
        free(kept[i].buf);
    }
    free(kept);

    return 0;
}

static int si_cmp(const void *a, const void *b) {
    size_t ia = ((sample_info *)a)->line_number;
    size_t ib = ((sample_info *)b)->line_number;
    return ia < ib ? -1 : ia > ib ? 1 : 0;
}

static bool keep(sample_info *si, const char *line, size_t len, size_t id) {
    if (si->buf_sz < len + 1) {
        size_t nsize = 2 * si->buf_sz;
        while (nsize < len + 1) { nsize *= 2; }
        char *nbuf = realloc(si->buf, nsize);
        if (nbuf) {
            si->buf_sz = nsize;
            si->buf = nbuf;
        } else {
            return false;
        }
    }

    memcpy(si->buf, line, len);
    si->buf[len] = '\0';
    si->length = len;
    si->line_number = id;

    return true;
}

/* Just print each line, with (percent)% probability. */
int sample_percent(config *cfg, line_iter_cb *line_iter) {
    struct out_pair *pairs = cfg->u.percent.pairs;
    size_t pair_count = cfg->u.percent.pair_count;

    const char *line = line_iter(cfg, NULL);
    while (line) {
        double rv = (random() % RAND_MAX)/(double)RAND_MAX;
        for (int i = 0; i < pair_count; i++) {
            if (rv < pairs[i].perc) {
                if (pairs[i].out) {
                    fprintf(pairs[i].out, "%s\n", line);
                }
                break;
            }
        }

        line = line_iter(cfg, NULL);
    }
    return 0;
}
