#include "C:/Program Files/MATLAB/R2017a/extern/include/mex.h"
#include "mapping.h"

void makeReturnStructure(mxArray* returnStructure[], int num_elems, const char** varnames, const char* filename);
mxArray* makeStruct(mxArray* returnStructure, int num_elems, Data* object);
mxArray* makeCell(mxArray* returnStructure, int num_elems, Data* objects);
char** getFieldNames(const int num_elems, Data* object);
mwSize* makeObjDims(uint32_t* dims, const mwSize num_dims);
int getNumDims(Data *object);
int getNumElems(Data *object);

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
	if(nrhs < 2)
	{
		mexErrMsgIdAndTxt("MATLAB:getmatvar:invalidNumInputs",
					   "At least two input arguments required.");
	}
	else
	{
		char* filename = mxArrayToString(prhs[0]);
		char** varnames = malloc((nrhs-1)*sizeof(char*));
		for(int i = 0; i < nrhs-1; i++)
		{
			varnames[i] = mxArrayToString(prhs[i+1]);
		}
		
		makeReturnStructure(plhs, nrhs-1, varnames, filename);
	}
	
}


void makeReturnStructure(mxArray* returnStructure[], const int num_elems, const char* varnames[], const char* filename)
{
	mwSize struct_dims[1] = {1};
	returnStructure[0] = mxCreateStructArray(1, struct_dims, num_elems, varnames);
	
	mwSize num_dims;
	mwSize* obj_dims;
	mxArray* mxDblPtr;
	double* mxDblPtrPr;
	mxArray* mxCharPtr;
	char* mxCharPtrPr;
	mxArray* mxIntPtr;
	char* mxIntPtrPr;
	mxArray* mxCellPtr;
	mxArray* mxStructPtr;
	int num_cells;
	int num_fields;
	
	for(int i = 0; i < num_elems; i++)
	{
		int *num_objs = (int *) malloc(sizeof(int));
		Data *objects = getDataObject(filename, varnames[i], num_objs);
		Data *hi_objects = organizeObjects(objects, *num_objs);
		int index = 0;
		mwSize num_dims;
		
		while (hi_objects[index].type != UNDEF)
		{
			switch (hi_objects[index].type)
			{
				case DOUBLE:
					num_dims = (mwSize)getNumDims(&hi_objects[index]);
					obj_dims = makeObjDims(hi_objects[index].dims, num_dims);
					mxDblPtr = mxCreateNumericArray(num_dims, obj_dims, mxDOUBLE_CLASS, mxREAL);
					mxDblPtrPr = mxGetPr(mxDblPtr);
					for (int j = 0; j < getNumElems(&hi_objects[index]); j++)
					{
						mxDblPtrPr[j] = hi_objects[index].double_data[j];
					}
					mxSetField(returnStructure[0], 0, varnames[i], mxDblPtr);
					break;
				case CHAR:
					num_dims = (mwSize)getNumDims(&hi_objects[index]);
					obj_dims = makeObjDims(hi_objects[index].dims, num_dims);
					mxCharPtr = mxCreateCharArray(num_dims, obj_dims);
					mxCharPtrPr = mxGetChars(mxCharPtr);
					for (int j = 0; j < getNumElems(&hi_objects[index]); j++)
					{
						mxCharPtrPr[j] = hi_objects[index].char_data[j];
					}
					mxSetField(returnStructure[0], 0, varnames[i], mxCharPtr);
					break;
				case UNSIGNEDINT16:
					num_dims = (mwSize)getNumDims(&hi_objects[index]);
					obj_dims = makeObjDims(hi_objects[index].dims, num_dims);
					mxIntPtrPr = mxCalloc(obj_dims[1], sizeof(char));
					for (int j = 0; j < obj_dims[1]; j++)
					{
						mxIntPtrPr[j] = hi_objects[index].ushort_data[j];
					}
					mxIntPtr = mxCreateString(mxIntPtrPr);
					mxSetField(returnStructure[0], 0, varnames[i], mxIntPtr);
					mxFree(mxIntPtrPr);
					break;
				case REF:
					num_dims = (mwSize)getNumDims(&hi_objects[index]);
					obj_dims = makeObjDims(hi_objects[index].dims, num_dims);
					num_cells = getNumElems(hi_objects[index].sub_objects);
					mxCellPtr = mxCreateCellArray(num_dims, obj_dims);
					mxSetField(returnStructure[0], 0, varnames[i], makeCell(mxCellPtr, num_cells, hi_objects[index].sub_objects));
					break;
				case STRUCT:
					num_dims = (mwSize)getNumDims(&hi_objects[index]);
					obj_dims = (mwSize *)hi_objects[index].dims;
					num_fields = getNumElems(hi_objects[index].sub_objects);
					mxStructPtr = mxCreateStructArray(1, struct_dims, num_fields, getFieldNames(num_fields, hi_objects[index].sub_objects));
					mxSetField(returnStructure[0], 0, varnames[i], makeStruct(mxStructPtr, num_fields, hi_objects[index].sub_objects));
					break;
				default:
					break;
			}
			index++;
		}
		freeDataObjects(objects, *num_objs);
		free(hi_objects);
		
	}
}

mxArray* makeStruct(mxArray* returnStructure, const int num_elems, Data* object)
{
	mwSize struct_dims[1] = {1};
	mwSize num_dims;
	mwSize* obj_dims;
	mxArray* mxDblPtr;
	double* mxDblPtrPr;
	mxArray* mxCharPtr;
	char* mxCharPtrPr;
	mxArray* mxIntPtr;
	uint16_t* mxIntPtrPr;
	mxArray* mxCellPtr;
	mxArray* mxStructPtr;
	int num_cells;
	int num_fields;
	
	for(int i = 0; i < num_elems; i++)
	{
		int index = 0;
		
		while (object[index].type != UNDEF)
		{
			switch (object[index].type)
			{
				case DOUBLE:
					num_dims = (mwSize)getNumDims(&object[index]);
					obj_dims = makeObjDims(object[index].dims, num_dims);
					mxDblPtr = mxCreateNumericArray(num_dims, obj_dims, mxDOUBLE_CLASS, mxREAL);
					mxDblPtrPr = mxGetPr(mxDblPtr);
					for (int j = 0; j < getNumElems(&object[index]); j++)
					{
						mxDblPtrPr[j] = object[index].double_data[j];
					}
					mxSetField(returnStructure, 0, object[index].name, mxDblPtr);
					break;
				case CHAR:
					num_dims = (mwSize)getNumDims(&object[index]);
					obj_dims = makeObjDims(object[index].dims, num_dims);
					mxCharPtr = mxCreateCharArray(num_dims, obj_dims);
					mxCharPtrPr = (char *)mxGetChars(mxCharPtr);
					for (int j = 0; j < getNumElems(&object[index]); j++)
					{
						mxCharPtrPr[j] = object[index].char_data[j];
					}
					mxSetField(returnStructure, 0, object[index].name, mxCharPtr);
					break;
				case UNSIGNEDINT16:
					num_dims = (mwSize)getNumDims(&object[index]);
					obj_dims = makeObjDims(object[index].dims, num_dims);
					mxIntPtrPr = mxCalloc(obj_dims[1], sizeof(char));
					for (int j = 0; j < obj_dims[1]; j++)
					{
						mxIntPtrPr[j] = object[index].ushort_data[j];
					}
					mxIntPtr = mxCreateString(mxIntPtrPr);
					mxSetField(returnStructure, 0, object[index].name, mxIntPtr);
					mxFree(mxIntPtrPr);
					break;
				case REF:
					num_dims = (mwSize)getNumDims(&object[index]);
					obj_dims = makeObjDims(object[index].dims, num_dims);
					mxCellPtr = mxCreateCellArray(num_dims, obj_dims);
					num_cells = getNumElems(object[index].sub_objects);
					mxSetField(returnStructure, 0, object[index].name, makeCell(mxCellPtr, num_cells, object[index].sub_objects));
					break;
				case STRUCT:
					num_dims = (mwSize)getNumDims(&object[index]);
					obj_dims = makeObjDims(object[index].dims, num_dims);
					num_fields = getNumElems(object[index].sub_objects);
					mxStructPtr = mxCreateStructArray(1, struct_dims, num_fields, getFieldNames(num_fields, object[index].sub_objects));
					mxSetField(returnStructure, 0, object[index].name, makeStruct(returnStructure, num_fields, object[index].sub_objects));
					break;
				default:
					break;
			}
			index++;
		}
		
	}
	
	return returnStructure;
}


mxArray* makeCell(mxArray* returnStructure, const int num_elems, Data* object)
{
	
	mwSize struct_dims[1] = {1};
	mwSize num_dims;
	mwSize* obj_dims;
	mxArray* mxDblPtr;
	double* mxDblPtrPr;
	mxArray* mxCharPtr;
	char* mxCharPtrPr;
	mxArray* mxIntPtr;
	uint16_t* mxIntPtrPr;
	mxArray* mxCellPtr;
	mxArray* mxStructPtr;
	int num_cells;
	int num_fields;
	
	for(int i = 0; i < num_elems; i++)
	{
		int index = 0;
		
		while (object[index].type != UNDEF)
		{
			switch (object[index].type)
			{
				case DOUBLE:
					num_dims = (mwSize)getNumDims(&object[index]);
					obj_dims = makeObjDims(object[index].dims, num_dims);
					mxDblPtr = mxCreateNumericArray(num_dims, obj_dims, mxDOUBLE_CLASS, mxREAL);
					mxDblPtrPr = mxGetPr(mxDblPtr);
					for (int j = 0; j < getNumElems(&object[index]); j++)
					{
						mxDblPtrPr[j] = object[index].double_data[j];
					}
					mxSetCell(returnStructure, i, mxDblPtr);
					break;
				case CHAR:
					num_dims = (mwSize)getNumDims(&object[index]);
					obj_dims = makeObjDims(object[index].dims, num_dims);
					mxCharPtr = mxCreateCharArray(num_dims, obj_dims);
					mxCharPtrPr = (char *)mxGetChars(mxCharPtr);
					for (int j = 0; j < getNumElems(&object[index]); j++)
					{
						mxCharPtrPr[j] = object[index].char_data[j];
					}
					mxSetCell(returnStructure, i, mxCharPtr);
					break;
				case UNSIGNEDINT16:
					num_dims = (mwSize)getNumDims(&object[index]);
					obj_dims = makeObjDims(object[index].dims, num_dims);
					mxIntPtrPr = mxCalloc(obj_dims[1], sizeof(char));
					for (int j = 0; j < obj_dims[1]; j++)
					{
						mxIntPtrPr[j] = object[index].ushort_data[j];
					}
					mxIntPtr = mxCreateString(mxIntPtrPr);
					mxSetCell(returnStructure, i, mxIntPtr);
					mxFree(mxIntPtrPr);
					break;
				case REF:
					num_dims = (mwSize)getNumDims(&object[index]);
					obj_dims = makeObjDims(object[index].dims, num_dims);
					mxCellPtr = mxCreateCellArray(num_dims, obj_dims);
					num_cells = getNumElems(object[index].sub_objects);
					mxSetCell(returnStructure, i, makeCell(mxCellPtr, num_cells, object[index].sub_objects));
					break;
				case STRUCT:
					num_dims = (mwSize)getNumDims(&object[index]);
					obj_dims = makeObjDims(object[index].dims, num_dims);
					num_fields = getNumElems(object[index].sub_objects);
					mxStructPtr = mxCreateStructArray(1, struct_dims, num_fields, getFieldNames(num_fields, object[index].sub_objects));
					mxSetField(returnStructure, 0, object[index].name, makeStruct(returnStructure, num_fields, object[index].sub_objects));
					break;
				default:
					break;
			}
			index++;
		}
		
	}
	
	return returnStructure;
	
}

char** getFieldNames(const int num_elems, Data* object)
{
	char** varnames = malloc(num_elems*sizeof(char*));
	int index = 0;
	while (object->sub_objects[index].type != UNDEF)
	{
		varnames[index] = object->sub_objects[index].name;
		index++;
	}
	
}


int getNumDims(Data *object)
{
	int num_dims = 0;
	int i = 0;
	while (object->dims[i] > 0)
	{
		num_dims++;
		i++;
	}
	return num_dims;
}

int getNumElems(Data *object)
{
	int num_elems = 1;
	int i = 0;
	while (object->dims[i] > 0)
	{
		num_elems *= object->dims[i];
		i++;
	}
	return num_elems;
}

mwSize* makeObjDims(uint32_t* dims, const mwSize num_dims)
{
	mwSize* obj_dims = malloc(num_dims);
	obj_dims[0] = 1;
	for(int i = 0; i < num_dims-1; i++)
	{
		obj_dims[i+1] = (mwSize)dims[i];
	}
	return obj_dims;
}

