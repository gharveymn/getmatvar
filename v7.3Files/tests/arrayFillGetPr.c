/*=================================================================
 * arrayFillGetPr.c - example used to illustrate how to fill an mxArray
 *
 * Create an mxArray and use mxGetPr to point to the starting
 * address of its real data. Fill mxArray with the contents 
 * of "data" and return it in plhs[0].
 *
 * Input:   none
 * Output:  mxArray
 *
 * Copyright 2008 The MathWorks, Inc.
 *	
 *=================================================================*/
#include "mex.h"

/* The mxArray in this example is 2x2 */
#define ROWS 2
#define COLUMNS 2
#define ELEMENTS 4

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
    double  *pointer;          /* pointer to real data in new array */
    mwSize index;
    const double data[] = {2.1, 3.4, 2.3, 2.45};  /* existing data */

    /* Check for proper number of arguments. */
    if ( nrhs != 0 ) {
        mexErrMsgIdAndTxt("MATLAB:arrayFillGetPr:rhs","This function takes no input arguments.");
    } 

    /* Create an m-by-n mxArray; you will copy existing data into it */
    plhs[0] = mxCreateNumericMatrix(ROWS, COLUMNS, mxDOUBLE_CLASS, mxREAL);
    pointer = mxGetPr(plhs[0]);

    /* Copy data into the mxArray */
    for ( index = 0; index < ELEMENTS; index++ ) {
        pointer[index] = data[index];
    }

    return;
}
