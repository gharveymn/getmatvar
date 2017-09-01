#include "mapping.h"

void readDataSpaceMessage(Data* object, char* msg_pointer, uint64_t msg_address, uint16_t msg_size)
{
	//assume version 1 and ignore max dims and permutation indices
	object->num_dims = (uint8_t)*(msg_pointer + 1);
	uint32_t* dims = malloc(sizeof(int) * (object->num_dims + 1));
	//uint64_t bytes_read = 0;
	
	for(int i = 0; i < object->num_dims; i++)
	{
		dims[i] = (uint32_t)getBytesAsNumber(msg_pointer + 8 + i * s_block.size_of_lengths, 4, META_DATA_BYTE_ORDER);
	}
	dims[object->num_dims] = 0;
	
	object->dims = dims;
	
	object->num_elems = 1;
	int index = 0;
	while(object->dims[index] > 0)
	{
		object->num_elems *= object->dims[index];
		index++;
	}
	
}

void readDataTypeMessage(Data* object, char* msg_pointer, uint64_t msg_address, uint16_t msg_size)
{
	object->type = UNDEF;
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
						object->type = INT8;
					}
					else
					{
						object->type = CHAR;
					}
					break;
				case 2:
					if(sign)
					{
						object->type = INT16;
					}
					else
					{
						object->type = UINT16;
					}
					break;
				case 4:
					if(sign)
					{
						object->type = INT32;
					}
					else
					{
						object->type = UINT32;
					}
					break;
				case 8:
					if(sign)
					{
						object->type = INT64;
					}
					else
					{
						object->type = UINT64;
					}
					break;
				default:
					//this shouldn't happen
					object->type = UNDEF;
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
					object->type = SINGLE;
					break;
				case 8:
					object->type = DOUBLE;
					break;
				default:
					//this shouldn't happen
					object->type = UNDEF;
					break;
			}
			break;
		case 7:
			//reference (cell), data consists of addresses aka references
			object->type = REF;
			object->byte_order = LITTLE_ENDIAN;
			break;
		default:
			//ignore
			object->type = UNDEF;
			object->byte_order = LITTLE_ENDIAN;
			break;
	}
	
	
}

char* readDataLayoutMessage(Data* object, char* msg_pointer, uint64_t msg_address, uint16_t msg_size)
{
	//assume version 3
	if(*msg_pointer != 3)
	{
		printf("Data layout version at address 0x%llu is %d; expected version 3.\n", msg_address, *msg_pointer);
		exit(EXIT_FAILURE);
	}
	char* data_pointer = NULL;
	
	object->layout_class = (uint8_t)*(msg_pointer + 1);
	switch(object->layout_class)
	{
		case 0:
			object->data_address = msg_address + 4;
			data_pointer = msg_pointer + 4;
			break;
		case 1:
			object->data_address = getBytesAsNumber(msg_pointer + 2, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
			data_pointer = msg_pointer + (object->data_address - msg_address);
			break;
		case 2:
			object->chunked_info.num_chunked_dims = (uint8_t)(*(msg_pointer + 2) - 1); //??
			object->chunked_info.chunked_dims = malloc((object->chunked_info.num_chunked_dims + 1) * sizeof(uint32_t));
			object->data_address = getBytesAsNumber(msg_pointer + 3, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
			data_pointer = msg_pointer + (object->data_address - msg_address);
			for(int j = 0; j < object->chunked_info.num_chunked_dims; j++)
			{
				object->chunked_info.chunked_dims[j] = (uint32_t)getBytesAsNumber(msg_pointer + 3 + s_block.size_of_offsets + 4 * j, 4, META_DATA_BYTE_ORDER);
			}
			object->chunked_info.chunked_dims[object->chunked_info.num_chunked_dims] = 0;
			break;
		default:
			printf("Unknown data layout class %d at address 0x%llu.\n", object->layout_class, msg_address + 1);
			exit(EXIT_FAILURE);
	}
	
	return data_pointer;
}

void readDataStoragePipelineMessage(Data* object, char* msg_pointer, uint64_t msg_address, uint16_t msg_size)
{
	object->chunked_info.num_filters = (uint8_t)*(msg_pointer + 1);
	
	char* helper_pointer = NULL;
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
			printf("Unknown data storage pipeline version %d at address 0x%llu.\n", *msg_pointer, msg_address);
			exit(EXIT_FAILURE);
		
	}
}

void readAttributeMessage(Data* object, char* msg_pointer, uint64_t msg_address, uint16_t msg_size)
{
	char name[NAME_LENGTH];
	uint16_t name_size = (uint16_t)getBytesAsNumber(msg_pointer + 2, 2, META_DATA_BYTE_ORDER);
	uint16_t datatype_size = (uint16_t)getBytesAsNumber(msg_pointer + 4, 2, META_DATA_BYTE_ORDER);
	uint16_t dataspace_size = (uint16_t)getBytesAsNumber(msg_pointer + 6, 2, META_DATA_BYTE_ORDER);
	strncpy(name, msg_pointer + 8, name_size);
	if(strcmp(name, "MATLAB_class") == 0)
	{
		uint32_t attribute_data_size = (uint32_t)getBytesAsNumber(msg_pointer + 8 + roundUp(name_size) + 4, 4, META_DATA_BYTE_ORDER);
		strncpy(object->matlab_class, msg_pointer + 8 + roundUp(name_size) + roundUp(datatype_size) + roundUp(dataspace_size), attribute_data_size);
		object->matlab_class[attribute_data_size] = 0x0;
		if(strcmp("struct", object->matlab_class) == 0)
		{
			object->type = STRUCT;
		}
		else if(strcmp("function_handle", object->matlab_class) == 0)
		{
			object->type = FUNCTION_HANDLE;
		}
	}
}



