sample - filter for random sampling of input.

Basic usage:

    sample [-h] [-n count] [-p percent] [-s seed] [FILE ...]

Examples:

    sample FILE                 # randomly choose & print 4 lines from file, in order
    input_stream | sample       # same, from stream (file defaults to stdin)
    sample FILE FILE2 FILE3     # randomly choose 4 lines between multiple input files
    sample -n 10 FILE           # choose 10 lines
    sample -p 10 FILE           # 10% chance of choosing each line
    input_stream | sample -p 5  # randomly print 5% of input lines

Options:

    -h       - Print help
    -n COUNT - Set sample count (default: -n 4)
    -p PERC  - Sample PERC percent of input
    -s SEED  - Set a specific seed (default: seed based on time)


Sampling by count (-n) works in O(n) space (but n=number of samples, not
input size) and outputs samples when end of input is reached. The input order
of the samples is preserved.

Sampling by percentage (-p) works in constant space (no data is accumulated)
and outputs as each group of lines is read.
