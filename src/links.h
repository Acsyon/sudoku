/* links.h
 *
 * Header for dancing links solver.
 *
 */

#ifndef LINKS_H
#define LINKS_H

#include "bmatrix.h"

struct dlsolution {
    int nrows;
    int *rows;
};

/* Solve binary matrix BMAT with dancing links algorithm. Returns array to found 
 * solutions and stores number in NSOLS */
struct dlsolution *
dlsolution_find(const struct bmatrix *bmat, int *nsols);

/* Returns solved binary matrix based on dancing links solution DLSOL and input
 * binary matrix BMAT */
struct bmatrix *
bmatrix_from_dlsolution(const struct dlsolution *dlsol, const struct bmatrix *bmat);

#endif /* LINKS_H */