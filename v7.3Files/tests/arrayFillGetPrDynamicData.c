/*=================================================================
 * arrayFillGetPrDynamicData.c - example used to illustrate how to fill an mxArray
 *
 * Create an mxArray and use mxGetPr to point to the starting
 * address of its real data, as in the arrayFillGetPr.c example.
 * Create a dynamic data array and copy your data into it.
 * Fill mxArray with the contents of "dynamicData" and return it in plhs[0].
 *
 * Input:   none
 * Output:  mxArray
 *
 * Copyright 2008-2011 The MathWorks, Inc.
 *	
 *=================================================================*/
#include "mex.h"

/* The mxArray in this example is 2x2 */
#define ROWS 2
#define COLUMNS 2
#define ELEMENTS 4

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
    double  *pointer;          /* pointer to real data in new array */
    double  *dynamicData;      /* pointer to dynamic data */
    const double data[] = {2.1, 3.4, 2.3, 2.45};  /* existing data */
    mwSize index;

	/* Check for proper number of arguments. */
    if ( nrhs != 0 ) {
        mexErrMsgIdAndTxt("MATLAB:arrayFillGetPrDynamicData:rhs",
                "This function takes no input arguments.");
    } 

    /* Create a local array and load data */
    dynamicData = mxMalloc(ELEMENTS * sizeof(double));
    for ( index = 0; index < ELEMENTS; index++ ) {
        dynamicData[index] = data[index];
    }

    /* Create a 2-by-2 mxArray; you will copy existing data into it */
    plhs[0] = mxCreateNumericMatrix(ROWS, COLUMNS, mxDOUBLE_CLASS, mxREAL);
    pointer = mxGetPr(plhs[0]);

    /* Copy data into the mxArray */
    for ( index = 0; index < ELEMENTS; index++ ) {
        pointer[index] = dynamicData[index];
    }

    /* You must call mxFree(dynamicData) on the local data.
       This is safe because you copied the data into plhs[0] */
    mxFree(dynamicData);
    
    return;
}
