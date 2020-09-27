/* bmatrix.h
 *
 * Header for binary matrix struct to be solved by dancing links algorithm.
 *
 */

#ifndef BMATRIX_H
#define BMATRIX_H

#include <inttypes.h>

struct bmatrix {
    int nrows;
    int ncols;
    uint8_t *data;
};

#endif /* BMATRIX_H */