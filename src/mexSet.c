#include "headers/getDataObjects.h"


void setNumericPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, mxClassID super_structure_type)
{
	mwSize* obj_dims = makeObjDims(object->dims, object->num_dims);
	mxArray* mxNumericPtr = mxCreateNumericArray(0, NULL, object->matlab_internal_type, object->complexity_flag);
	if(object->complexity_flag == mxCOMPLEX)
	{
		DataArrays imag_data = rearrangeImaginaryData(object);
		mxSetData(mxNumericPtr, (void*)object->data_arrays.data);
		mxSetImagData(mxNumericPtr, (void*)imag_data.data);
	}
	else
	{
		mxSetData(mxNumericPtr, (void*)object->data_arrays.data);
	}
	mxSetDimensions(mxNumericPtr, obj_dims, object->num_dims);
	
	if(super_structure_type == mxSTRUCT_CLASS)
	{
		mxSetField(returnStructure, index, varname, mxNumericPtr);
	}
	else if(super_structure_type == mxCELL_CLASS)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxNumericPtr);
	}
	
	free(obj_dims);
}


void setLogicPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, mxClassID super_structure_type)
{
	mwSize* obj_dims = makeObjDims(object->dims, object->num_dims);
	mxArray* mxLogicPtr = mxCreateLogicalArray(0, NULL);
	mxSetData(mxLogicPtr, (void*)object->data_arrays.data);
	mxSetDimensions(mxLogicPtr, obj_dims, object->num_dims);
	
	if(super_structure_type == mxSTRUCT_CLASS)
	{
		mxSetField(returnStructure, index, varname, mxLogicPtr);
	}
	else if(super_structure_type == mxCELL_CLASS)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxLogicPtr);
	}
	
	free(obj_dims);
}


void setCharPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, mxClassID super_structure_type)
{
	mwSize* obj_dims = makeObjDims(object->dims, object->num_dims);
	mxArray* mxCharPtr = mxCreateCharArray(0, NULL);
	mxSetData(mxCharPtr, (void*)object->data_arrays.data);
	mxSetDimensions(mxCharPtr, obj_dims, object->num_dims);
	
	if(super_structure_type == mxSTRUCT_CLASS)
	{
		mxSetField(returnStructure, index, varname, mxCharPtr);
	}
	else if(super_structure_type == mxCELL_CLASS)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxCharPtr);
	}
	
	free(obj_dims);
}


void setSpsPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, mxClassID super_structure_type)
{
	
	Data* data = findSubObjectByShortName(object, "data");
	Data* ir = findSubObjectByShortName(object, "ir");
	Data* jc = findSubObjectByShortName(object, "jc");
	//mxArray* mxSpsPtr = mxCreateSparse(object->dims[0], jc->num_elems - 1, ir->num_elems, object->complexity_flag);
	mxArray* mxSpsPtr = NULL;
	
	switch(object->matlab_sparse_type)
	{
		case mxDOUBLE_CLASS:
			mxSpsPtr = mxCreateSparse(((mwSize*)object->dims)[0], ((mwSize*)jc->dims)[0] - 1, 0, object->complexity_flag);
			break;
		case mxLOGICAL_CLASS:
			mxSpsPtr = mxCreateSparseLogicalMatrix(((mwSize*)object->dims)[0], ((mwSize*)jc->dims)[0] - 1, 0);
			break;
		default:
			//error;
			break;
			
	}

	if (data == NULL)
	{
		//Already set to zero and NULL
		//mxSetNzmax(mxSpsPtr, 0)
	}
	else
	{

		mxSetNzmax(mxSpsPtr, (mwSize)ir->num_elems);

		if(object->complexity_flag == mxCOMPLEX)
		{
			DataArrays imag_data = rearrangeImaginaryData(data);
			mxSetData(mxSpsPtr, (void*)data->data_arrays.data);
			mxSetImagData(mxSpsPtr, (void*)imag_data.data);
		}
		else
		{
			mxSetData(mxSpsPtr, (void*)data->data_arrays.data);
		}
		data->data_flags.is_mx_used = TRUE;

		mwIndex* irPtr = mxMalloc(ir->num_elems*sizeof(mwIndex));
		for (int i = 0; i < ir->num_elems; i++)
		{
			irPtr[i] = ((mwIndex*)ir->data_arrays.data)[i];
		}
		mxSetIr(mxSpsPtr, irPtr);

		ir->data_flags.is_mx_used = FALSE;

	}

	//sparse matrices always have two dimensions
	
	mwIndex* jcPtr = mxGetJc(mxSpsPtr);
	for(int i = 0; i < jc->num_elems; i++)
	{
		jcPtr[i] = ((mwIndex*)jc->data_arrays.data)[i];
	}
	jc->data_flags.is_mx_used = FALSE;
	
	if(super_structure_type == mxSTRUCT_CLASS)
	{
		mxSetField(returnStructure, index, varname, mxSpsPtr);
	}
	else if(super_structure_type == mxCELL_CLASS)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxSpsPtr);
	}
	
}


void setCellPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, mxClassID super_structure_type)
{
	mwSize* obj_dims = makeObjDims(object->dims, object->num_dims);
	int num_fields = object->num_sub_objs;
	mxArray* mxCellPtr = mxCreateCellArray(object->num_dims, obj_dims);
	
	if(super_structure_type == mxSTRUCT_CLASS)
	{
		mxSetField(returnStructure, index, varname, makeSubstructure(mxCellPtr, num_fields, object->sub_objects, mxCELL_CLASS));
	}
	else if(super_structure_type == mxCELL_CLASS)
	{
		mxSetCell(returnStructure, index, makeSubstructure(mxCellPtr, num_fields, object->sub_objects, mxCELL_CLASS));
	}
	
	free(obj_dims);
}


void setStructPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, mxClassID super_structure_type)
{
	mwSize* obj_dims = makeObjDims(object->dims, object->num_dims);
	int num_fields = object->num_sub_objs;
	char** field_names = getFieldNames(object);
	mxArray* mxStructPtr = mxCreateStructArray(object->num_dims, obj_dims, num_fields, field_names);
	
	if(super_structure_type == mxSTRUCT_CLASS)
	{
		mxSetField(returnStructure, index, varname, makeSubstructure(mxStructPtr, num_fields, object->sub_objects, mxSTRUCT_CLASS));
	}
	else if(super_structure_type == mxCELL_CLASS)
	{
		mxSetCell(returnStructure, index, makeSubstructure(mxStructPtr, num_fields, object->sub_objects, mxSTRUCT_CLASS));
	}
	
	free(field_names);
	free(obj_dims);
	
}


char** getFieldNames(Data* object)
{
	char** varnames = malloc((object->num_sub_objs)*sizeof(char*));
	for(uint16_t index = 0; index < object->num_sub_objs; index++)
	{
		Data* obj = dequeue(object->sub_objects);
		varnames[index] = obj->names.short_name;
	}
	restartQueue(object->sub_objects);
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
	imag_data.data = NULL;
	imag_data.sub_object_header_offsets = NULL;
	
	switch(object->matlab_internal_type)
	{
		case mxINT8_CLASS:
		case mxUINT8_CLASS:
		case mxINT16_CLASS:
		case mxUINT16_CLASS:
		case mxINT32_CLASS:
		case mxUINT32_CLASS:
		case mxINT64_CLASS:
		case mxUINT64_CLASS:
		case mxSINGLE_CLASS:
		case mxDOUBLE_CLASS:
			imag_data.data = mxMalloc(object->num_elems*object->elem_size/2);
			for(uint32_t i = 0; i < object->num_elems; i++)
			{
				memcpy(imag_data.data + i*object->elem_size/2, object->data_arrays.data + (2*i + 1)*object->elem_size/2, object->elem_size/2);
				memcpy(object->data_arrays.data + i*object->elem_size/2, object->data_arrays.data + i*object->elem_size, object->elem_size/2);
			}
			mxRealloc(object->data_arrays.data, object->num_elems*object->elem_size/2);
			break;
		default:
			//this shouldn't happen
			readMXError("getmatvar:thisShouldntHappen", "Imaginary data was not numeric.\n\n");
	}
	
	return imag_data;
	
}