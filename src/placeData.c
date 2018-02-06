#include "headers/placeData.h"


error_t allocateSpace(Data* object)
{
	
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
		case mxLOGICAL_CLASS:
		case mxCHAR_CLASS:
			if(object->elem_size > 16)
			{
				sprintf(error_id, "getmatvar:unexpectedSizeError");
				sprintf(error_message, "A variable had a datatype of unexpected size.\n\n");
				return 1;
			}
			//note that we need AVX alignment for matlab
			object->data_arrays.data = mxMalloc((size_t)object->num_elems*object->elem_size);
#ifdef NO_MEX
			if(object->data_arrays.data == NULL)
			{
				sprintf(error_id, "getmatvar:mallocErrData");
				sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
				return 1;
			}
#endif
			break;
		case mxCELL_CLASS:
			if(object->elem_size > 16)
			{
				sprintf(error_id, "getmatvar:unexpectedSizeError");
				sprintf(error_message, "A variable had a datatype of unexpected size.\n\n");
				return 1;
			}
			//STORE ADDRESSES IN THE UDOUBLE_DATA ARRAY; THESE ARE NOT ACTUAL ELEMENTS
			object->data_arrays.sub_object_header_offsets = mxMalloc(object->num_elems*sizeof(address_t));
#ifdef NO_MEX
			if(object->data_arrays.sub_object_header_offsets == NULL)
			{
				sprintf(error_id, "getmatvar:mallocErrSOHO");
				sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
				return 1;
			}
#endif
			break;
		case mxSTRUCT_CLASS:
		case mxFUNCTION_CLASS:
			//Don't allocate anything yet. This will be handled later
			object->num_elems = 1;
			object->num_dims = 2;
			if(object->dims != NULL)
			{
				mxFree(object->dims);
			}
			object->dims = mxMalloc(3*sizeof(index_t));
#ifdef NO_MEX
			if(object->dims == NULL)
			{
				//very very unlikely
				sprintf(error_id, "getmatvar:mallocErrDataSt");
				sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
				return 1;
			}
#endif
			object->dims[0] = 1;
			object->dims[1] = 1;
			object->dims[2] = 0;
			break;
		case mxOBJECT_CLASS:
		case mxSPARSE_CLASS:
		case mxOPAQUE_CLASS:
			//do nothing
			break;
		case mxUNKNOWN_CLASS:
		default:
			sprintf(error_id, "getmatvar:unknownTypeError");
			sprintf(error_message, "Tried to allocate space for an unknown type.\n\n");
			return 1;
		
	}
	
	return 0;
	
}


void placeData(Data* object, byte* data_pointer, index_t dst_ind, index_t src_ind, index_t num_elems, size_t elem_size, ByteOrder data_byte_order)
{
	
	//reverse the bytes if the byte order doesn't match the cpu architecture
	if(__byte_order__ != data_byte_order)
	{
		for(index_t j = 0; j < num_elems; j += elem_size)
		{
			reverseBytes(data_pointer + j, elem_size);
		}
	}
	
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
		case mxLOGICAL_CLASS:
		case mxCHAR_CLASS:
			memcpy(object->data_arrays.data + elem_size*dst_ind, data_pointer + elem_size*src_ind, num_elems*elem_size);
			break;
		case mxCELL_CLASS:
			for(index_t i = 0; i < num_elems; i++)
			{
				memcpy(object->data_arrays.sub_object_header_offsets + (dst_ind + i), data_pointer + (src_ind + i)*elem_size, sizeof(address_t));
			}
			break;
		case mxSTRUCT_CLASS:
		case mxFUNCTION_CLASS:
		case mxOBJECT_CLASS:
		case mxSPARSE_CLASS:
		case mxOPAQUE_CLASS:
			break;
		case mxUNKNOWN_CLASS:
			if(object->data_flags.is_struct_array == TRUE)
			{
				for(index_t i = 0; i < num_elems; i++)
				{
					memcpy(object->data_arrays.sub_object_header_offsets + (dst_ind + i), data_pointer + (src_ind + i)*elem_size, sizeof(address_t));
				}
			}
			break;
		default:
			//nothing to be done
			break;
		
	}
	
}


void placeDataWithIndexMap(Data* object, byte* data_pointer, index_t num_elems, index_t elem_size, ByteOrder data_byte_order, const index_t* index_map, const index_t* index_sequence)
{
	
	//reverse the bytes if the byte order doesn't match the cpu architecture
	if(__byte_order__ != data_byte_order)
	{
		for(index_t j = 0; j < num_elems; j += elem_size)
		{
			reverseBytes(data_pointer + j, (size_t)elem_size);
		}
	}
	
	
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
		case mxLOGICAL_CLASS:
		case mxCHAR_CLASS:
			for(index_t j = 0; j < num_elems; j++)
			{
				memcpy(object->data_arrays.data + elem_size*index_map[index_sequence[j]], data_pointer + index_sequence[j]*elem_size, (size_t)elem_size);
			}
			break;
		case mxCELL_CLASS:
			for(index_t j = 0; j < num_elems; j++)
			{
				memcpy(object->data_arrays.sub_object_header_offsets + elem_size*index_map[index_sequence[j]], data_pointer + index_sequence[j]*elem_size, (size_t)elem_size);
			}
			break;
		case mxSTRUCT_CLASS:
		case mxFUNCTION_CLASS:
		case mxOBJECT_CLASS:
		case mxSPARSE_CLASS:
		default:
			//nothing to be done
			break;
		
	}
	
}
