#include "headers/readMessage.h"


void readDataSpaceMessage(Data* object, byte* msg_pointer, address_t msg_address, uint16_t msg_size, error_t* err_flag)
{
	
	//assume version 1 and ignore max dims and permutation indices (never implemented in hdf5 library)
	if(*(msg_pointer) != 1)
	{
		sprintf(error_id, "getmatvar:internalError");
		sprintf(error_message, "Unexpected dataspace message version number %d (expected 1).\n\n", (int)*msg_pointer);
		*err_flag = 1;
		return;
	}
	
	object->num_dims = (uint8_t)*(msg_pointer + 1);
	if(object->num_dims > HDF5_MAX_DIMS)
	{
		sprintf(error_id, "getmatvar:dataCorruptionError");
		sprintf(error_message, "Data has too many dimensions. Data may have been corrupted.\n\n");
		*err_flag = 1;
		return;
	}
	object->dims = mxMalloc((object->num_dims + 1)*sizeof(index_t));
#ifdef NO_MEX
	if(object->dims == NULL)
	{
		sprintf(error_id, "getmatvar:mallocErrDims");
		sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
		*err_flag = 1;
		*err_flag = 1;
		return;
	}
#endif
	for(int i = 0; i < object->num_dims; i++)
	{
		object->dims[object->num_dims - i - 1] = (size_t)getBytesAsNumber(msg_pointer + 8 + i*s_block.size_of_lengths, s_block.size_of_lengths, META_DATA_BYTE_ORDER);
	}
	object->dims[object->num_dims] = 0;
	
	
	object->num_elems = 1;
	int index = 0;
	while(object->dims[index] > 0)
	{
		object->num_elems *= object->dims[index];
		index++;
	}
	
}


void readDataTypeMessage(Data* object, byte* msg_pointer, address_t msg_address, uint16_t msg_size, error_t* err_flag)
{
	//assume version 1
	if((*(msg_pointer) >> 4) != 1)
	{
		sprintf(error_id, "getmatvar:internalError");
		sprintf(error_message, "Unexpected datatype message version number %d (expected 1).\n\n", (int)(*(msg_pointer) >> 4));
		*err_flag = 1;
		return;
	}
	
	object->hdf5_internal_type = (HDF5Datatype)(*(msg_pointer) & 0x0F); //only want bottom 4 bits
	//only run once
	if(object->elem_size == 0)
	{
		object->elem_size = (uint32_t)getBytesAsNumber(msg_pointer + 4, 4, META_DATA_BYTE_ORDER);
	}
	object->datatype_bit_field = (uint32_t)getBytesAsNumber(msg_pointer + 1, 3, META_DATA_BYTE_ORDER);
	
	byte* cmpd_pointer;
	switch(object->hdf5_internal_type)
	{
		case HDF5_FIXED_POINT:
			//fixed point (string)
			//assume that the bytes wont be VAX-endian because I don't know what that is
			object->byte_order = (ByteOrder)(object->datatype_bit_field & 1);
			break;
		case HDF5_TIME:
			object->byte_order = (ByteOrder)(object->datatype_bit_field & 1);
			break;
		case HDF5_FLOATING_POINT:
			//floating point
			//no mantissa normalization, 0 based padding
			object->byte_order = (ByteOrder)(object->datatype_bit_field & 1);
			break;
		case HDF5_COMPOUND:
			if(memcmp(msg_pointer + 8, "real", 4*sizeof(byte)) == 0)
			{
				object->complexity_flag = (mxComplexity)mxCOMPLEX;
			}
			size_t next_member_offset = 8 + roundUp8(strlen((char*)msg_pointer) + 1) + 32;
			cmpd_pointer = msg_pointer + next_member_offset;
			readDataTypeMessage(object, cmpd_pointer, msg_address + next_member_offset, 20, err_flag);
			break;
		default:
			object->byte_order = LITTLE_ENDIAN;
			break;
	}
	
	
}


void readDataLayoutMessage(Data* object, byte* msg_pointer, address_t msg_address, uint16_t msg_size, error_t* err_flag)
{
	//assume version 3
	if(*msg_pointer != 3)
	{
		sprintf(error_id, "getmatvar:internalError");
		sprintf(error_message, "Unexpected data layout message version number %d (expected 3).\n\n", (int)*(msg_pointer));
		*err_flag = 1;
		return;
	}
	
	object->layout_class = (uint8_t)*(msg_pointer + 1);
	switch(object->layout_class)
	{
		case 0:
			object->data_address = msg_address + 4;
			//object->data_pointer = msg_pointer + 4;
			break;
		case 1:
			object->data_address = (address_t)getBytesAsNumber(msg_pointer + 2, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
			//object->data_pointer = msg_pointer + (object->data_address - msg_address);
			break;
		case 2:
			object->chunked_info.num_chunked_dims = (uint8_t)(*(msg_pointer + 2) - 1); //??
			object->chunked_info.chunked_dims = mxMalloc((object->chunked_info.num_chunked_dims + 1)*sizeof(index_t));
#ifdef NO_MEX
			if(object->chunked_info.chunked_dims == NULL)
			{
				sprintf(error_id, "getmatvar:mallocErrChunkedDims");
				sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
				*err_flag = 1;
				*err_flag = 1;
				return;
			}
#endif
			object->chunked_info.chunk_update = mxMalloc((object->chunked_info.num_chunked_dims + 1)*sizeof(index_t));
#ifdef NO_MEX
			if(object->chunked_info.chunk_update == NULL)
			{
				sprintf(error_id, "getmatvar:mallocErrChunkUpdate");
				sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
				*err_flag = 1;
				*err_flag = 1;
				return;
			}
#endif
			object->data_address = (address_t)getBytesAsNumber(msg_pointer + 3, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
			//object->data_pointer = msg_pointer + (object->data_address - msg_address);
			for(int j = 0; j < object->chunked_info.num_chunked_dims; j++)
			{
				//chunked storage dims are 4 bytes wide
				object->chunked_info.chunked_dims[object->chunked_info.num_chunked_dims - j - 1] = (size_t)getBytesAsNumber(msg_pointer + 3 + s_block.size_of_offsets + 4*j, 4,
																										META_DATA_BYTE_ORDER);
			}
			object->chunked_info.chunked_dims[object->chunked_info.num_chunked_dims] = 0;
			object->chunked_info.num_chunked_elems = 1;
			for(int i = 0; i < object->chunked_info.num_chunked_dims; i++)
			{
				object->chunked_info.num_chunked_elems *= object->chunked_info.chunked_dims[i];
			}
			
			makeChunkedUpdates(object->chunked_info.chunk_update, object->chunked_info.chunked_dims, object->dims, object->num_dims);
			
			break;
		default:
			sprintf(error_id, "getmatvar:internalError");
			sprintf(error_message, "Unknown data layout class %d.\n\n", object->layout_class);
			*err_flag = 1;
			return;
	}
}


void readDataStoragePipelineMessage(Data* object, byte* msg_pointer, address_t msg_address, uint16_t msg_size, error_t* err_flag)
{
	
	object->chunked_info.num_filters = (uint8_t)*(msg_pointer + 1);
	if(object->chunked_info.num_filters > 0)
	{
		object->chunked_info.filters = mxMalloc(object->chunked_info.num_filters * sizeof(Filter));
#ifdef NO_MEX
		if(object->chunked_info.filters == NULL)
		{
			sprintf(error_id, "getmatvar:mallocErrFilters");
			sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
			*err_flag = 1;
			*err_flag = 1;
			return;
		}
#endif
	}
	byte* helper_pointer = NULL;
	uint16_t name_size = 0;
	
	//version number
	switch(*msg_pointer)
	{
		case 1:
			helper_pointer = msg_pointer + 8;
			
			for(int j = 0; j < object->chunked_info.num_filters; j++)
			{
				object->chunked_info.filters[j].filter_id = (FilterType)getBytesAsNumber(helper_pointer, 2, META_DATA_BYTE_ORDER);
				name_size = (uint16_t)getBytesAsNumber(helper_pointer + 2, 2, META_DATA_BYTE_ORDER);
				object->chunked_info.filters[j].optional_flag = (uint8_t)(getBytesAsNumber(helper_pointer + 4, 2, META_DATA_BYTE_ORDER) & 1);
				object->chunked_info.filters[j].num_client_vals = (uint16_t)getBytesAsNumber(helper_pointer + 6, 2, META_DATA_BYTE_ORDER);
				object->chunked_info.filters[j].client_data = mxMalloc(object->chunked_info.filters[j].num_client_vals*sizeof(uint32_t));
#ifdef NO_MEX
				if(object->chunked_info.filters[j].client_data == NULL)
				{
					sprintf(error_id, "getmatvar:mallocErrCliDat1");
					sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
					*err_flag = 1;
					*err_flag = 1;
					return;
				}
#endif
				helper_pointer += 8 + name_size;
				for(int k = 0; k < object->chunked_info.filters[j].num_client_vals; k++)
				{
					object->chunked_info.filters[j].client_data[k] = (uint32_t)getBytesAsNumber(helper_pointer, 4, META_DATA_BYTE_ORDER);
					helper_pointer += 4;
				}
				
			}
			
			break;
		case 2:
			helper_pointer = msg_pointer + 2;
			
			for(int j = 0; j < object->chunked_info.num_filters; j++)
			{
				
				object->chunked_info.filters[j].filter_id = (FilterType)getBytesAsNumber(helper_pointer, 2, META_DATA_BYTE_ORDER);
				object->chunked_info.filters[j].optional_flag = (uint8_t)(getBytesAsNumber(helper_pointer + 2, 2, META_DATA_BYTE_ORDER) & 1);
				object->chunked_info.filters[j].num_client_vals = (uint16_t)getBytesAsNumber(helper_pointer + 4, 2, META_DATA_BYTE_ORDER);
				object->chunked_info.filters[j].client_data = mxMalloc(object->chunked_info.filters[j].num_client_vals*sizeof(uint32_t));
#ifdef NO_MEX
				if(object->chunked_info.filters[j].client_data == NULL)
				{
					sprintf(error_id, "getmatvar:mallocErrCliDat2");
					sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
					*err_flag = 1;
					*err_flag = 1;
					return;
				}
#endif
				helper_pointer += 6;
				for(int k = 0; k < object->chunked_info.filters[j].num_client_vals; k++)
				{
					object->chunked_info.filters[j].client_data[k] = (uint32_t)getBytesAsNumber(helper_pointer, 4, META_DATA_BYTE_ORDER);
					helper_pointer += 4;
				}
				
			}
			
			break;
		default:
			sprintf(error_id, "getmatvar:internalError");
			sprintf(error_message, "Unknown data storage pipeline version %d.\n\n", *msg_pointer);
			*err_flag = 1;
			return;
		
	}
}


void readAttributeMessage(Data* object, byte* msg_pointer, address_t msg_address, uint16_t msg_size, error_t* err_flag)
{
	
	//assume version 1
	if(*msg_pointer != 1)
	{
		sprintf(error_id, "getmatvar:internalError");
		sprintf(error_message, "Unexpected attribute message version number %d (expected 1).\n\n", (int)*(msg_pointer));
		*err_flag = 1;
		return;
	}
	
	char name[NAME_LENGTH] = {0};
	uint16_t name_size = (uint16_t)getBytesAsNumber(msg_pointer + 2, 2, META_DATA_BYTE_ORDER);
	uint16_t datatype_size = (uint16_t)getBytesAsNumber(msg_pointer + 4, 2, META_DATA_BYTE_ORDER);
	uint16_t dataspace_size = (uint16_t)getBytesAsNumber(msg_pointer + 6, 2, META_DATA_BYTE_ORDER);
	strncpy(name, (char*)(msg_pointer + 8), name_size);
	if(strcmp(name, "MATLAB_class") == 0)
	{
		uint32_t attribute_data_size = (uint32_t)getBytesAsNumber(msg_pointer + 8 + roundUp8(name_size) + 4, 4, META_DATA_BYTE_ORDER);
		object->matlab_class = mxMalloc((attribute_data_size + 1)*sizeof(char));
#ifdef NO_MEX
		if(object->matlab_class == NULL)
		{
			sprintf(error_id, "getmatvar:mallocErrMatCla");
			sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
			*err_flag = 1;
			*err_flag = 1;
			return;
		}
#endif
		memcpy(object->matlab_class, (char*)(msg_pointer + 8 + roundUp8(name_size) + roundUp8(datatype_size) + roundUp8(dataspace_size)), attribute_data_size*sizeof(char));
		object->matlab_class[attribute_data_size] = 0x0;
		selectMatlabClass(object);
	}
	else if(strcmp(name, "MATLAB_sparse") == 0)
	{
		object->matlab_internal_attributes.MATLAB_sparse = TRUE;
		if(object->dims != NULL)
		{
			mxFree(object->dims);
		}
		object->dims = mxMalloc(2*sizeof(index_t));
#ifdef NO_MEX
		if(unlikely(object->dims == NULL))
		{
			sprintf(error_id, "getmatvar:mallocErrSpDims");
			sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
			*err_flag = 1;
			*err_flag = 1;
			return;
		}
#endif
		memcpy(object->dims, (index_t*)(msg_pointer + 8 + roundUp8(name_size) + roundUp8(datatype_size) + roundUp8(dataspace_size)), sizeof(index_t));
		object->dims[1] = 0;
	}
	else if(strcmp(name, "MATLAB_empty") == 0)
	{
		object->matlab_internal_attributes.MATLAB_empty = TRUE;
	}
	else if(strcmp(name, "MATLAB_object_decode") == 0)
	{
		object->matlab_internal_attributes.MATLAB_object_decode = *((objectDecodingHint*)(msg_pointer + 8 + roundUp8(name_size) + roundUp8(datatype_size) + roundUp8(dataspace_size)));
		if(object->matlab_internal_attributes.MATLAB_object_decode == FUNCTION_HINT)
		{
			object->matlab_internal_type = mxFUNCTION_CLASS;
		}
		else if(object->matlab_internal_attributes.MATLAB_object_decode == OBJECT_HINT)
		{
			object->matlab_internal_type = mxOBJECT_CLASS;
		}
		else if(object->matlab_internal_attributes.MATLAB_object_decode == OPAQUE_HINT)
		{
			object->matlab_internal_type = mxOPAQUE_CLASS;
		}
	}
}


void selectMatlabClass(Data* object)
{
	//most likely first because this can be expensive in large numbers
	if(strcmp("double", object->matlab_class) == 0)
	{
		object->matlab_internal_type = mxDOUBLE_CLASS;
	}
	else if(strcmp("char", object->matlab_class) == 0)
	{
		object->matlab_internal_type = mxCHAR_CLASS;
	}
	else if(strcmp("logical", object->matlab_class) == 0)
	{
		object->matlab_internal_type = mxLOGICAL_CLASS;
	}
	else if(strcmp("struct", object->matlab_class) == 0)
	{
		object->matlab_internal_type = mxSTRUCT_CLASS;
	}
	else if(strcmp("cell", object->matlab_class) == 0)
	{
		object->matlab_internal_type = mxCELL_CLASS;
	}
	else if(strcmp("function_handle", object->matlab_class) == 0)
	{
		object->matlab_internal_type = mxFUNCTION_CLASS;
	}
	else if(strcmp("table", object->matlab_class) == 0)
	{
		object->matlab_internal_type = mxOBJECT_CLASS;
	}
	else if(strcmp("uint8", object->matlab_class) == 0)
	{
		object->matlab_internal_type = mxUINT8_CLASS;
	}
	else if(strcmp("uint16", object->matlab_class) == 0)
	{
		object->matlab_internal_type = mxUINT16_CLASS;
	}
	else if(strcmp("uint32", object->matlab_class) == 0)
	{
		object->matlab_internal_type = mxUINT32_CLASS;
	}
	else if(strcmp("uint64", object->matlab_class) == 0)
	{
		object->matlab_internal_type = mxUINT64_CLASS;
	}
	else if(strcmp("int8", object->matlab_class) == 0)
	{
		object->matlab_internal_type = mxINT8_CLASS;
	}
	else if(strcmp("int16", object->matlab_class) == 0)
	{
		object->matlab_internal_type = mxINT16_CLASS;
	}
	else if(strcmp("int32", object->matlab_class) == 0)
	{
		object->matlab_internal_type = mxINT32_CLASS;
	}
	else if(strcmp("int64", object->matlab_class) == 0)
	{
		object->matlab_internal_type = mxINT64_CLASS;
	}
	else if(strcmp("single", object->matlab_class) == 0)
	{
		object->matlab_internal_type = mxSINGLE_CLASS;
	}
	else if(strcmp("opaque", object->matlab_class) == 0)
	{
		object->matlab_internal_type = mxOPAQUE_CLASS;
	}
	else
	{
		//do nothing
	}
	
	
}



