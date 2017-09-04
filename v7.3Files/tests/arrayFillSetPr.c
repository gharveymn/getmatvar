/*=================================================================
 * arrayFillSetPr.c - example used to illustrate how to fill an mxArray
 *
 * Create an empty mxArray. Create a dynamic data array and
 * copy your data into it. Use mxSetPr to dynamically allocate memory
 * as you fill mxArray with the contents of "dynamicData".
 *
 * Input:   none
 * Output:  mxArray
 *
 * Copyright 2008 The MathWorks, Inc.
 *
 *=================================================================*/
#include "mex.h"

/* The mxArray in this example is 2x2 */
#define ROWS 100
#define COLUMNS 101
#define PAGES 102
#define ELEMENTS 1030200

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
	double  *dynamicData;          /* pointer to dynamic data */
	const double data[] = {2.1, 3.4, 2.3, 2.45};  /* existing data */
	
	/* Check for proper number of arguments. */
	if ( nrhs != 0 ) {
		mexErrMsgIdAndTxt("MATLAB:arrayFillSetPr:rhs","This function takes no input arguments.");
	}
	
	/* Create a local array and load data */
	dynamicData = mxMalloc(ELEMENTS * sizeof(double));
	for (int i = 0; i < ELEMENTS; i++ ) 
	{
		dynamicData[i] = (double)i;
	}
	
	/* Create a 0-by-0 mxArray; you will allocate the memory dynamically */
	plhs[0] = mxCreateNumericArray(0, NULL, mxDOUBLE_CLASS, mxREAL);
	
	/* Put the C array into the mxArray and define its dimensions */
	mxSetPr(plhs[0], dynamicData);
	
	mwSize* obj_dims = malloc(3*sizeof(mwSize));
	obj_dims[0] = ROWS;
	obj_dims[1] = COLUMNS;
	obj_dims[2] = PAGES;
	mwSize num_obj_dims = 3;
	
	mxSetDimensions(plhs[0], obj_dims, num_obj_dims);
	
	free(obj_dims);
	
	/* Do not call mxFree(dynamicData) because plhs[0] points to dynamicData */
	
	return;
}
