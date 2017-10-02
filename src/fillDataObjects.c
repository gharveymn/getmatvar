#include "headers/fillDataObjects.h"
#include "headers/getDataObjects.h"


void fillVariable(char* variable_name)
{
	
	if(is_done == TRUE)
	{
		return;
	}
	
	char* delim = ".", * token;
	flushQueue(varname_queue);
	if(strcmp(variable_name, "\0") == 0)
	{
		for(int i = 0; i < virtual_super_object->num_sub_objs; i++)
		{
			if(virtual_super_object->sub_objects[i]->names.short_name[0] != '#')
			{
				fillDataTree(virtual_super_object->sub_objects[i]);
				enqueue(top_level_objects, virtual_super_object->sub_objects[i]);
			}
		}
		is_done = TRUE;
	}
	else
	{
		//TODO make this cleaner
		//copy over variable name before tokenizing so we can print variable not found output if needed
		char* variable_name_cpy = malloc((strlen(variable_name) + 1)* sizeof(char));
		strcpy(variable_name_cpy, variable_name);
		token = strtok(variable_name_cpy, delim);
		while(token != NULL)
		{
			char* vn = malloc((strlen(token) + 1)*sizeof(char));
			strcpy(vn, token);
			enqueue(varname_queue, vn);
			token = strtok(NULL, delim);
		}
		
		Data* object = virtual_super_object;
		do
		{
			object = findSubObjectByShortName(object, dequeue(varname_queue));
			if(object == NULL)
			{
				sprintf(warn_message, "Variable \'%s\' was not found.", variable_name);
				free(variable_name_cpy);
				readMXWarn("getmatvar:variableNotFound", warn_message);
				return;
			}
		} while(varname_queue->length > 0);
		
		fillDataTree(object);
		enqueue(top_level_objects, object);
		free(variable_name_cpy);
		
	}
	
}


/*fill this super object and all below it*/
void fillDataTree(Data* object)
{
	
	//main objective
	fillObject(object, object->this_obj_address);
	
	if(object->matlab_internal_type == mxFUNCTION_CLASS)
	{
		fillFunctionHandleData(object);
	}
	else
	{
		for(int i = 0; i < object->num_sub_objs; i++)
		{
			fillDataTree(object->sub_objects[i]);
		}
		object->is_finalized = TRUE;
	}
	
}


void fillObject(Data* object, uint64_t this_obj_address)
{
	
	if(object->is_filled == TRUE)
	{
		return;
	}
	
	object->is_filled = TRUE;
	
	byte* header_pointer = navigateTo(this_obj_address, 12);
	uint32_t header_length = (uint32_t)getBytesAsNumber(header_pointer + 8, 4, META_DATA_BYTE_ORDER);
	uint16_t num_msgs = (uint16_t)getBytesAsNumber(header_pointer + 2, 2, META_DATA_BYTE_ORDER);
	object->this_obj_address = this_obj_address;
	releasePages(this_obj_address, 12);
	
	collectMetaData(object, this_obj_address, num_msgs, header_length);
	
	if(object->hdf5_internal_type == HDF5_UNKNOWN && object->matlab_internal_type == mxUNKNOWN_CLASS)
	{
		error_flag = TRUE;
		sprintf(error_id, "getmatvar:unknownDataTypeError");
		sprintf(error_message, "Unknown data type encountered in the HDF5 file.\n\n");
		return;
	}
	
	//allocate space for data
	if(allocateSpace(object) != 0)
	{
		error_flag = TRUE;
		sprintf(error_id, "getmatvar:allocationError");
		sprintf(error_message, "Unknown error happened during allocation.\n\n");
		return;
	}
	
	
	//fetch data
	switch(object->layout_class)
	{
		case 0:
		case 1:
			//compact storage or contiguous storage
			//placeData will just segfault if it has an error, ie. if this encounters an error something is very wrong
			object->data_pointer = navigateTo(object->data_address, object->num_elems * object->elem_size);
			placeData(object, object->data_pointer, 0, object->num_elems, object->elem_size, object->byte_order);
			releasePages(object->data_address, object->num_elems * object->elem_size);
			break;
		case 2:
			//chunked storage
			if(getChunkedData(object) != 0)
			{
				return;
			}
			break;
		case 3:
			//don't do anything in the case of structs, functions, etc.
			break;
		default:
			error_flag = TRUE;
			sprintf(error_id, "getmatvar:unknownLayoutClassError");
			sprintf(error_message, "Unknown layout class encountered.\n\n");
			return;
	}
	
	// we have encountered a cell array
	if(object->data_arrays.sub_object_header_offsets != NULL && object->matlab_internal_type == mxCELL_CLASS)
	{
		object->sub_objects = malloc(object->num_elems * sizeof(Data*));
		object->num_sub_objs = object->num_elems;
		for(int i = object->num_elems - 1; i >= 0; i--)
		{
			address_t new_obj_address = object->data_arrays.sub_object_header_offsets[i] + s_block.base_address;
			//search from virtual_super_object since the reference might be in #refs#
			Data* ref = findObjectByHeaderAddress(new_obj_address);
			object->sub_objects[i] = ref;
			fillDataTree(ref);
			
			//get the number of digits in i + 1
			int n = i + 1;
			uint16_t num_digits = 0;
			do
			{
				n /= 10;
				num_digits++;
			} while(n != 0);
			
			free(ref->names.short_name);
			ref->names.short_name_length = (uint16_t)(object->names.short_name_length + 1 + num_digits + 1);
			ref->names.short_name = malloc((ref->names.short_name_length + 1) * sizeof(char));
			sprintf(ref->names.short_name, "%s{%d}", object->names.short_name, i+1);
			ref->names.short_name[ref->names.short_name_length] = '\0';
			
			free(ref->names.long_name);
			ref->names.long_name_length = (uint16_t)(object->names.long_name_length + 1 + num_digits + 1);
			ref->names.long_name = malloc((ref->names.long_name_length + 1) * sizeof(char));
			sprintf(ref->names.long_name, "%s{%d}", object->names.long_name, i+1);
			ref->names.long_name[ref->names.long_name_length] = '\0';
			
		}
	}
	
}


void collectMetaData(Data* object, uint64_t header_address, uint16_t num_msgs, uint32_t header_length)
{
	
	interpretMessages(object, header_address, header_length, 0, num_msgs, 0);
	
	if(object->matlab_internal_attributes.MATLAB_empty == TRUE)
	{
		object->num_elems = 0;
		for(int i = 0; i < object->num_dims; i++)
		{
			object->dims[i] = 0;
		}
		object->num_dims = 0;
		return;
	}
	
}


uint16_t interpretMessages(Data* object, uint64_t header_address, uint32_t header_length, uint16_t message_num, uint16_t num_msgs, uint16_t repeat_tracker)
{
	
	byte* header_pointer = navigateTo(header_address, header_length);
	
	uint64_t cont_header_address = UNDEF_ADDR;
	uint32_t cont_header_length = 0;
	
	uint16_t msg_type = 0;
	uint16_t msg_size = 0;
	uint64_t msg_address = 0;
	byte* msg_pointer = NULL;
	uint32_t bytes_read = 0;
	
	//interpret messages in header
	for(; message_num < num_msgs && bytes_read < header_length; message_num++)
	{
		msg_type = (uint16_t)getBytesAsNumber(header_pointer + 16 + bytes_read, 2, META_DATA_BYTE_ORDER);
		//msg_address = header_address + 16 + bytes_read;
		msg_size = (uint16_t)getBytesAsNumber(header_pointer + 16 + bytes_read + 2, 2, META_DATA_BYTE_ORDER);
		msg_pointer = header_pointer + 16 + bytes_read + 8;
		msg_address = header_address + 16 + bytes_read + 8;
		
		switch(msg_type)
		{
			case 1:
				// Dataspace message, not repeated
				if((repeat_tracker & (1 << msg_type)) == FALSE)
				{
					readDataSpaceMessage(object, msg_pointer, msg_address, msg_size);
					repeat_tracker |= 1 << msg_type;
				}
				break;
			case 3:
				// DataType message, not repeated
				if((repeat_tracker & (1 << msg_type)) == FALSE)
				{
					readDataTypeMessage(object, msg_pointer, msg_address, msg_size);
					repeat_tracker |= 1 << msg_type;
				}
				break;
			case 8:
				// Data Layout message, not repeated
				if((repeat_tracker & (1 << msg_type)) == FALSE)
				{
					readDataLayoutMessage(object, msg_pointer, msg_address, msg_size);
					repeat_tracker |= 1 << msg_type;
				}
				break;
			case 11:
				//data storage pipeline message, not repeated
				if((repeat_tracker & (1 << msg_type)) == FALSE)
				{
					readDataStoragePipelineMessage(object, msg_pointer, msg_address, msg_size);
					repeat_tracker |= 1 << msg_type;
				}
				break;
			case 12:
				//attribute message
				readAttributeMessage(object, msg_pointer, msg_address, msg_size);
				break;
			case 16:
				//object header continuation message
				//ie no info for the object
				cont_header_address = getBytesAsNumber(msg_pointer, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
				cont_header_length = (uint32_t)getBytesAsNumber(msg_pointer + s_block.size_of_offsets, s_block.size_of_lengths, META_DATA_BYTE_ORDER);
				message_num++;
				
				releasePages(header_address, header_length);
				message_num = interpretMessages(object, cont_header_address - 16, cont_header_length, message_num, num_msgs, repeat_tracker);
				header_pointer = navigateTo(header_address, header_length);
				break;
			default:
				//ignore message
				//case 17 -- B tree already traversed and in queue
				break;
		}
		
		bytes_read += msg_size + 8;
		
	}
	
	releasePages(header_address, header_length);
	return (uint16_t)(message_num - 1);
	
}

void fillFunctionHandleData(Data* fh)
{
	/*
	 * need to traverse structure:
	 *
	 * fh_data-
	 *    |-function_handle-
	 *    |                 |-file
	 *    |                 |-function		<-- NEED
	 *    |                 |-(within_file)
	 *    |                 |-type
	 *    |                 |-(workspace)
	 *    |-matlabroot
	 *    |-sentinel
	 *    |-separator
	 *
	 */
	
	// the object may be filled but not finalized, like in this case where
	// we have to retrieve some extra data from the substructure
	if(fh->is_finalized == TRUE)
	{
		return;
	}
	fh->is_finalized = TRUE;
	
	Data* function_handle = findSubObjectByShortName(fh, "function_handle");
	Data* fh_data = findSubObjectByShortName(function_handle, "function");
	fillObject(fh_data, fh_data->this_obj_address);
	fh->elem_size = fh_data->elem_size;
	
	//dont use strchr because we need to know the length of the copied string
	uint32_t fh_len = fh_data->num_elems;
	char* func_name = NULL;
	for(; fh_len > 0; fh_len--)
	{
		func_name = (char*)(fh_data->data_arrays.data + (fh_data->num_elems - fh_len) * sizeof(uint16_t));
		if(*func_name == '@')
		{
			break;
		}
	}
	
	if(fh_len == 0)
	{
		//need to add the '@' character
		fh->data_arrays.data = mxMalloc((1 + fh_data->num_elems) * fh_data->elem_size);
		uint16_t seperator = 64;
		memcpy(fh->data_arrays.data, &seperator, sizeof(uint16_t)); //this is the '@' character
		memcpy(fh->data_arrays.data + sizeof(uint16_t), fh_data->data_arrays.data, fh_data->num_elems*fh_data->elem_size);
		fh->num_elems = 1 + fh_data->num_elems; //+ 1 for the seperator
	}
	else
	{
		fh->data_arrays.data = mxMalloc((fh_len) * fh_data->elem_size);
		memcpy(fh->data_arrays.data, func_name, fh_len * fh_data->elem_size);
		fh->num_elems = fh_len;
	}
	
	fh->num_dims = 2;
	fh->dims[0] = 1;
	fh->dims[1] = fh->num_elems;
	fh->dims[2] = 0;
	
}

