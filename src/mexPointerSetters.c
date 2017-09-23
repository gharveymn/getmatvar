#include "mapping.h"

//void setNumericPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
//{
//	mwSize* obj_dims = makeObjDims(object->dims, object->num_dims);
//	mxArray* mxIntPtr = mxCreateNumericArray(0, NULL, mxINT16_CLASS, object->complexity_flag);
//	mxSetData(mxIntPtr, object->data_arrays.i16_data);
//	mxSetDimensions(mxIntPtr, obj_dims, object->num_dims);
//
//	if(super_structure_type == STRUCT_DATA)
//	{
//		mxSetField(returnStructure, 0, varname, mxIntPtr);
//	}
//	else if(super_structure_type == REF_DATA)
//	{
//		//is a cell array
//		mxSetCell(returnStructure, index, mxIntPtr);
//	}
//
//	free(obj_dims);
//}

//for logicals and ui8s
void setUI8Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	mwSize* obj_dims = makeObjDims(object->dims, object->num_dims);
	mxArray* mxIntPtr;
	if(strncmp(object->matlab_class, "logical", 7) == 0)
	{
		mxIntPtr = mxCreateLogicalArray(0, NULL);
	}
	else
	{
		mxIntPtr = mxCreateNumericArray(0, NULL, mxUINT8_CLASS, object->complexity_flag);
	}
	
	if(object->complexity_flag == mxCOMPLEX)
	{
		DataArrays imag_data = rearrangeImaginaryData(object);
		mxSetData(mxIntPtr, object->data_arrays.ui8_data);
		mxSetImagData(mxIntPtr, imag_data.ui8_data);
	}
	else
	{
		mxSetData(mxIntPtr, object->data_arrays.ui8_data);
	}
	mxSetDimensions(mxIntPtr, obj_dims, object->num_dims);
	
	if(super_structure_type == STRUCT_DATA)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF_DATA)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	
	free(obj_dims);
	
}


void setI8Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	mwSize* obj_dims = makeObjDims(object->dims, object->num_dims);
	mxArray* mxIntPtr = mxCreateNumericArray(0, NULL, mxINT8_CLASS, object->complexity_flag);
	if(object->complexity_flag == mxCOMPLEX)
	{
		DataArrays imag_data = rearrangeImaginaryData(object);
		mxSetData(mxIntPtr, object->data_arrays.i8_data);
		mxSetImagData(mxIntPtr, imag_data.i8_data);
	}
	else
	{
		mxSetData(mxIntPtr, object->data_arrays.i8_data);
	}
	mxSetDimensions(mxIntPtr, obj_dims, object->num_dims);
	
	if(super_structure_type == STRUCT_DATA)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF_DATA)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	
	free(obj_dims);
	
}


//for strings and ui16s
void setUI16Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	mwSize* obj_dims = makeObjDims(object->dims, object->num_dims);
	mxArray* mxIntPtr;
	if(strncmp(object->matlab_class, "char", 7) == 0)
	{
		mxIntPtr = mxCreateCharArray(0, NULL);
	}
	else
	{
		mxIntPtr = mxCreateNumericArray(0, NULL, mxUINT16_CLASS, object->complexity_flag);
	}
	
	if(object->complexity_flag == mxCOMPLEX)
	{
		DataArrays imag_data = rearrangeImaginaryData(object);
		mxSetData(mxIntPtr, object->data_arrays.ui16_data);
		mxSetImagData(mxIntPtr, imag_data.ui16_data);
	}
	else
	{
		mxSetData(mxIntPtr, object->data_arrays.ui16_data);
	}
	mxSetDimensions(mxIntPtr, obj_dims, object->num_dims);
	
	if(super_structure_type == STRUCT_DATA)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF_DATA)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	
	free(obj_dims);
	
}


void setI16Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	mwSize* obj_dims = makeObjDims(object->dims, object->num_dims);
	mxArray* mxIntPtr = mxCreateNumericArray(0, NULL, mxINT16_CLASS, object->complexity_flag);
	if(object->complexity_flag == mxCOMPLEX)
	{
		DataArrays imag_data = rearrangeImaginaryData(object);
		mxSetData(mxIntPtr, object->data_arrays.i16_data);
		mxSetImagData(mxIntPtr, imag_data.i16_data);
	}
	else
	{
		mxSetData(mxIntPtr, object->data_arrays.i16_data);
	}
	mxSetDimensions(mxIntPtr, obj_dims, object->num_dims);
	
	if(super_structure_type == STRUCT_DATA)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF_DATA)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	
	free(obj_dims);
	
}


void setUI32Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	mwSize* obj_dims = makeObjDims(object->dims, object->num_dims);
	mxArray* mxIntPtr = mxCreateNumericArray(0, NULL, mxUINT32_CLASS, object->complexity_flag);
	if(object->complexity_flag == mxCOMPLEX)
	{
		DataArrays imag_data = rearrangeImaginaryData(object);
		mxSetData(mxIntPtr, object->data_arrays.ui32_data);
		mxSetImagData(mxIntPtr, imag_data.ui32_data);
	}
	else
	{
		mxSetData(mxIntPtr, object->data_arrays.ui32_data);
	}
	mxSetDimensions(mxIntPtr, obj_dims, object->num_dims);
	
	if(super_structure_type == STRUCT_DATA)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF_DATA)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	
	free(obj_dims);
	
}


void setI32Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	mwSize* obj_dims = makeObjDims(object->dims, object->num_dims);
	mxArray* mxIntPtr = mxCreateNumericArray(0, NULL, mxINT32_CLASS, object->complexity_flag);
	if(object->complexity_flag == mxCOMPLEX)
	{
		DataArrays imag_data = rearrangeImaginaryData(object);
		mxSetData(mxIntPtr, object->data_arrays.i32_data);
		mxSetImagData(mxIntPtr, imag_data.i32_data);
	}
	else
	{
		mxSetData(mxIntPtr, object->data_arrays.i32_data);
	}
	mxSetDimensions(mxIntPtr, obj_dims, object->num_dims);
	
	if(super_structure_type == STRUCT_DATA)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF_DATA)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	
	free(obj_dims);
	
}


void setUI64Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	mwSize* obj_dims = makeObjDims(object->dims, object->num_dims);
	mxArray* mxIntPtr = mxCreateNumericArray(0, NULL, mxUINT64_CLASS, object->complexity_flag);
	if(object->complexity_flag == mxCOMPLEX)
	{
		DataArrays imag_data = rearrangeImaginaryData(object);
		mxSetData(mxIntPtr, object->data_arrays.ui64_data);
		mxSetImagData(mxIntPtr, imag_data.ui64_data);
	}
	else
	{
		mxSetData(mxIntPtr, object->data_arrays.ui64_data);
	}
	mxSetDimensions(mxIntPtr, obj_dims, object->num_dims);
	
	if(super_structure_type == STRUCT_DATA)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF_DATA)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	
	free(obj_dims);
	
}


void setI64Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	mwSize* obj_dims = makeObjDims(object->dims, object->num_dims);
	mxArray* mxIntPtr = mxCreateNumericArray(0, NULL, mxINT64_CLASS, object->complexity_flag);
	if(object->complexity_flag == mxCOMPLEX)
	{
		DataArrays imag_data = rearrangeImaginaryData(object);
		mxSetData(mxIntPtr, object->data_arrays.i64_data);
		mxSetImagData(mxIntPtr, imag_data.i64_data);
	}
	else
	{
		mxSetData(mxIntPtr, object->data_arrays.i64_data);
	}
	mxSetDimensions(mxIntPtr, obj_dims, object->num_dims);
	
	if(super_structure_type == STRUCT_DATA)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF_DATA)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	
	free(obj_dims);
	
}


void setSglPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	mwSize* obj_dims = makeObjDims(object->dims, object->num_dims);
	mxArray* mxSglPtr = mxCreateNumericArray(0, NULL, mxSINGLE_CLASS, object->complexity_flag);
	if(object->complexity_flag == mxCOMPLEX)
	{
		DataArrays imag_data = rearrangeImaginaryData(object);
		mxSetData(mxSglPtr, object->data_arrays.single_data);
		mxSetImagData(mxSglPtr, imag_data.single_data);
	}
	else
	{
		mxSetData(mxSglPtr, object->data_arrays.single_data);
	}
	mxSetDimensions(mxSglPtr, obj_dims, object->num_dims);
	
	if(super_structure_type == STRUCT_DATA)
	{
		mxSetField(returnStructure, 0, varname, mxSglPtr);
	}
	else if(super_structure_type == REF_DATA)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxSglPtr);
	}
	
	free(obj_dims);
	
}


void setDblPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	mwSize* obj_dims = makeObjDims(object->dims, object->num_dims);
	mxArray* mxDblPtr = mxCreateNumericArray(0, NULL, mxDOUBLE_CLASS, object->complexity_flag);
	if(object->complexity_flag == mxCOMPLEX)
	{
		DataArrays imag_data = rearrangeImaginaryData(object);
		mxSetPr(mxDblPtr, object->data_arrays.double_data);
		mxSetPi(mxDblPtr, imag_data.double_data);
	}
	else
	{
		mxSetPr(mxDblPtr, object->data_arrays.double_data);
	}
	mxSetDimensions(mxDblPtr, obj_dims, object->num_dims);
	
	if(super_structure_type == STRUCT_DATA)
	{
		mxSetField(returnStructure, 0, varname, mxDblPtr);
	}
	else if(super_structure_type == REF_DATA)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxDblPtr);
	}
	
	free(obj_dims);
	
}


void setFHPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	mxArray* mxFHPtr = mxCreateString(object->matlab_class);
	
	if(super_structure_type == STRUCT_DATA)
	{
		mxSetField(returnStructure, 0, varname, mxFHPtr);
	}
	else if(super_structure_type == REF_DATA)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxFHPtr);
	}
	
}


void setCellPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	mwSize* obj_dims = makeObjDims(object->dims, object->num_dims);
	int num_fields = object->num_sub_objs;
	mxArray* mxCellPtr = mxCreateCellArray(object->num_dims, obj_dims);
	
	if(super_structure_type == STRUCT_DATA)
	{
		mxSetField(returnStructure, 0, varname, makeSubstructure(mxCellPtr, num_fields, object->sub_objects, REF_DATA));
	}
	else if(super_structure_type == REF_DATA)
	{
		mxSetCell(returnStructure, index, makeSubstructure(mxCellPtr, num_fields, object->sub_objects, REF_DATA));
	}
	
	free(obj_dims);
}


void setStructPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	mwSize* obj_dims = makeObjDims(object->dims, object->num_dims);
	int num_fields = object->num_sub_objs;
	const char** field_names = getFieldNames(object);
	mxArray* mxStructPtr = mxCreateStructArray(object->num_dims, obj_dims, num_fields, field_names);
	
	if(super_structure_type == STRUCT_DATA)
	{
		mxSetField(returnStructure, 0, varname, makeSubstructure(mxStructPtr, num_fields, object->sub_objects, STRUCT_DATA));
	}
	else if(super_structure_type == REF_DATA)
	{
		mxSetCell(returnStructure, index, makeSubstructure(mxStructPtr, num_fields, object->sub_objects, STRUCT_DATA));
	}
	
	free(field_names);
	free(obj_dims);
	
}


const char** getFieldNames(Data* object)
{
	const char** varnames = malloc(object->num_sub_objs*sizeof(char*));
	for(uint16_t index = 0; index < object->num_sub_objs; index++)
	{
		varnames[index] = object->sub_objects[index]->name;
	}
	return varnames;
}


mwSize* makeObjDims(const uint32_t* dims, const mwSize num_dims)
{
	
	mwSize* obj_dims = malloc(num_dims*sizeof(mwSize));
	for(int i = 0; i < num_dims; i++)
	{
		obj_dims[i] = (mwSize)dims[i];
	}
	return obj_dims;
	
}


DataArrays rearrangeImaginaryData(Data* object)
{
	DataArrays imag_data;
	
	switch(object->type)
	{
		case INT8_DATA:
			imag_data.i8_data = mxMalloc(object->num_elems*object->elem_size/2);
			for(uint32_t i = 0; i < object->num_elems/2; i++)
			{
				imag_data.i8_data[i] = object->data_arrays.i8_data[2*i + 1];
				object->data_arrays.i8_data[i] = object->data_arrays.i8_data[2*i];
			}
			mxRealloc(object->data_arrays.i8_data, object->num_elems*object->elem_size/2);
			break;
		case UINT8_DATA:
			imag_data.ui8_data = mxMalloc(object->num_elems*object->elem_size/2);
			for(uint32_t i = 0; i < object->num_elems/2; i++)
			{
				imag_data.ui8_data[i] = object->data_arrays.ui8_data[2*i + 1];
				object->data_arrays.ui8_data[i] = object->data_arrays.ui8_data[2*i];
			}
			mxRealloc(object->data_arrays.ui8_data, object->num_elems*object->elem_size/2);
			break;
		case INT16_DATA:
			imag_data.i16_data = mxMalloc(object->num_elems*object->elem_size/2);
			for(uint32_t i = 0; i < object->num_elems/2; i++)
			{
				imag_data.i16_data[i] = object->data_arrays.i16_data[2*i + 1];
				object->data_arrays.i16_data[i] = object->data_arrays.i16_data[2*i];
			}
			mxRealloc(object->data_arrays.i16_data, object->num_elems*object->elem_size/2);
			break;
		case UINT16_DATA:
			imag_data.ui8_data = mxMalloc(object->num_elems*object->elem_size/2);
			for(uint32_t i = 0; i < object->num_elems/2; i++)
			{
				imag_data.ui8_data[i] = object->data_arrays.ui8_data[2*i + 1];
				object->data_arrays.ui8_data[i] = object->data_arrays.ui8_data[2*i];
			}
			mxRealloc(object->data_arrays.ui8_data, object->num_elems*object->elem_size/2);
			break;
		case INT32_DATA:
			imag_data.i32_data = mxMalloc(object->num_elems*object->elem_size/2);
			for(uint32_t i = 0; i < object->num_elems/2; i++)
			{
				imag_data.i32_data[i] = object->data_arrays.i32_data[2*i + 1];
				object->data_arrays.i32_data[i] = object->data_arrays.i32_data[2*i];
			}
			mxRealloc(object->data_arrays.i32_data, object->num_elems*object->elem_size/2);
			break;
		case UINT32_DATA:
			imag_data.ui32_data = mxMalloc(object->num_elems*object->elem_size/2);
			for(uint32_t i = 0; i < object->num_elems/2; i++)
			{
				imag_data.ui32_data[i] = object->data_arrays.ui32_data[2*i + 1];
				object->data_arrays.ui32_data[i] = object->data_arrays.ui32_data[2*i];
			}
			mxRealloc(object->data_arrays.ui32_data, object->num_elems*object->elem_size/2);
			break;
		case INT64_DATA:
			imag_data.i64_data = mxMalloc(object->num_elems*object->elem_size/2);
			for(uint32_t i = 0; i < object->num_elems/2; i++)
			{
				imag_data.i64_data[i] = object->data_arrays.i64_data[2*i + 1];
				object->data_arrays.i64_data[i] = object->data_arrays.i64_data[2*i];
			}
			mxRealloc(object->data_arrays.i64_data, object->num_elems*object->elem_size/2);
			break;
		case UINT64_DATA:
			imag_data.ui64_data = mxMalloc(object->num_elems*object->elem_size/2);
			for(uint32_t i = 0; i < object->num_elems/2; i++)
			{
				imag_data.ui64_data[i] = object->data_arrays.ui64_data[2*i + 1];
				object->data_arrays.ui64_data[i] = object->data_arrays.ui64_data[2*i];
			}
			mxRealloc(object->data_arrays.ui64_data, object->num_elems*object->elem_size/2);
			break;
		case SINGLE_DATA:
			imag_data.single_data = mxMalloc(object->num_elems*object->elem_size/2);
			for(uint32_t i = 0; i < object->num_elems/2; i++)
			{
				imag_data.single_data[i] = object->data_arrays.single_data[2*i + 1];
				object->data_arrays.single_data[i] = object->data_arrays.single_data[2*i];
			}
			mxRealloc(object->data_arrays.single_data, object->num_elems*object->elem_size/2);
			break;
		case DOUBLE_DATA:
			imag_data.double_data = mxMalloc(object->num_elems*object->elem_size/2);
			for(uint32_t i = 0; i < object->num_elems/2; i++)
			{
				imag_data.double_data[i] = object->data_arrays.double_data[2*i + 1];
				object->data_arrays.double_data[i] = object->data_arrays.double_data[2*i];
			}
			mxRealloc(object->data_arrays.double_data, object->num_elems*object->elem_size/2);
			break;
		default:
			//this shouldn't happen
			readMXError("getmatvar:thisShouldntHappen", "Imaginary was not numeric.\n\n");
	}
	
	return imag_data;
}