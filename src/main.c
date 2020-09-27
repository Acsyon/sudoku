#include <stdio.h>  /* FILE, fopen, flcose */
#include <stdlib.h> /* EXIT_SUCCESS, free */
#include <string.h> /* strcmp */

#include "sudoku.h"

int
main(int argc, char **argv)
{
    static const char *usage =
      "usage: sudoku [OPTIONS] FILE\n"
      "Read sudoku from FILE and write solution to file or standard output.\n"
      "\n"
      "With no FILE print help.\n"
      "\n"
      "  -o, --output  specify output file (or stdout, stderr)\n"
      "  -f, --fancy   print solution with separators between blocks\n"
      "  -h, --help    display this help and exit\n";

    int fancy = 0;
    char *infile = NULL;
    char *outfile = NULL;
    FILE *out = stdout;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            ++i;
            if (strcmp(argv[i], "stdout") == 0) {
                continue;
            }
            if (strcmp(argv[i], "stderr") == 0) {
                out = stderr;
                continue;
            }
            outfile = argv[i];
        }
    
        if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--fancy") == 0) {
            fancy = 1;
            continue;
        }

        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            fprintf(stdout, "%s", usage);
            return EXIT_SUCCESS;
        }

        if (!infile) {
            infile = argv[i];
        }
    }

    if (!infile) {
        fprintf(stderr, "%s", usage);
        return EXIT_FAILURE;
    }

    struct sudoku *sudoku = sudoku_read(infile);
    
    int nsols;
    struct sudoku **sols = sudoku_solve(sudoku, &nsols);

    if (nsols == 0) {
        puts("No solution found!");
        outfile = NULL;
    }
    if (nsols > 1) {
        puts("Multiple solutions found!");
    }

    if (outfile) {
        out = fopen(outfile, "w");
        if (!out) {
            fprintf(stderr, "Could not open output file '%s'\n", outfile);
            out = stdout;
            outfile = NULL;
        }
    }

    fprintf(out, "Puzzle:\n");
    sudoku_fprint(out, sudoku, fancy);
    for (int i = 0; i < nsols; ++i) {
        if (nsols == 1) {
            fprintf(out, "\n\nSolution:\n");
        } else {
            fprintf(out, "\n\nSolution %i:\n", i + 1);
        }
        sudoku_fprint(out, sols[i], fancy);
        sudoku_free(sols[i]);
    }

    if (outfile) {
        fclose(out);
    }

    free(sols);
    sudoku_free(sudoku);

    return EXIT_SUCCESS;
}
