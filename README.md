sample - filter for random sampling of input.

Basic usage:

    sample [-h] [-d files] [-n count] [-p percent] [-s seed] [FILE ...]

Examples:

    sample FILE                 # randomly choose & print 4 lines from file, in order
    input | sample              # same, from stream (file defaults to stdin)
    sample FILE FILE2 FILE3     # randomly choose 4 lines between multiple input files
    sample -n 10 FILE           # choose 10 lines
    sample -p 10 FILE           # 10% chance of choosing each line
    input | sample -p 5         # randomly print 5% of input lines
    input | sample -d a,b,c     # append input to files a, b, and c, even odds
    input | sample -d a,b,c,    # append input to files a, b, c, or /dev/null

Options:

    -h       - Print help
    -d FILES - randomly deal lines to multiple files (',' separated)
    -n COUNT - Set sample count (default: -n 4)
    -p PERC  - Sample PERC percent for input(s) (',' separated)
    -s SEED  - Set a specific random seed (default: seed based on time)


Sampling by count (-n) works in O(n) space (but n=number of samples, not
input size) and outputs samples when end of input is reached. The input order
of the samples is preserved.

Sampling by percentage (-p) works in constant space (no data is accumulated)
and outputs as each group of lines is read.

If multiple files are used, they can have custom probability percentages:

    input | sample -d a,b,c,d -p 0.5,0.25,0.125,0.125

If the last probability is omitted, it will take all remaining:

    input | sample -d a,b,c,d -p 0.5,0.25,0.125,

If a file is omitted, it will behave like /dev/null:

    input | sample -d a,b,c, -p 0.25,0.25,0.25,0.25

If probabilities are between 1 and 100, they will be treated as percentages:

    input | sample -d a,b,c,d -p 25,25,25,25


