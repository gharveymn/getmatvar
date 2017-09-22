#include "mapping.h"


void readDataSpaceMessage(Data* object, byte* msg_pointer, uint64_t msg_address, uint16_t msg_size)
{
	
	//assume version 1 and ignore max dims and permutation indices (never implemented in hdf5 library)
	object->num_dims = (uint8_t)*(msg_pointer + 1);
	
	for(int i = 0; i < object->num_dims; i++)
	{
		object->dims[object->num_dims-i-1] = (uint32_t)getBytesAsNumber(msg_pointer + 8 + i * s_block.size_of_lengths, 4, META_DATA_BYTE_ORDER);
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

void readDataTypeMessage(Data* object, byte* msg_pointer, uint64_t msg_address, uint16_t msg_size)
{
	object->type = NULLTYPE_DATA;
	//assume version 1
	uint8_t class = (uint8_t)(*(msg_pointer) & 0x0F); //only want bottom 4 bits
	object->elem_size = (uint32_t)getBytesAsNumber(msg_pointer + 4, 4, META_DATA_BYTE_ORDER);
	object->datatype_bit_field = (uint32_t)getBytesAsNumber(msg_pointer + 1, 3, META_DATA_BYTE_ORDER);
	uint8_t sign;
	
	switch(class)
	{
		case 0:
			//fixed point (string)
			
			//assume that the bytes wont be VAX-endian because I don't know what that is
			object->byte_order = (ByteOrder)(object->datatype_bit_field & 1);
			sign = (uint8_t)(object->datatype_bit_field >> 3);
			
			//size in bits
			switch(object->elem_size)
			{
				case 1:
					if(sign)
					{
						object->type = INT8_DATA;
					}
					else
					{
						object->type = UINT8_DATA;
					}
					break;
				case 2:
					if(sign)
					{
						object->type = INT16_DATA;
					}
					else
					{
						object->type = UINT16_DATA;
					}
					break;
				case 4:
					if(sign)
					{
						object->type = INT32_DATA;
					}
					else
					{
						object->type = UINT32_DATA;
					}
					break;
				case 8:
					if(sign)
					{
						object->type = INT64_DATA;
					}
					else
					{
						object->type = UINT64_DATA;
					}
					break;
				default:
					//this shouldn't happen
					object->type = NULLTYPE_DATA;
					break;
			}
			break;
		case 1:
			//floating point
			object->byte_order = (ByteOrder)(object->datatype_bit_field & 1);
			
			//no mantissa normalization, 0 based padding
			switch(object->elem_size)
			{
				case 4:
					object->type = SINGLE_DATA;
					break;
				case 8:
					object->type = DOUBLE_DATA;
					break;
				default:
					//this shouldn't happen
					object->type = NULLTYPE_DATA;
					break;
			}
			break;
		case 6:
			object->complexity_flag = 1;
			msg_pointer = navigateTo(msg_address + 48, 20, TREE);
			readDataTypeMessage(object, msg_pointer, msg_address + 48, 20);
			object->num_elems *= 2;
			navigateTo(msg_address, msg_size, TREE);
			break;
		case 7:
			//reference (cell), data consists of addresses aka references
			object->type = REF_DATA;
			object->byte_order = LITTLE_ENDIAN;
			break;
		default:
			//ignore
			object->type = NULLTYPE_DATA;
			object->byte_order = LITTLE_ENDIAN;
			break;
	}
	
	
}

void readDataLayoutMessage(Data* object, byte* msg_pointer, uint64_t msg_address, uint16_t msg_size)
{
	//assume version 3
	if(*msg_pointer != 3)
	{
		readMXError("getmatvar:internalError", "Data layout version at address\n\n");
	}
	
	object->layout_class = (uint8_t)*(msg_pointer + 1);
	switch(object->layout_class)
	{
		case 0:
			object->data_address = msg_address + 4;
			object->data_pointer = msg_pointer + 4;
			break;
		case 1:
			object->data_address = getBytesAsNumber(msg_pointer + 2, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
			object->data_pointer = msg_pointer + (object->data_address - msg_address);
			break;
		case 2:
			object->chunked_info.num_chunked_dims = (uint8_t)(*(msg_pointer + 2) - 1); //??
			object->data_address = getBytesAsNumber(msg_pointer + 3, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
			object->data_pointer = msg_pointer + (object->data_address - msg_address);
			for(int j = 0; j < object->chunked_info.num_chunked_dims; j++)
			{
				object->chunked_info.chunked_dims[object->chunked_info.num_chunked_dims - j - 1] = (uint32_t)getBytesAsNumber(msg_pointer + 3 + s_block.size_of_offsets + 4 * j, 4, META_DATA_BYTE_ORDER);
			}
			object->chunked_info.chunked_dims[object->chunked_info.num_chunked_dims] = 0;
			object->chunked_info.num_chunked_elems = 1;
			for(int i = 0; i < object->chunked_info.num_chunked_dims; i++)
			{
				object->chunked_info.num_chunked_elems *= object->chunked_info.chunked_dims[i];
			}

			makeChunkedUpdates(object->chunked_info.chunk_update,
						    object->chunked_info.chunked_dims,
						    object->dims,
						    object->num_dims);

			break;
		default:
			readMXError("getmatvar:internalError", "Unknown data layout class\n\n");
	}
}

void readDataStoragePipelineMessage(Data* object, byte* msg_pointer, uint64_t msg_address, uint16_t msg_size)
{
	object->chunked_info.num_filters = (uint8_t)*(msg_pointer + 1);
	
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
				object->chunked_info.filters[j].client_data = malloc(object->chunked_info.filters[j].num_client_vals * sizeof(uint32_t));
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
				object->chunked_info.filters[j].client_data = malloc(object->chunked_info.filters[j].num_client_vals * sizeof(uint32_t));
				helper_pointer += 6;
				
				for(int k = 0; k < object->chunked_info.filters[j].num_client_vals; k++)
				{
					object->chunked_info.filters[j].client_data[k] = (uint32_t)getBytesAsNumber(helper_pointer, 4, META_DATA_BYTE_ORDER);
					helper_pointer += 4;
				}
				
			}
			
			break;
		default:
			readMXError("getmatvar:internalError", "Unknown data storage pipeline version\n\n");
			//fprintf(stderr, "Unknown data storage pipeline version %d at address 0x%llu.\n", *msg_pointer, msg_address);
			//exit(EXIT_FAILURE);
		
	}
}

void readAttributeMessage(Data* object, byte* msg_pointer, uint64_t msg_address, uint16_t msg_size)
{
	char name[NAME_LENGTH];
	uint16_t name_size = (uint16_t)getBytesAsNumber(msg_pointer + 2, 2, META_DATA_BYTE_ORDER);
	uint16_t datatype_size = (uint16_t)getBytesAsNumber(msg_pointer + 4, 2, META_DATA_BYTE_ORDER);
	uint16_t dataspace_size = (uint16_t)getBytesAsNumber(msg_pointer + 6, 2, META_DATA_BYTE_ORDER);
	strncpy(name, (char*)(msg_pointer + 8), name_size);
	if(strcmp(name, "MATLAB_class") == 0)
	{
		uint32_t attribute_data_size = (uint32_t)getBytesAsNumber(msg_pointer + 8 + roundUp(name_size) + 4, 4, META_DATA_BYTE_ORDER);
		strncpy(object->matlab_class, (char*)(msg_pointer + 8 + roundUp(name_size) + roundUp(datatype_size) + roundUp(dataspace_size)), attribute_data_size);
		object->matlab_class[attribute_data_size] = 0x0;
		if(strcmp("struct", object->matlab_class) == 0)
		{
			object->type = STRUCT_DATA;
		}
		else if(strcmp("cell", object->matlab_class) == 0)
		{
			object->type = REF_DATA;
		}
		else if(strcmp("function_handle", object->matlab_class) == 0)
		{
			object->type = FUNCTION_HANDLE_DATA;
		}
		else if(strcmp("table", object->matlab_class) == 0)
		{
			object->type = TABLE_DATA;
		}
	}
}



