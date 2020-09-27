/* sudoku.h
 *
 * Header for sudoku struct and functions.
 *
 */

#ifndef SUDOKU_H
#define SUDOKU_H

#include <stdio.h>

/* Structure to hold necessary data of sudoku puzzle */
struct sudoku {
    int size;
    int nelems;
    int *data;
};

/* Create sudoku from data read from input file FILE */
struct sudoku *
sudoku_read(const char *file);

/* Free memory of sudoku SUDOKU */
void
sudoku_free(struct sudoku *sudoku);

/* Solve sudoku SUDOKU with dancing links algorithm. Returns array of solutions 
 * and writes number to NSOLS */
struct sudoku **
sudoku_solve(const struct sudoku *sudoku, int *nsols);

/* Print data in SUDOKU to OUT. FANCY toggles between plain matrix (= false) and 
 * separators between blocks (= true). */
void
sudoku_fprint(FILE *out, const struct sudoku *sudoku, int fancy);

#endif /* SUDOKU_H */