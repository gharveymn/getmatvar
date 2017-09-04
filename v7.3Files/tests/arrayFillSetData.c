/*=================================================================
 * arrayFillSetData.c - example used to illustrate how to fill an mxArray
 *
 * Same functionality as arrayFillSetPr.c, except the function
 * mxSetData operates on data types other than double.
 *
 * UINT16_T declares appropriate uint16 type for platform
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
    UINT16_T *dynamicData;                        /* pointer to dynamic data */
    const UINT16_T data[] = {2, 3, 2, 1};  /* existing data */
    mwSize index;

	/* Check for proper number of arguments. */
    if ( nrhs != 0 ) {
        mexErrMsgIdAndTxt("MATLAB:arrayFillSetData:rhs",
                "This function takes no input arguments.");
    } 

    /* Create a local array and load data */
    dynamicData = mxCalloc(ELEMENTS, sizeof(UINT16_T));
    for ( index = 0; index < ELEMENTS; index++ ) {
        dynamicData[index] = data[index];
    }

    /* Create a 0-by-0 mxArray; you will allocate the memory dynamically */
    plhs[0] = mxCreateNumericMatrix(0, 0, mxUINT16_CLASS, mxREAL);

    /* Point mxArray to dynamicData */
    mxSetData(plhs[0], dynamicData);
    mxSetM(plhs[0], ROWS);
    mxSetN(plhs[0], COLUMNS);

    /* Do not call mxFree(dynamicData) because plhs[0] points to dynamicData */
    
    return;
}
