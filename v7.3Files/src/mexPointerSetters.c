#include "getmatvar_.h"

//for logicals and ui8s
void setUI8Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	mwSize num_obj_dims = object->num_dims;
	mwSize num_obj_elems = object->num_elems;
	mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	mxArray* mxIntPtr;
	if(strncmp(object->matlab_class,"logical",7) == 0)
	{
		mxIntPtr = mxCreateLogicalArray(0, NULL);
	}
	else
	{
		mxIntPtr = mxCreateNumericArray(0, NULL, mxUINT8_CLASS, mxREAL);
	}

	mxSetData(mxIntPtr, object->data_arrays.ui8_data);
	mxSetDimensions(mxIntPtr, obj_dims, num_obj_dims);
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	
	free(obj_dims);
	
}

void setI8Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	mwSize num_obj_dims = object->num_dims;
	mwSize num_obj_elems = object->num_elems;
	mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	mxArray* mxIntPtr = mxCreateNumericArray(0, NULL, mxINT8_CLASS, mxREAL);
	mxSetData(mxIntPtr, object->data_arrays.i8_data);
	mxSetDimensions(mxIntPtr, obj_dims, num_obj_dims);
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	
	free(obj_dims);
	
}

//for strings and ui16s
void setUI16Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	mwSize num_obj_dims = object->num_dims;
	mwSize num_obj_elems = object->num_elems;
	mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	mxArray* mxIntPtr;
	if(strncmp(object->matlab_class,"char",7) == 0)
	{
		mxIntPtr = mxCreateCharArray(0, NULL);
	}
	else if(strncmp(object->matlab_class,"uint16",6) == 0)
	{
		mxIntPtr = mxCreateNumericArray(0, NULL, mxUINT16_CLASS, mxREAL);
	}
	else
	{
		mxIntPtr = mxCreateLogicalArray(num_obj_dims, obj_dims);
		//hotfix, i have no idea why they did this, 97 == 'a' though...
		for (int i = 0; i < num_obj_elems; i++)
		{
			if (object->data_arrays.ui16_data[i] == 97)
			{
				object->data_arrays.ui16_data[i] = 1;
			}
		}

	}

	mxSetData(mxIntPtr, object->data_arrays.ui16_data);
	mxSetDimensions(mxIntPtr, obj_dims, num_obj_dims);

	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	
	free(obj_dims);
	
}

void setI16Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	mwSize num_obj_dims = object->num_dims;
	mwSize num_obj_elems = object->num_elems;
	mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	mxArray* mxIntPtr = mxCreateNumericArray(0, NULL, mxINT16_CLASS, mxREAL);
	mxSetData(mxIntPtr, object->data_arrays.i16_data);
	mxSetDimensions(mxIntPtr, obj_dims, num_obj_dims);
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	
	free(obj_dims);
	
}

void setUI32Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	mwSize num_obj_dims = object->num_dims;
	mwSize num_obj_elems = object->num_elems;
	mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	mxArray* mxIntPtr = mxCreateNumericArray(0, NULL, mxUINT32_CLASS, mxREAL);
	mxSetData(mxIntPtr, object->data_arrays.ui32_data);
	mxSetDimensions(mxIntPtr, obj_dims, num_obj_dims);
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	
	free(obj_dims);
	
}

void setI32Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	mwSize num_obj_dims = object->num_dims;
	mwSize num_obj_elems = object->num_elems;
	mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	mxArray* mxIntPtr = mxCreateNumericArray(0, NULL, mxINT32_CLASS, mxREAL);
	mxSetData(mxIntPtr, object->data_arrays.i32_data);
	mxSetDimensions(mxIntPtr, obj_dims, num_obj_dims);
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	
	free(obj_dims);
	
}

void setUI64Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	mwSize num_obj_dims = object->num_dims;
	mwSize num_obj_elems = object->num_elems;
	mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	mxArray* mxIntPtr = mxCreateNumericArray(0, NULL, mxUINT64_CLASS, mxREAL);
	mxSetData(mxIntPtr, object->data_arrays.ui64_data);
	mxSetDimensions(mxIntPtr, obj_dims, num_obj_dims);
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	
	free(obj_dims);
	
}

void setI64Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	mwSize num_obj_dims = object->num_dims;
	mwSize num_obj_elems = object->num_elems;
	mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	mxArray* mxIntPtr = mxCreateNumericArray(0, NULL, mxINT64_CLASS, mxREAL);
	mxSetData(mxIntPtr, object->data_arrays.i64_data);
	mxSetDimensions(mxIntPtr, obj_dims, num_obj_dims);
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	
	free(obj_dims);
	
}

void setSglPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	mwSize num_obj_dims = object->num_dims;
	mwSize num_obj_elems = object->num_elems;
	mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	mxArray* mxSglPtr = mxCreateNumericArray(0, NULL, mxSINGLE_CLASS, mxREAL);
	mxSetData(mxSglPtr, object->data_arrays.single_data);
	mxSetDimensions(mxSglPtr, obj_dims, num_obj_dims);
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxSglPtr);
	}
	else if(super_structure_type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxSglPtr);
	}
	
	free(obj_dims);
	
}

void setDblPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	mwSize num_obj_dims = object->num_dims;
	mwSize num_obj_elems = object->num_elems;
	mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	mxArray* mxDblPtr = mxCreateNumericArray(0, NULL, mxDOUBLE_CLASS, mxREAL);
	mxSetPr(mxDblPtr, object->data_arrays.double_data);
	mxSetDimensions(mxDblPtr, obj_dims, num_obj_dims);
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxDblPtr);
	}
	else if(super_structure_type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxDblPtr);
	}
	
	free(obj_dims);
	
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
	
	free(obj_dims);
	
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
	free(obj_dims);
	
}

const char** getFieldNames(Data* object)
{
	const char** varnames = malloc(object->num_sub_objs * sizeof(char*));
	for(uint16_t index = 0; index < object->num_sub_objs; index++)
	{
		varnames[index] = object->sub_objects[index]->name;
	}
	return varnames;
}


mwSize* makeObjDims(const uint32_t* dims, const mwSize num_obj_dims)
{
	
	//ie. flip them around...
	
	mwSize* obj_dims = malloc(num_obj_dims * sizeof(mwSize));
	for(int i = 0; i < num_obj_dims; i++)
	{
		obj_dims[i] = (mwSize) dims[num_obj_dims - 1 - i];
	}
	return obj_dims;
	
}