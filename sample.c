/* 
 * Copyright (c) 2011 Scott Vokes <vokes.s@gmail.com>
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/time.h>

typedef unsigned long I;

#if defined(__linux__)
#define HAS_GETLINE
#elif defined(BSD)
#define HAS_FGETLN
#endif

/* Settings */
static char mode = 's';         /* 's' = shuffle, 'p' = percentage */
static I samples = 4;           /* N random lines from all input */
static I percent = 10;          /* each line has N% chance of sampling */
static FILE *cur_file = NULL;
static char **fnames;
static I fn_count = 0;
static I fn_index = 0;

typedef struct sample_info {
        char *line;
        I ct;                   /* line number */
} sample_info;

static void usage() {
        fprintf(stderr, "Usage: sample [-h] [-n count] [-p percent] [-s seed] [FILE ...]\n");
        exit(1);
}

static void *alloc(I sz) {
        void *p = malloc(sz);
        if (p == NULL) err(1, "malloc fail");
        return p;
}

/* Iterate over input files. "-" is stdin. */
static int next_file() {
        char *fn;
        FILE *f;
        if (cur_file && cur_file != stdin &&
            fn_index < fn_count && fnames[fn_index] != NULL) {
                if (fclose(cur_file) != 0) err(1, "%s", fnames[fn_index]);
                cur_file = NULL;
        }
        while (fn_index < fn_count) {
                fn = fnames[fn_index];
                f = (strcmp(fn, "-")==0 ? stdin : fopen(fn, "r"));
                fn_index++;
                cur_file = f;
                if (f == NULL) { /* warn about unreadable file and skip */
                        warn("%s", fn);
                        errno = 0;
                        return next_file();
                }
                return 1;
        }
        return 0;
}

/* Get next input line, switching to new files as necessary. */
static char *next_line() {
        if (cur_file == NULL && next_file() == 0) return NULL;
#if defined(HAS_GETLINE)
        static char buf[4096];
        char *line = NULL;
        size_t sz = 4096;
        ssize_t res = getline(&line, &sz, cur_file);
        if (res < 1) {
                if (next_file() == 0) return NULL;
                return next_line();
        }
        strncpy(buf, line, 4096);
        buf[res-1] = '\0';
        free(line);
        return buf;
#elif defined(HAS_FGETLN)
        size_t len;
        char *n = fgetln(cur_file, &len);
        if (n == NULL) {
                if (next_file() == 0) return NULL;
                return next_line();
        }
        if (n[len-1] == '\n') n[len-1] = '\0';
        return n;
#else
#error Must have getline or fgetln
#endif
}

static void keep(sample_info *kept, char *line, I line_ct, I i) {
        I len = strlen(line);
        char *p = alloc(sizeof(char)*len + 1);
        strncpy(p, line, len);
        p[len > 0 ? len : 0] = '\0';
        if (kept[i].line != NULL) free(kept[i].line);
        kept[i].ct = line_ct; /* so lines can be output in orig. order */
        kept[i].line = p;
}

static int si_cmp(const void *a, const void *b) {
        I ia = ((sample_info *)a)->ct;
        I ib = ((sample_info *)b)->ct;
        return ia < ib ? -1 : ia > ib ? 1 : 0;
}

/* Randomly sample N values from a stream of unknown length, with
 * unknown probability. Knuth 3.4.2, algorithm R. */
static void sample_count(I samples) {
        I i=0;
        double rv;
        sample_info *kept = alloc(samples*sizeof(sample_info));
        memset(kept, 0, samples*sizeof(sample_info *));

        for (i=0; i<samples; i++) {       /* Initial samples */
                char *line = next_line();
                if (line == NULL) break;
                keep(kept, line, i, i);
        }

        while (1) {                       /* Randomly replace current samples */
                char *line = next_line();
                if (line == NULL) break;
                i++;
                rv = (random() % RAND_MAX)/(double)RAND_MAX;
                if (rv <= samples/(1.0*i))
                        keep(kept, line, i, random() % samples);
        }

        /* Output samples, in original input order. */
        qsort(kept, samples, sizeof(sample_info), si_cmp);
        for (i=0; i<samples; i++) {
                if (kept[i].line != NULL) printf("%s\n", kept[i].line);
        }
}

/* Just print each line, with (percent)% probability. */
static void sample_perc() {
        double perc = percent*.01;
        double rv;
        while (1) {
                char *line = next_line();
                if (line == NULL) return;
                rv = (random() % RAND_MAX)/(double)RAND_MAX;
                if (rv <= perc) printf("%s\n", line);
        }
}

static void handle_args(int *argc, char ***argv) {
        int fl, i;
        while ((fl = getopt(*argc, *argv, "hn:p:s:")) != -1) {
                switch (fl) {
                case 'h':       /* help */
                        usage();
                        break;  /* NOTREACHED */
                case 'n':       /* number of samples */
                        samples = atol(optarg);
                        mode = 's';
                        if (samples < 1) {
                                fprintf(stderr, "Bad sample count.\n");
                                exit(1);
                        }
                        break;
                case 'p':       /* percent */
                        mode = 'p';
                        percent = atoi(optarg);
                        if (percent < 0 || percent > 100) {
                                fprintf(stderr, "Bad percentage.\n");
                                exit(1);
                        }
                        break;
                case 's':       /* seed */
                        srandom(atol(optarg));
                        break;
                default:
                        usage();
                        /* NOTREACHED */
                }
        }
        *argc -= (optind-1);
        *argv += (optind-1);

        fn_count = (*argc - 1);
        if (fn_count == 0) fn_count = 1;

        fnames = alloc(fn_count*sizeof(char *));
        fnames[0] = "-";        /* default to stdin */
        for (i=1; i<*argc; i++) {
                fnames[i-1] = (*argv)[i];
        }
}

static void seed_random() {
        struct timeval tv;
        if (gettimeofday(&tv, NULL) != 0) err(1, "gettimeofday");
        srandom(tv.tv_usec);
}

int main(int argc, char **argv) {
        seed_random();
        handle_args(&argc, &argv);
        next_file();

        if (mode == 's')
                sample_count(samples);
        else if (mode == 'p')
                sample_perc();
        else
                usage();
        return 0;
}
