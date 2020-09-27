#include "links.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct dlnode {
    struct dlnode *right;
    struct dlnode *left;
    struct dlnode *up;
    struct dlnode *down;
    int row_id;
    struct dlnode *col;
};

static void
dlnode_cover_column(struct dlnode *node)
{
    struct dlnode *col = node->col;

    col->right->left = col->left;
    col->left->right = col->right;

    for (struct dlnode *row = col->down; row != col; row = row->down) {
        for (struct dlnode *ptr = row->right; ptr != row; ptr = ptr->right) {
            ptr->up->down = ptr->down;
            ptr->down->up = ptr->up;
        }
    }
}

static void
dlnode_uncover_column(struct dlnode *node)
{
    struct dlnode *col = node->col;

    for (struct dlnode *row = col->up; row != col; row = row->up) {
        for (struct dlnode *ptr = row->left; ptr != row; ptr = ptr->left) {
            ptr->up->down = ptr;
            ptr->down->up = ptr;
        }
    }

    col->right->left = col;
    col->left->right = col;
}

struct dlmatrix {
    struct dlnode head;
    int nrows;
    int ncols;
    struct dlnode *cols;
    struct dlnode *data;
};

static void
dlmatrix_free(struct dlmatrix *dlmat)
{
    if (dlmat == NULL) {
        return;
    }
    free(dlmat->cols);
    free(dlmat->data);
    free(dlmat);
}

static struct dlmatrix *
dlmatrix_create(const struct bmatrix *bmat)
{
    struct dlmatrix *dlmat = malloc(sizeof *dlmat);

    const int nrows = bmat->nrows;
    const int ncols = bmat->ncols;
    dlmat->nrows = nrows;
    dlmat->ncols = ncols;

    /* Initialize headers */
    dlmat->cols = malloc(ncols * sizeof *dlmat->cols);

    dlmat->head.up = NULL;
    dlmat->head.down = NULL;
    dlmat->head.row_id = -1;
    dlmat->head.col = NULL;
    dlmat->head.right = &dlmat->cols[0];
    dlmat->head.left = &dlmat->cols[ncols - 1];

    dlmat->cols[0].left = &dlmat->head;
    dlmat->cols[0].row_id = -1;
    for (int i = 1; i < ncols - 1; ++i) {
        dlmat->cols[i].left = &dlmat->cols[i - 1];
        dlmat->cols[i - 1].right = &dlmat->cols[i];
        dlmat->cols[i].right = &dlmat->cols[i + 1];
        dlmat->cols[i + 1].left = &dlmat->cols[i];
        dlmat->cols[0].row_id = -1;
    }
    dlmat->cols[ncols - 1].right = &dlmat->head;
    dlmat->cols[ncols - 1].row_id = -1;

    /* Initialize elements */
    /* We don't save the whole binary matrix but only non-zero elements */
    int *idcs = malloc(nrows * ncols * sizeof *idcs);
    int num_elems = 0;
    for (int i = 0; i < nrows * ncols; ++i) {
        if (bmat->data[i]) {
            idcs[i] = num_elems;
            ++num_elems;
        } else {
            idcs[i] = -1;
        }
    }
    dlmat->data = malloc(num_elems * sizeof *dlmat->data);

    struct dlnode *first = NULL;
    struct dlnode *last = NULL;
    struct dlnode *prev = NULL;
    struct dlnode *curr = NULL;

    /* Fill rows */
    for (int i_row = 0; i_row < nrows; ++i_row) {
        first = NULL;
        last = NULL;
        prev = NULL;
        curr = NULL;
        for (int i_col = 0; i_col < ncols; ++i_col) {
            const int idx = idcs[i_row * ncols + i_col];
            if (idx == -1) {
                continue;
            }
            prev = curr;
            curr = &dlmat->data[idx];
            curr->row_id = i_row;
            if (!first) {
                first = curr;
            }
            if (prev) {
                prev->right = curr;
                curr->left = prev;
            }
            last = curr;
        }
        /* Rows can be empty */
        if (first) {
            first->left = last;
            last->right = first;
        }
    }

    /* Fill cols */
    struct dlnode *col;
    for (int i_col = 0; i_col < ncols; ++i_col) {
        first = NULL;
        last = NULL;
        prev = NULL;
        curr = NULL;
        col = &dlmat->cols[i_col];
        col->col = col;
        for (int i_row = 0; i_row < nrows; ++i_row) {
            const int idx = idcs[i_row * ncols + i_col];
            if (idx == -1) {
                continue;
            }
            prev = curr;
            curr = &dlmat->data[idx];
            curr->col = col;
            if (!first) {
                first = curr;
            }
            if (prev) {
                prev->down = curr;
                curr->up = prev;
            }
            last = curr;
        }
        /* Cols must not be empty */
        if (first) {
            first->up = col;
            col->down = first;
            last->down = col;
            col->up = last;
        } else {
            dlmatrix_free(dlmat);
            dlmat = NULL;
            break;
        }
    }

    free(idcs);

    return dlmat;
}

struct dlresult {
    int tot_rows;
    int *rows;
    int ctr;
    int nsols;
    struct dlsolution *sols;
};

static struct dlresult *
dlresult_create(int nrows)
{
    struct dlresult *dlres = malloc(sizeof *dlres);
    dlres->tot_rows = nrows;
    dlres->rows = malloc(nrows * sizeof *dlres->rows);
    dlres->ctr = 0;
    dlres->nsols = 0;
    dlres->sols = NULL;
    return dlres;
}

static void
dlresult_free(struct dlresult *dlres)
{
    if (dlres == NULL) {
        return;
    }
    free(dlres->rows);
    free(dlres);
}

static void
dlresult_add(struct dlresult *dlres, int row_id)
{
    dlres->rows[dlres->ctr] = row_id;
    ++dlres->ctr;
}

static void
dlresult_remove(struct dlresult *dlres, int row_id)
{
    int found = 0;
    --dlres->ctr;
    for (int i = 0; i < dlres->ctr; ++i) {
        if (!found && dlres->rows[i] == row_id) {
            found = 1;
        }
        if (found) {
            dlres->rows[i] = dlres->rows[i + 1];
        }
    }
    if (!found) {
        if (dlres->rows[dlres->ctr] != row_id) {
            fprintf(stderr, "Row id not found!");
            exit(EXIT_FAILURE);
        }
    }
}

static void
dlresult_add_solution(struct dlresult *dlres)
{
    const int nsols = ++dlres->nsols;
    if (dlres->sols) {
        /* The reallocs should be okay here */
        dlres->sols = realloc(dlres->sols, nsols * sizeof *dlres->sols);
    } else {
        dlres->sols = malloc(nsols * sizeof *dlres->sols);
    }
    int *rows = malloc(dlres->ctr * sizeof *rows);
    rows = memcpy(rows, dlres->rows, dlres->ctr * sizeof *rows);
    dlres->sols[nsols - 1].nrows = dlres->ctr;
    dlres->sols[nsols - 1].rows = rows;
}

/* Knuth's dancing links algorithm */
static void
dlresult_search(struct dlresult *dlres, struct dlmatrix *dlmat)
{
    if (dlres->nsols == 1024) {
        /* Sneaky valgrind easter egg */
        fprintf(stderr, "More than 1024 solutions found! Go fix your sudoku!\n");
        return;
    }
    
    struct dlnode *col = dlmat->head.right;

    if (col == &dlmat->head) {
        dlresult_add_solution(dlres);
        return;
    }

    dlnode_cover_column(col);
      
    for (struct dlnode *row = col->down; row != col; row = row->down) {
        dlresult_add(dlres, row->row_id);

        for (struct dlnode *ptr = row->right; ptr != row; ptr = ptr->right) {
            dlnode_cover_column(ptr);
        }

        dlresult_search(dlres, dlmat);
        dlresult_remove(dlres, row->row_id);

        for (struct dlnode *ptr = row->left; ptr != row; ptr = ptr->left) {
            dlnode_uncover_column(ptr);
        }

    }

    dlnode_uncover_column(col);
}

struct dlsolution *
dlsolution_find(const struct bmatrix *bmat, int *nsols)
{
    struct dlmatrix *dlmat = dlmatrix_create(bmat);
    if (!dlmat) {
        *nsols = 0;
        return NULL;
    }
    struct dlresult *dlres = dlresult_create(bmat->nrows);

    dlresult_search(dlres, dlmat);

    struct dlsolution *sols = dlres->sols;
    dlres->sols = NULL;
    *nsols = dlres->nsols;
    
    dlresult_free(dlres);
    dlmatrix_free(dlmat);

    return sols;
}

/* Integer comparison function for `qsort' */
static int
cmpfnc(const void *a, const void *b)
{
    return (*(int *) a - *(int *) b);
}

struct bmatrix *
bmatrix_from_dlsolution(const struct dlsolution *dlsol, const struct bmatrix *bmat)
{
    struct bmatrix *res = malloc(sizeof *res);

    const int nrows = bmat->nrows;
    const int ncols = bmat->ncols;

    res->nrows = nrows;
    res->ncols = ncols;
    res->data = malloc(nrows * ncols * sizeof *bmat->data);
    res->data = memcpy(res->data, bmat->data, nrows * ncols * sizeof *res->data);

    /* Eliminate rows */
    int nsolrows = dlsol->nrows;
    int *solrows = dlsol->rows;
    qsort(solrows, nsolrows, sizeof *solrows, &cmpfnc);
    int ctr = 0;
    int nextrow = solrows[0];
    for (int i_row = 0; i_row < nrows; ++i_row) {
        if (i_row == nextrow) {
            ++ctr;
            nextrow = (ctr == nsolrows) ? nrows : solrows[ctr];
            continue;
        }
        for (int i_col = 0; i_col < ncols; ++i_col) {
            res->data[i_row * ncols + i_col] = 0;
        }
    }

    return res;
}