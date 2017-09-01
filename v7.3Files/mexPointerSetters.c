#include "getmatvar.h"

//for logicals and ui8s
void setUI8Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	mwSize num_obj_dims = object->num_dims;
	mwSize num_obj_elems = object->num_elems;
	mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	mxArray* mxIntPtr;
	uint8_t* mxIntPtrPr;
	if(strncmp(object->name,"logical",7) == 0)
	{
		mxIntPtr = mxCreateLogicalArray(num_obj_dims, obj_dims);
		mxIntPtrPr = mxGetLogicals(mxIntPtr);
	}
	else
	{
		mxIntPtr = mxCreateNumericArray(num_obj_dims, obj_dims, mxUINT8_CLASS, mxREAL);
		mxIntPtrPr = mxGetData(mxIntPtr);
	}
	
	for(int j = 0; j < num_obj_elems; j++)
	{
		mxIntPtrPr[j] = object->data_arrays.ui8_data[j];
	}
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	
}

void setI8Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	mwSize num_obj_dims = object->num_dims;
	mwSize num_obj_elems = object->num_elems;
	mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	mxArray* mxIntPtr = mxCreateNumericArray(num_obj_dims, obj_dims, mxINT8_CLASS, mxREAL);
	int8_t* mxIntPtrPr = mxGetData(mxIntPtr);
	
	for(int j = 0; j < num_obj_elems; j++)
	{
		mxIntPtrPr[j] = object->data_arrays.i8_data[j];
	}
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	
}

//for strings and ui16s
void setUI16Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	mwSize num_obj_dims = object->num_dims;
	mwSize num_obj_elems = object->num_elems;
	const mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	mxArray* mxIntPtr;
	uint16_t* mxIntPtrPr;
	if(strncmp(object->matlab_class,"char",7) == 0)
	{
		mxIntPtr = mxCreateCharArray(num_obj_dims, obj_dims);
		mxIntPtrPr = mxGetChars(mxIntPtr);
	}
	else if(strncmp(object->matlab_class,"uint16",6) == 0)
	{
		mxIntPtr = mxCreateNumericArray(num_obj_dims, obj_dims, mxUINT16_CLASS, mxREAL);
		 mxIntPtrPr = mxGetData(mxIntPtr);
	}
	else
	{
		mxIntPtr = mxCreateLogicalArray(num_obj_dims, obj_dims);
		mxIntPtrPr= mxGetLogicals(mxIntPtr);
	}
	
	memcpy(mxIntPtrPr, object->data_arrays.ui16_data, num_obj_elems * object->elem_size);
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	
}

void setI16Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	mwSize num_obj_dims = object->num_dims;
	mwSize num_obj_elems = object->num_elems;
	mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	mxArray* mxIntPtr = mxCreateNumericArray(num_obj_dims, obj_dims, mxINT16_CLASS, mxREAL);
	int16_t* mxIntPtrPr = mxGetData(mxIntPtr);
	
	for(int j = 0; j < num_obj_elems; j++)
	{
		mxIntPtrPr[j] = object->data_arrays.i16_data[j];
	}
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	
}

void setUI32Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	mwSize num_obj_dims = object->num_dims;
	mwSize num_obj_elems = object->num_elems;
	mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	mxArray* mxIntPtr = mxCreateNumericArray(num_obj_dims, obj_dims, mxUINT32_CLASS, mxREAL);
	uint32_t* mxIntPtrPr = mxGetData(mxIntPtr);
	
	for(int j = 0; j < num_obj_elems; j++)
	{
		mxIntPtrPr[j] = object->data_arrays.ui32_data[j];
	}
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	
}

void setI32Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	mwSize num_obj_dims = object->num_dims;
	mwSize num_obj_elems = object->num_elems;
	mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	mxArray* mxIntPtr = mxCreateNumericArray(num_obj_dims, obj_dims, mxINT32_CLASS, mxREAL);
	int32_t* mxIntPtrPr = mxGetData(mxIntPtr);
	
	for(int j = 0; j < num_obj_elems; j++)
	{
		mxIntPtrPr[j] = object->data_arrays.i32_data[j];
	}
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	
}

void setUI64Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	mwSize num_obj_dims = object->num_dims;
	mwSize num_obj_elems = object->num_elems;
	mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	mxArray* mxIntPtr = mxCreateNumericArray(num_obj_dims, obj_dims, mxUINT64_CLASS, mxREAL);
	uint64_t* mxIntPtrPr = mxGetData(mxIntPtr);
	
	for(int j = 0; j < num_obj_elems; j++)
	{
		mxIntPtrPr[j] = object->data_arrays.ui64_data[j];
	}
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	
}

void setI64Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	mwSize num_obj_dims = object->num_dims;
	mwSize num_obj_elems = object->num_elems;
	mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	mxArray* mxIntPtr = mxCreateNumericArray(num_obj_dims, obj_dims, mxINT64_CLASS, mxREAL);
	int64_t* mxIntPtrPr = mxGetData(mxIntPtr);
	
	for(int j = 0; j < num_obj_elems; j++)
	{
		mxIntPtrPr[j] = object->data_arrays.i64_data[j];
	}
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	
}

void setSglPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	mwSize num_obj_dims = object->num_dims;
	mwSize num_obj_elems = object->num_elems;
	mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	mxArray* mxSglPtr = mxCreateNumericArray(num_obj_dims, obj_dims, mxSINGLE_CLASS, mxREAL);
	double* mxSglPtrPr = mxGetPr(mxSglPtr);
	
	//must copy over because all objects are freed at the end of execution
	for(int j = 0; j < num_obj_elems; j++)
	{
		mxSglPtrPr[j] = object->data_arrays.single_data[j];
	}
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxSglPtr);
	}
	else if(super_structure_type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxSglPtr);
	}
	
}

void setDblPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	mwSize num_obj_dims = object->num_dims;
	mwSize num_obj_elems = object->num_elems;
	mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	mxArray* mxDblPtr = mxCreateNumericArray(num_obj_dims, obj_dims, mxDOUBLE_CLASS, mxREAL);
	double* mxDblPtrPr = mxGetPr(mxDblPtr);
	
	//must copy over because all objects are freed at the end of execution
	for(int j = 0; j < num_obj_elems; j++)
	{
		mxDblPtrPr[j] = object->data_arrays.double_data[j];
	}
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxDblPtr);
	}
	else if(super_structure_type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxDblPtr);
	}
	
}

void setCellPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	DataType this_stucture_type = REF;
	
	mwSize num_obj_dims = object->num_dims;
	mwSize num_obj_elems = object->num_elems;
	mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	int num_fields = object->num_sub_objs;
	mxArray* mxCellPtr = mxCreateCellArray(num_obj_dims, obj_dims);
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, makeSubstructure(mxCellPtr, num_fields, object->sub_objects, this_stucture_type));
	}
	else if(super_structure_type == REF)
	{
		mxSetCell(returnStructure, index, makeSubstructure(mxCellPtr, num_fields, object->sub_objects, this_stucture_type));
	}
	
}

void setStructPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	DataType this_stucture_type = STRUCT;
	
	mwSize num_obj_dims = object->num_dims;
	mwSize num_obj_elems = object->num_elems;
	mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	int num_fields = object->num_sub_objs;
	const char** field_names = getFieldNames(object);
	mxArray* mxStructPtr = mxCreateStructArray(num_obj_dims, obj_dims, num_fields, field_names);
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, makeSubstructure(mxStructPtr, num_fields, object->sub_objects, this_stucture_type));
	}
	else if(super_structure_type == REF)
	{
		mxSetCell(returnStructure, index, makeSubstructure(mxStructPtr, num_fields, object->sub_objects, this_stucture_type));
	}
	
	free(field_names);
	
}