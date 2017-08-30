#include <mex.h>
#include "mapping.h"


void makeReturnStructure(mxArray* returnStructure[], const int num_elems, const char* varnames[], const char* filename);
mxArray* makeStruct(mxArray* returnStructure, int num_elems, Data* object);
mxArray* makeCell(mxArray* returnStructure, int num_elems, Data* objects);
char** getFieldNames(Data* object, const int num_elems);
void setDblPtr(Data object, mxArray* returnStructure, const char* varname, DataType type);
void setCharPtr(Data object, mxArray* returnStructure, const char* varname, DataType type);
void setIntPtr(Data object, mxArray* returnStructure, const char* varname, DataType type);
mwSize* makeObjDims(uint32_t* dims, const mwSize num_obj_dims);
void getNums(Data* object, mwSize* num_obj_dims, mwSize* num_elems);
void getNumFields(Data* object, mwSize* num_fields);


void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[])
{
	if(nrhs < 2)
	{
		mexErrMsgIdAndTxt("MATLAB:getmatvar:invalidNumInputs", "At least two input arguments required.");
	}
	else
	{
		char* filename = mxArrayToString(prhs[0]);
		char** varnames = malloc((nrhs - 1) * sizeof(char*));
		for(int i = 0; i < nrhs - 1; i++)
		{
			varnames[i] = mxArrayToString(prhs[i + 1]);
		}
		
		makeReturnStructure(plhs, nrhs - 1, varnames, filename);
	}
	
}


void makeReturnStructure(mxArray* returnStructure[], const int num_elems, const char* varnames[], const char* filename)
{
	mwSize ret_struct_dims[1] = {1};
	returnStructure[0] = mxCreateStructArray(1, ret_struct_dims, num_elems, varnames);
	
	mwSize num_obj_dims, num_obj_elems, num_fields = 0;
	mwSize* obj_dims;
	mxArray* mxCellPtr;
	mxArray* mxStructPtr;
	char** field_names;
	DataType data_insertion_type = STRUCT;
	
	for(int i = 0; i < num_elems; i++)
	{
		int* num_objs = (int*) malloc(sizeof(int));
		Data* objects = getDataObject(filename, varnames[i], num_objs);
		Data* hi_objects = organizeObjects(objects, *num_objs);
		int index = 0;
		mwSize num_obj_dims;
		
		while(hi_objects[index].type != UNDEF)
		{
			switch(hi_objects[index].type)
			{
				case DOUBLE:
					setDblPtr(hi_objects[index], returnStructure[0], varnames[i], data_insertion_type);
					break;
				case CHAR:
					setCharPtr(hi_objects[index], returnStructure[0], varnames[i], data_insertion_type);
					break;
				case UNSIGNEDINT16:
					setIntPtr(hi_objects[index], returnStructure[0], varnames[i], data_insertion_type);
					break;
				case REF:
					getNums(&hi_objects[index], &num_obj_dims, &num_obj_elems);
					obj_dims = makeObjDims(hi_objects[index].dims, num_obj_dims);
					getNumFields(&hi_objects[index], &num_fields);
					mxCellPtr = mxCreateCellArray(num_obj_dims, obj_dims);
					mxSetField(returnStructure[0], 0, varnames[i], makeCell(mxCellPtr, num_fields, hi_objects[index].sub_objects));
					break;
				case STRUCT:
					getNums(&hi_objects[index], &num_obj_dims, &num_obj_elems);
					obj_dims = makeObjDims(hi_objects[index].dims, num_obj_dims);
					getNumFields(&hi_objects[index], &num_fields);
					field_names = getFieldNames(&hi_objects[index], num_fields);
					mxStructPtr = mxCreateStructArray(num_obj_dims, obj_dims, num_fields, field_names);
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


mxArray* makeStruct(mxArray* returnStructure, const int num_elems, Data* objects)
{
	mwSize num_obj_dims, num_obj_elems, num_fields = 0;
	mwSize* obj_dims;
	mxArray* mxCellPtr;
	mxArray* mxStructPtr;
	char** field_names;
	DataType data_insertion_type = STRUCT;
	
	for(int index = 0; index < num_elems; index++)
	{
		switch(objects[index].type)
		{
			case DOUBLE:
				setDblPtr(objects[index], returnStructure, objects[index].name, data_insertion_type);
				break;
			case CHAR:
				setCharPtr(objects[index], returnStructure, objects[index].name, data_insertion_type);
				break;
			case UNSIGNEDINT16:
				setIntPtr(objects[index], returnStructure, objects[index].name, data_insertion_type);
				break;
			case REF:
				getNums(&objects[index], &num_obj_dims, &num_obj_elems);
				obj_dims = makeObjDims(objects[index].dims, num_obj_dims);
				getNumFields(&objects[index], &num_fields);
				mxCellPtr = mxCreateCellArray(num_obj_dims, obj_dims);
				mxSetField(returnStructure, 0, objects[index].name, makeCell(mxCellPtr, num_fields, objects[index].sub_objects));
				break;
			case STRUCT:
				getNums(&objects[index], &num_obj_dims, &num_obj_elems);
				obj_dims = makeObjDims(objects[index].dims, num_obj_dims);
				getNumFields(&objects[index], &num_fields);
				field_names = getFieldNames(&objects[index], num_fields);
				mxStructPtr = mxCreateStructArray(num_obj_dims, obj_dims, num_fields, field_names);
				mxSetField(returnStructure, 0, objects[index].name, makeStruct(mxStructPtr, num_obj_elems, objects[index].sub_objects));
				break;
			default:
				break;
		}
		
	}
	
	return returnStructure;
}


mxArray* makeCell(mxArray* returnStructure, const int num_elems, Data* objects)
{
	
	mwSize struct_dims[1] = {1};
	mwSize num_obj_dims, num_obj_elems, num_fields = 0;
	mwSize* obj_dims;
	mxArray* mxCellPtr;
	mxArray* mxStructPtr;
	char** field_names;
	DataType data_insertion_type = REF;
	
	for(int index = 0; index < num_elems; index++)
	{
		switch(objects[index].type)
		{
			case DOUBLE:
				setDblPtr(objects[index], returnStructure, &index, data_insertion_type);
				break;
			case CHAR:
				setCharPtr(objects[index], returnStructure, &index, data_insertion_type);
				break;
			case UNSIGNEDINT16:
				setIntPtr(objects[index], returnStructure, &index, data_insertion_type);
				break;
			case REF:
				getNums(&objects[index], &num_obj_dims, &num_obj_elems);
				obj_dims = makeObjDims(objects[index].dims, num_obj_dims);
				getNumFields(&objects[index], &num_fields);
				mxCellPtr = mxCreateCellArray(num_obj_dims, obj_dims);
				mxSetCell(returnStructure, index, makeCell(mxCellPtr, num_fields, objects[index].sub_objects));
				break;
			case STRUCT:
				getNums(&objects[index], &num_obj_dims, &num_obj_elems);
				obj_dims = makeObjDims(objects[index].dims, num_obj_dims);
				getNumFields(&objects[index], &num_fields);
				field_names = getFieldNames(&objects[index], num_fields);
				mxStructPtr = mxCreateStructArray(num_obj_dims, obj_dims, num_fields, field_names);
				mxSetCell(returnStructure, index, makeStruct(mxStructPtr, num_obj_elems, objects[index].sub_objects));
				break;
			default:
				break;
		}
		
	}
	
	return returnStructure;
	
}


void setDblPtr(Data object, mxArray* returnStructure, const char* varname, DataType type)
{
	mwSize num_obj_dims, num_obj_elems = 0;
	getNums(&object, &num_obj_dims, &num_obj_elems);
	mwSize* obj_dims = makeObjDims(object.dims, num_obj_dims);
	mxArray* mxDblPtr = mxCreateNumericArray(num_obj_dims, obj_dims, mxDOUBLE_CLASS, mxREAL);
	double* mxDblPtrPr = mxGetPr(mxDblPtr);
	
	for(int j = 0; j < num_obj_elems; j++)
	{
		mxDblPtrPr[j] = object.double_data[j];
	}
	
	if(type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxDblPtr);
	}
	else if(type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, *varname, mxDblPtr);
	}
}


void setCharPtr(Data object, mxArray* returnStructure, const char* varname, DataType type)
{
	mwSize num_obj_dims, num_obj_elems = 0;
	getNums(&object, &num_obj_dims, &num_obj_elems);
	char* mxCharPtrPr = mxCalloc(num_obj_elems, sizeof(char));
	
	for(int j = 0; j < num_obj_elems; j++)
	{
		mxCharPtrPr[j] = object.ushort_data[j];
	}
	mxArray* mxCharPtr = mxCreateString(mxCharPtrPr);
	
	if(type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxCharPtr);
	}
	else if(type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, *varname, mxCharPtr);
	}
	mxFree(mxCharPtrPr);
}


void setIntPtr(Data object, mxArray* returnStructure, const char* varname, DataType type)
{
	mwSize num_obj_dims, num_obj_elems = 0;
	getNums(&object, &num_obj_dims, &num_obj_elems);
	char* mxIntPtrPr = mxCalloc(num_obj_elems, sizeof(char));
	
	for(int j = 0; j < num_obj_elems; j++)
	{
		mxIntPtrPr[j] = object.ushort_data[j];
	}
	mxArray* mxIntPtr = mxCreateString(mxIntPtrPr);
	
	if(type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, *varname, mxIntPtr);
	}
	mxFree(mxIntPtrPr);
}


char** getFieldNames(Data* object, const int num_elems)
{
	char** varnames = malloc(num_elems * sizeof(char*));
	for(int index = 0; index < num_elems; index++)
	{
		varnames[index] = object->sub_objects[index].name;
	}
	return varnames;
}


void getNumFields(Data* object, mwSize* num_fields)
{
	int index = 0;
	while(object->sub_objects[index].type != UNDEF)
	{
		index++;
	}
	*num_fields = index;
}


void getNums(Data* object, mwSize* num_obj_dims, mwSize* num_elems)
{
	mwSize ne = 1;
	mwSize nd = 0;
	int i = 0;
	while(object->dims[i] > 0)
	{
		ne *= object->dims[i];
		nd++;
		i++;
	}
	*num_obj_dims = nd;
	*num_elems = ne;
}


mwSize* makeObjDims(uint32_t* dims, const mwSize num_obj_dims)
{
	mwSize* obj_dims = malloc(num_obj_dims);
	for(int i = 0; i < num_obj_dims; i++)
	{
		obj_dims[i] = (mwSize) dims[num_obj_dims - 1 - i];
	}
	return obj_dims;
}

