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

/* Version 0.1.0 => 0.2.0: +DEAL */
#define SAMPLE_VERSION_MAJOR 0
#define SAMPLE_VERSION_MINOR 1
#define SAMPLE_VERSION_PATCH 0
#define SAMPLE_AUTHOR "Scott Vokes <vokes.s@gmail.com>"

static void cleanup(config *cfg);
static void parse_percent_settings(config *cfg,
    char *deal_arg, char *perc_arg);

static void usage(const char *msg) {
    if (msg) { fprintf(stderr, "%s\n\n", msg); }
    fprintf(stderr, "sample v. %d.%d.%d by %s\n",
        SAMPLE_VERSION_MAJOR, SAMPLE_VERSION_MINOR,
        SAMPLE_VERSION_PATCH, SAMPLE_AUTHOR);
    fprintf(stderr,
        "Usage: sample [-h] [-d files] [-n count] [-p percent] [-s seed] [FILE ...]\n"
        "    -h       - Print help\n"
        "    -d FILES - randomly deal lines to multiple files (',' separated)\n"
        "    -n COUNT - Set sample count (default: -n 4)\n"
        "    -p PERC  - Sample PERC percent for input(s) (',' separated)\n"
        "               If >= 1, argument is used as -p 5 => 0.05 (5%%).\n"
        "    -s SEED  - Set a specific random seed (default: seed based on time)\n"
        );
    exit(1);
}

static void set_mode(config *cfg, filter_mode m) {
    if (cfg->mode == m) { return; }
    if (cfg->mode != M_UNDEF) { usage("Error: Mix of percent and count modes."); }
    cfg->mode = m;
}

static void handle_args(config *cfg, int argc, char **argv) {
    char *deal_arg = NULL;
    char *perc_arg = NULL;

    int fl, i;
    while ((fl = getopt(argc, argv, "hd:n:p:s:")) != -1) {
        switch (fl) {
        case 'h':               /* help */
            usage(NULL);
            break;              /* NOTREACHED */
        case 'd':               /* deal out to N files */
            set_mode(cfg, M_PERC);
            if (deal_arg) { usage("Error: Multiple -d arguments."); }
            deal_arg = optarg;
            break;
        case 'n':               /* number of samples */
            set_mode(cfg, M_COUNT);
            cfg->u.count.samples = atol(optarg);
            if (cfg->u.count.samples < 1) {
                fprintf(stderr, "Bad sample count.\n");
                exit(1);
            }
            break;
        case 'p':               /* percent(s) */
            set_mode(cfg, M_PERC);
            perc_arg = optarg;
            break;
        case 's':               /* seed */
            cfg->seed = atol(optarg);
            break;
        case '?':
        default:
            usage(NULL);
            /* NOTREACHED */
        }
    }

    if (cfg->mode == M_PERC) {
        parse_percent_settings(cfg, deal_arg, perc_arg);
    } else {                    /* default to sample count */
        cfg->mode = M_COUNT;
    }

    argc -= (optind-1);
    argv += (optind-1);
    
    cfg->fn_count = argc - 1;
    if (cfg->fn_count == 0) {
        cfg->fn_count = 1;
    }
    
    cfg->fnames = calloc(cfg->fn_count, sizeof(cfg->fnames[0]));
    if (cfg->fnames == NULL) { err(1, "calloc"); }

    cfg->fnames[0] = "-";        /* default to stdin */
    for (i = 1; i < argc; i++) {
        cfg->fnames[i - 1] = argv[i];
    }
}

static void parse_percent_settings(config *cfg,
        char *deal_arg, char *perc_arg) {
    const int def_pairs = 1;
    struct out_pair *pairs = calloc(def_pairs, sizeof(*pairs));
    if (pairs == NULL) { err(1, "calloc"); }

    assert(deal_arg != NULL || perc_arg != NULL);
    if (deal_arg) {
        /* Multiple output files: */
        int pair_count = 0;
        int pair_max = def_pairs;

        /* Open files by name, split by ',' */
        for (char *fn = strsep(&deal_arg, ",");
             fn; fn = strsep(&deal_arg, ",")) {
            FILE *f = NULL;
            if (fn[0] == '\0') {                /* /dev/null */
            } else if (0 == strcmp("-", fn)) {  /* stdout */
                f = stdout;
            } else {                            /* file */
                f = fopen(fn, "ab");
                if (f == NULL) { err(1, "fopen"); }
            }
            pairs[pair_count].out = f;
            pair_count++;
            if (pair_count == pair_max) {  /* grow array */
                int nmax = 2 * pair_max;
                struct out_pair *npairs = realloc(pairs, nmax * sizeof(*npairs));
                if (npairs == NULL) { err(1, "realloc"); }
                pair_max = nmax;
                pairs = npairs;
            }
        }

        /* Deal uneven percentages to the files */
        if (perc_arg) {
            double total = 0;
            int perc_count = 0;
            for (char *perc = strsep(&perc_arg, ",");
                 perc; perc = strsep(&perc_arg, ",")) {
                double v;
                if (perc[0] == '\0') {   /* trailing "," -> remaining */
                    v = 1.0 - total;
                } else {
                    v = strtod(perc, NULL);
                }
                if (v < 0 || v > 100) { usage("Error: Bad percentage."); }
                if (v > 1.0 && v <= 100.0) { v /= 100.0; }
                total += v;
                pairs[perc_count].perc = total;
                perc_count++;
            }

            if (perc_count == 1) {
                for (int i = 1; i < pair_count; i++) {
                    total += pairs[0].perc;
                    pairs[i].perc = total;  /* project */
                }
            } else if (perc_count < pair_count) {
                usage("Error: Percent count does not match output count");
            }
            if (total > 1.0) { usage("Error: Total is over 100%"); }
        } else {                /* divide evenly */
            double total = 0;
            for (int i = 0; i < pair_count; i++) {
                total += 1.0 / pair_count;
                pairs[i].perc = total;
            }
        }

        cfg->u.percent.pair_count = pair_count;
        cfg->u.percent.pairs = pairs;
    } else if (perc_arg) {
        /* -p % to stdout */
        double v = strtod(perc_arg, NULL);
        if (v < 0 || v > 100.0) { usage("Error: Bad percentage."); }
        if (v > 1.0 && v <= 100.0) { v /= 100.0; }
        pairs[0].perc = v;
        pairs[0].out = stdout;
        cfg->u.percent.pair_count = 1;
        cfg->u.percent.pairs = pairs;
    }
}

static void set_defaults(config *cfg) {
    cfg->mode = M_UNDEF;
    cfg->u.count.samples = 4;

    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) { err(1, "gettimeofday"); }
    cfg->seed = (unsigned int)tv.tv_usec;
}

static bool open_next_file(config *cfg) {
    while (cfg->fn_index < cfg->fn_count) {
        const char *fn = cfg->fnames[cfg->fn_index];
         cfg->fn_index++;
        if (0 == strcmp("-", fn)) {
            cfg->cur_file = stdin;
            return true;
        } else {
            FILE *f = fopen(fn, "r");
            if (f) {
                cfg->cur_file = f;
                return true;
            } else {
                warn("%s", fn);
                errno = 0;
            }
        }
    }
    return false;
}

static const char *line_iter(config *cfg, size_t *length) {
    for (;;) {
        if (cfg->cur_file == NULL) {
            if (!open_next_file(cfg)) { return NULL; }
        }
        
        size_t len = 0;
        char *line = fgetln(cfg->cur_file, &len);
        if (line) {
            if (line[len - 1] == '\n') { line[len - 1] = '\0'; }
            if (length) { *length = len; }
            return line;
        } else {
            fclose(cfg->cur_file);
            cfg->cur_file = NULL;
        }
    }
}

static void cleanup(config *cfg) {
    free(cfg->fnames);
    if (cfg->mode == M_PERC) {
        struct out_pair *pairs = cfg->u.percent.pairs;
        for (int i = 0; i < cfg->u.percent.pair_count; i++) {
            if (pairs[i].out) { fclose(pairs[i].out); }
        }
        free(pairs);
    }
}

int main(int argc, char **argv) {
    config cfg;
    memset(&cfg, 0, sizeof(cfg));
    set_defaults(&cfg);
    handle_args(&cfg, argc, argv);

    srandom(cfg.seed);

    int res = 0;

    switch (cfg.mode) {
    case M_COUNT:
        res = sample_count(&cfg, line_iter);
        break;
    case M_PERC:
        res = sample_percent(&cfg, line_iter);
        break;
    default:
        assert(0);
    }

    cleanup(&cfg);
    return res;
}
