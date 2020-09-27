#include "sudoku.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "bmatrix.h"
#include "links.h"

/* Insert ELEM into sudoku of size SSIZE at sudoku row SROW and sudoku column 
 * SCOL into binary matrix BMAT such that it can by solved by dancing links. */
static void
bmatrix_insert(struct bmatrix *bmat, int ssize, int srow, int scol, int elem)
{
    const int nelems = ssize * ssize;
    const int sblock = (srow / ssize) * ssize + (scol / ssize);

    int brow = (srow * nelems + scol) * nelems + (elem - 1);

    int bcol = srow * nelems + scol;
    bmat->data[brow * bmat->ncols + bcol] = 1;

    bcol = 1 * nelems * nelems + srow * nelems + (elem - 1);
    bmat->data[brow * bmat->ncols + bcol] = 1;

    bcol = 2 * nelems * nelems + scol * nelems + (elem - 1);
    bmat->data[brow * bmat->ncols + bcol] = 1;
    
    bcol = 3 * nelems * nelems + sblock * nelems + (elem - 1);
    bmat->data[brow * bmat->ncols + bcol] = 1;
}

/* Check if ELEM is in row ROW of sudoku SUDOKU */
static int
sudoku_inrow(const struct sudoku *sudoku, int row, int col, int elem)
{
    (void) col;
    for (int i = 0; i < sudoku->nelems; ++i) {
        if (sudoku->data[row * sudoku->nelems + i] == elem) {
            return 1;
        }
    }
    return 0;
}

/* Check if ELEM is in column COL of sudoku SUDOKU */
static int
sudoku_incol(const struct sudoku *sudoku, int row, int col, int elem)
{
    (void) row;
    for (int i = 0; i < sudoku->nelems; ++i) {
        if (sudoku->data[i * sudoku->nelems + col] == elem) {
            return 1;
        }
    }
    return 0;
}

/* Check if ELEM is in block at row ROW and column COL of sudoku SUDOKU */
static int
sudoku_inblock(const struct sudoku *sudoku, int row, int col, int elem)
{
    const int orow = (row / sudoku->size) * sudoku->size;
    const int ocol = (col / sudoku->size) * sudoku->size;
    for (int i = 0; i < sudoku->size; ++i) {
        for (int j = 0; j < sudoku->size; ++j) {
            int srow = orow + i;
            int scol = ocol + j;
            if (sudoku->data[srow * sudoku->nelems + scol] == elem) {
                return 1;
            }
        }
    }
    return 0;
}

/* Convert data of sudoku SUDOKU to binary matrix */
static struct bmatrix *
bmatrix_from_sudoku(const struct sudoku *sudoku)
{
    struct bmatrix *bmat = malloc(sizeof *bmat);

    bmat->nrows = sudoku->nelems * sudoku->nelems * sudoku->nelems;
    bmat->ncols = 4 * sudoku->nelems * sudoku->nelems;
    bmat->data = calloc(bmat->nrows * bmat->ncols, sizeof *bmat->data);

    const int ssize = sudoku->size;
    for (int row = 0; row < sudoku->nelems; ++row) {
        for (int col = 0; col < sudoku->nelems; ++col) {
            int elem = sudoku->data[row * sudoku->nelems + col];
            if (elem != 0) {
                bmatrix_insert(bmat, ssize, row, col, elem);
                continue;
            }
            for (elem = 1; elem <= sudoku->nelems; ++elem) {
                if (sudoku_inrow(sudoku, row, col, elem)) {
                    continue;
                }
                if (sudoku_incol(sudoku, row, col, elem)) {
                    continue;
                }
                if (sudoku_inblock(sudoku, row, col, elem)) {
                    continue;
                }
                bmatrix_insert(bmat, ssize, row, col, elem);
            }
        }
    }

    return bmat;
}

/* Free memory of BMAT */
static void
bmatrix_free(struct bmatrix *bmat)
{
    if (!bmat) {
        return;
    }
    free(bmat->data);
    free(bmat);
}

/* Convert data of binary matrix BMAT to sudoku */
static struct sudoku *
sudoku_from_bmatrix(const struct bmatrix *bmat)
{
    struct sudoku *sudoku = malloc(sizeof *sudoku);

    sudoku->nelems = (int) cbrt(bmat->nrows);
    sudoku->size = (int) sqrt(sudoku->nelems);

    const int totnum = sudoku->nelems * sudoku->nelems;
    sudoku->data = malloc(totnum * sizeof *sudoku->data);
    for (int i = 0; i < totnum; ++i) {
        int j;
        for (j = 0; bmat->data[j * bmat->ncols + i] != 1; ++j);
        sudoku->data[i] = (j % sudoku->nelems) + 1;
    }

    return sudoku;
}

/* Longer lines (and larger sudokus) are INSANITY!!1 */
#define LINE_BUFFER_SIZE 1024

struct sudoku *
sudoku_read(const char *file)
{
    struct sudoku *sudoku = malloc(sizeof *sudoku);

    FILE *in = fopen(file, "r");
    if (!in) {
        fprintf(stderr, "Could not read file '%s'!\n", file);
        exit(EXIT_FAILURE);
    }

    /* Read input file line by line */
    char linebuf[LINE_BUFFER_SIZE];
    int ctr = 0;
    char *endptr;
    do {
        char *str = fgets(linebuf, LINE_BUFFER_SIZE, in);
        if (str == NULL) {
            break;
        }
        do {
            int i = strtol(str, &endptr, 10);
            if ((i == 0 && endptr == NULL) || str == endptr) {
                fprintf(stderr, "Invalid character in '%s'!\n", file);
                exit(EXIT_FAILURE);
            } else {
                ++ctr;
                str = endptr;
            }
        } while (*endptr != '\0' && *endptr != '\n' && *endptr != EOF);
    } while (*endptr != '\0' && *endptr != EOF);

    /* Check if sudoku is valid (square of squares) */
    const int nelems = (int) sqrt(ctr);
    const int size = (int) sqrt(nelems);
    if (size * size != nelems || nelems * nelems != ctr) {
        fprintf(stderr, "Sudoku in file '%s' is invalid!\n", file);
        exit(EXIT_FAILURE);
    }
    sudoku->size = size;
    sudoku->nelems = nelems;
    sudoku->data = malloc(ctr * sizeof *sudoku->data);

    rewind(in);
    ctr = 0;
    do {
        char *str = fgets(linebuf, LINE_BUFFER_SIZE, in);
        if (str == NULL) {
            break;
        }
        do {
            int i = strtol(str, &endptr, 10);
            if (i > sudoku->nelems || i < 0) {
                fprintf(stderr, "Invalid number in '%s'!\n", file);
                exit(EXIT_FAILURE);
            }
            sudoku->data[ctr++] = i;
            str = endptr;
        } while (*endptr != '\0' && *endptr != '\n' && *endptr != EOF);
    } while (*endptr != '\0' && *endptr != EOF);

    fclose(in);
    return sudoku;
}

void
sudoku_free(struct sudoku *sudoku)
{
    if (!sudoku) {
        return;
    }
    free(sudoku->data);
    free(sudoku);
}

struct sudoku **
sudoku_solve(const struct sudoku *sudoku, int *nsols)
{
    struct bmatrix *bmat = bmatrix_from_sudoku(sudoku);
    struct dlsolution *dlsol = dlsolution_find(bmat, nsols);
    if (*nsols == 0) {
        bmatrix_free(bmat);
        return NULL;
    }

    struct sudoku **sols = malloc(*nsols * sizeof *sols);

    for (int i = 0; i < *nsols; ++i) {
        struct bmatrix *solmat = bmatrix_from_dlsolution(&dlsol[i], bmat);
        sols[i] = sudoku_from_bmatrix(solmat);
        bmatrix_free(solmat);
        free(dlsol[i].rows);
    }

    free(dlsol);
    bmatrix_free(bmat);

    return sols;
}

void
sudoku_fprint(FILE *out, const struct sudoku *sudoku, int fancy)
{
    if (!out || !sudoku) {
        return;
    }

    const int size = sudoku->size;
    const int nelems = sudoku->nelems;
    const int width = (int) ceil(log10(nelems));

    char *clrline = NULL;
    char *sepline = NULL;

    if (fancy) {
        const int buflen = width + 2;
        char *clrbuf = calloc(buflen + 1, sizeof *clrbuf);
        char *sepbuf = calloc(buflen + 1, sizeof *sepbuf);
        clrbuf = memset(clrbuf, ' ', buflen);
        sepbuf = memset(sepbuf, '-', buflen);

        const int linelen = nelems * buflen + 3 * (size - 1);
        clrline = calloc(linelen + 1, sizeof *clrline);
        sepline = calloc(linelen + 1, sizeof *sepline);
        for (int i = 0; i < size; ++i) {
            for (int j = 0; j < size; ++j) {
                clrline = strncat(clrline, clrbuf, linelen);
                sepline = strncat(sepline, sepbuf, linelen);
            }
            if (i < size - 1) {
                clrline = strncat(clrline, " | ", linelen);
                sepline = strncat(sepline, "-+-", linelen);
            }
        }
        clrline[linelen] = '\0';
        sepline[linelen] = '\0';

        free(clrbuf);
        free(sepbuf);

        fprintf(out, "%s\n", clrline);
    }

    for (int i = 0; i < nelems; ++i) {
        for (int j = 0; j < nelems; ++j) {
            const int elem = sudoku->data[i * nelems + j];
            if (fancy && (j > 0) && (j % size == 0)) {
                fprintf(out, " %c ", '|');
            }
            if (elem) {
                fprintf(out, " %*i ", width, elem);
            } else {
                fprintf(out, " %*c ", width, ' ');
            }
        }
        if (fancy) {
            fprintf(out, "\n%s\n", clrline);
            if ((i < nelems - 1) && (i + 1) % size == 0) {
                fprintf(out, "%s\n", sepline);
                fprintf(out, "%s\n", clrline);
            }
        } else {
            fprintf(out, "\n");
        }
    }

    if (fancy) {
        free(clrline);
        free(sepline);
    }
}