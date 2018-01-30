#include "headers/fillDataObjects.h"


int fillVariable(char* variable_name)
{
	
	if(is_done == TRUE)
	{
		return 0;
	}
	
	if(strcmp(variable_name, "\0") == 0)
	{
		for(int i = 0; i < virtual_super_object->num_sub_objs; i++)
		{
			Data* obj = dequeue(virtual_super_object->sub_objects);
			if(obj->names.short_name[0] != '#')
			{
				fillDataTree(obj);
				enqueue(top_level_objects, obj);
			}
		}
		restartQueue(virtual_super_object->sub_objects);
		is_done = TRUE;
	}
	else
	{
		
		makeVarnameQueue(variable_name);
		
		Data* object = virtual_super_object;
		do
		{
			VariableNameToken* varname_token = dequeue(varname_queue);
			
			switch(varname_token->variable_name_type)
			{
				case VT_LOCAL_NAME:
					object = findSubObjectByShortName(object, varname_token->variable_local_name);
					if(object == NULL)
					{
						sprintf(warn_message, "Variable \'%s\' was not found.\n", variable_name);
						readMXError("getmatvar:variableNotFound", warn_message);
						return 1;
					}
					
					if(fillObject(object, object->this_obj_address) != 0)
					{
						return 1;
					}
					
					if(object->data_flags.is_struct_array == TRUE)
					{
						sprintf(warn_message, "Variable \'%s\' was not found.\n", variable_name);
						readMXError("getmatvar:variableNotFound", warn_message);
						return 1;
					}
					break;
				case VT_LOCAL_INDEX:
					
					if(fillObject(object, object->this_obj_address) != 0)
					{
						return 1;
					}
					
					
					if(object->matlab_internal_type == mxSTRUCT_CLASS)
					{
						
						if(varname_queue->length == 0)
						{
							sprintf(warn_message, "Selection of array elements is not supported. Could not return \'%s\'.\n", variable_name);
							readMXError("getmatvar:arrayElementError", warn_message);
							return 1;
						}
						
						//matlab reads struct arrays as my_struct(3).dog
						//but stores them as my_struct.dog(3)
						VariableNameToken* field = dequeue(varname_queue);
						object = findSubObjectByShortName(object, field->variable_local_name);
						if(object == NULL)
						{
							sprintf(warn_message, "Variable \'%s\' was not found.\n", variable_name);
							readMXError("getmatvar:variableNotFound", warn_message);
							return 1;
						}
						
						if(fillObject(object, object->this_obj_address) != 0)
						{
							return 1;
						}
						
						//does not execute if the struct is 1x1
						if(object->data_flags.is_struct_array == TRUE)
						{
							object = findSubObjectBySCIndex(object, varname_token->variable_local_index);
							if(object == NULL)
							{
								sprintf(warn_message, "Variable \'%s\' was not found.\n", variable_name);
								readMXError("getmatvar:variableNotFound", warn_message);
								return 1;
							}
						}
						
					}
					else
					{
						object = findSubObjectBySCIndex(object, varname_token->variable_local_index);
						if(object == NULL)
						{
							sprintf(warn_message, "Variable \'%s\' was not found.\n", variable_name);
							readMXError("getmatvar:variableNotFound", warn_message);
							return 1;
						}
					}
					break;
				case VT_LOCAL_COORDINATES:
					
					if(fillObject(object, object->this_obj_address) != 0)
					{
						return 1;
					}
					
					if(object->matlab_internal_type == mxSTRUCT_CLASS)
					{
						
						if(varname_queue->length == 0)
						{
							sprintf(warn_message, "Selection of array elements is not supported. Could not return \'%s\'.\n", variable_name);
							readMXError("getmatvar:arrayElementError", warn_message);
							return 1;
						}
						
						//matlab reads struct arrays as my_struct(3).dog
						//but stores them as my_struct.dog(3)
						
						VariableNameToken* field = dequeue(varname_queue);
						object = findSubObjectByShortName(object, field->variable_local_name);
						if(object == NULL)
						{
							sprintf(warn_message, "Variable \'%s\' was not found.\n", variable_name);
							readMXError("getmatvar:variableNotFound", warn_message);
							return 1;
						}
						
						if(fillObject(object, object->this_obj_address) != 0)
						{
							return 1;
						}
						
						if(object->data_flags.is_struct_array == TRUE)
						{
							varname_token->variable_local_index = coordToInd(
									varname_token->variable_local_coordinates, object->dims, object->num_dims);
							object = findSubObjectBySCIndex(object, varname_token->variable_local_index);
							if(object == NULL)
							{
								sprintf(warn_message, "Variable \'%s\' was not found.\n", variable_name);
								readMXError("getmatvar:variableNotFound", warn_message);
								return 1;
							}
						}
						
					}
					else
					{
						varname_token->variable_local_index = coordToInd(
								varname_token->variable_local_coordinates, object->dims, object->num_dims);
						object = findSubObjectBySCIndex(object, varname_token->variable_local_index);
						if(object == NULL)
						{
							sprintf(warn_message, "Variable \'%s\' was not found.\n", variable_name);
							readMXError("getmatvar:variableNotFound", warn_message);
							return 1;
						}
					}
					break;
				default:
					break;
			}
			
		} while(varname_queue->length > 0);
		
		if(fillDataTree(object) != 0)
		{
			return 1;
		}
		
		size_t n = top_level_objects->length;
		uint16_t num_digits = 0;
		do
		{
			n /= 10;
			num_digits++;
		} while(n != 0);
		
		if(object->names.short_name_length != 0)
		{
			free(object->names.short_name);
			object->names.short_name = NULL;
			object->names.short_name_length = 0;
		}
		
		if(object->names.long_name_length != 0)
		{
			free(object->names.long_name);
			object->names.long_name = NULL;
			object->names.long_name_length = 0;
		}
		
		object->names.short_name_length = (uint16_t)SELECTION_SIG_LEN + num_digits;
		object->names.short_name = malloc((object->names.short_name_length + 1)*sizeof(char));
		sprintf(object->names.short_name, "%s%d", SELECTION_SIG, (int)top_level_objects->length);
		
		object->names.long_name_length = (uint16_t)SELECTION_SIG_LEN + num_digits;
		object->names.long_name = malloc((object->names.long_name_length + 1)*sizeof(char));
		sprintf(object->names.long_name, "%s%d", SELECTION_SIG, (int)top_level_objects->length);
		
		enqueue(top_level_objects, object);
		
	}
	
	return 0;
	
}


/*fill this super object and all below it*/
int fillDataTree(Data* object)
{
	
	if(fillObject(object, object->this_obj_address) != 0)
	{
		return 1;
	}
	
	for(int i = 0; i < object->num_sub_objs; i++)
	{
		Data* obj = dequeue(object->sub_objects);
		if(fillDataTree(obj) != 0)
		{
			return 1;
		}
	}
	restartQueue(object->sub_objects);
	//object->data_flags.is_finalized = TRUE;
	
	return 0;
	
}


int fillObject(Data* object, uint64_t this_obj_address)
{
	
	if(object->data_flags.is_filled == TRUE)
	{
		return 0;
	}
	
	object->data_flags.is_filled = TRUE;
	
	mapObject* header_map_obj = st_navigateTo(this_obj_address, 12);
	byte* header_pointer = header_map_obj->address_ptr;
	uint32_t header_length = (uint32_t)getBytesAsNumber(header_pointer + 8, 4, META_DATA_BYTE_ORDER);
	uint16_t num_msgs = (uint16_t)getBytesAsNumber(header_pointer + 2, 2, META_DATA_BYTE_ORDER);
	object->this_obj_address = this_obj_address;
	st_releasePages(header_map_obj);
	
	//aligned on 8-byte boundaries, so msgs start at +16
	collectMetaData(object, this_obj_address + 16, num_msgs, header_length);
	
	if(object->matlab_internal_attributes.MATLAB_empty == TRUE)
	{
		return 0;
	}
	
	if(object->hdf5_internal_type == HDF5_UNKNOWN && object->matlab_internal_type == mxUNKNOWN_CLASS)
	{
		error_flag = TRUE;
		sprintf(error_id, "getmatvar:unknownDataTypeError");
		sprintf(error_message, "Unknown data type encountered in the HDF5 file.\n\n");
		return 1;
	}
	
	//allocate space for data
	if(allocateSpace(object) != 0)
	{
		return 1;
	}
	
	
	//fetch data
	mapObject* data_map_obj;
	switch(object->layout_class)
	{
		case 0:
		case 1:
			//compact storage or contiguous storage
			//placeData will just segfault if it has an error, ie. if this encounters an error something is very wrong
			data_map_obj = st_navigateTo(object->data_address, object->num_elems * object->elem_size);
			placeData(object, data_map_obj->address_ptr, 0, 0, object->num_elems, object->elem_size, object->byte_order);
			st_releasePages(data_map_obj);
			break;
		case 2:
			//chunked storage
			if(getChunkedData(object) != 0)
			{
				return 1;
			}
			break;
		case 3:
			//don't do anything in the case of structs, functions, etc.
			break;
		default:
			error_flag = TRUE;
			sprintf(error_id, "getmatvar:unknownLayoutClassError");
			sprintf(error_message, "Unknown layout class encountered.\n\n");
			return 1;
	}
	
	// we have encountered a cell array
	if(object->data_arrays.sub_object_header_offsets != NULL && object->hdf5_internal_type == HDF5_REFERENCE)
	{
		flushQueue(object->sub_objects);
		object->num_sub_objs = (uint32_t)object->num_elems;
		for(uint64_t i = 0; i < object->num_elems; i++)
		{
			address_t new_obj_address = object->data_arrays.sub_object_header_offsets[i] + s_block.base_address;
			//search from virtual_super_object since the reference might be in #refs#
			Data* ref = findObjectByHeaderAddress(new_obj_address);
			while(object->sub_objects->length > 0)
			{
				Data* obj = dequeue(object->sub_objects);
				if(obj == ref)
				{
					ref = cloneData(obj);
				}
			}
			restartQueue(object->sub_objects);
			ref->s_c_array_index = i;
			enqueue(object->sub_objects, ref);
			
			//get the number of digits in i + 1
			int n = (int)i + 1;
			uint16_t num_digits = 0;
			do
			{
				n /= 10;
				num_digits++;
			} while(n != 0);
			
			if(ref->names.short_name_length != 0)
			{
				free(ref->names.short_name);
				ref->names.short_name = NULL;
				ref->names.short_name_length = 0;
			}
			
			if(ref->names.long_name_length != 0)
			{
				free(ref->names.long_name);
				ref->names.long_name = NULL;
				ref->names.long_name_length = 0;
			}
			
			if(object->data_flags.is_struct_array == FALSE)
			{
				//make names for cells, ie. blah{k}
				ref->names.short_name_length = (uint16_t)(num_digits + 1);
				ref->names.short_name = malloc((ref->names.short_name_length + 1)*sizeof(char));
				sprintf(ref->names.short_name, "%d}", (int)i + 1);
				ref->names.short_name[ref->names.short_name_length] = '\0';
				
				ref->names.long_name_length = (uint16_t)(object->names.long_name_length + 1 + num_digits + 1);
				ref->names.long_name = malloc((ref->names.long_name_length + 1)*sizeof(char));
				sprintf(ref->names.long_name, "%s{%d}", object->names.long_name, (int)i + 1);
				ref->names.long_name[ref->names.long_name_length] = '\0';
			}
			else
			{
				//make names for struct array, ie. blah(k).bleh
				//make names for cells, ie. blah{k}
//				free(ref->names.short_name);
//				ref->names.short_name_length = (uint16_t)(num_digits + 1 + 1 + object->names.short_name_length);
//				ref->names.short_name = malloc((ref->names.short_name_length + 1)*sizeof(char));
//				sprintf(ref->names.short_name, "%d).%s", i + 1, object->names.short_name);
//				ref->names.short_name[ref->names.short_name_length] = '\0';
				
				ref->names.short_name_length = (uint16_t)(object->names.short_name_length);
				ref->names.short_name = malloc((ref->names.short_name_length + 1)*sizeof(char));
				sprintf(ref->names.short_name, "%s", object->names.short_name);
				ref->names.short_name[ref->names.short_name_length] = '\0';
				
				ref->names.long_name_length = (uint16_t)(object->super_object->names.long_name_length + 1 + num_digits + 1 + 1 + object->names.short_name_length);
				ref->names.long_name = malloc((ref->names.long_name_length + 1)*sizeof(char));
				sprintf(ref->names.long_name, "%s(%d).%s", object->super_object->names.long_name, (int)i + 1, object->names.short_name);
				ref->names.long_name[ref->names.long_name_length] = '\0';
			}
			
			ref->super_object = object;
			
		}
	}
	
	return 0;
	
}


//TODO add error checking here for corrupted data
void collectMetaData(Data* object, uint64_t header_address, uint16_t num_msgs, uint32_t header_length)
{
	
	interpretMessages(object, header_address, header_length, 0, num_msgs, 0);
	
	if(object->matlab_internal_attributes.MATLAB_empty == TRUE)
	{
		object->num_elems = 0;
		if(object->dims != NULL)
		{
			free(object->dims);
		}
		object->dims = malloc(2*sizeof(uint64_t));
		object->dims[0] = 0;
		object->num_dims = 0;
		return;
	}
	
	if(object->hdf5_internal_type == HDF5_REFERENCE && object->matlab_internal_type == mxUNKNOWN_CLASS && object->super_object->matlab_internal_type == mxSTRUCT_CLASS)
	{
		object->data_flags.is_struct_array = TRUE;
		//pretend this is a cell
		object->matlab_internal_type = mxCELL_CLASS;
		object->super_object->num_dims = object->num_dims;
		if(object->super_object->dims != NULL)
		{
			free(object->super_object->dims);
		}
		object->super_object->dims = malloc((object->num_dims + 1)*sizeof(uint64_t));
		memcpy(object->super_object->dims, object->dims, object->num_dims *sizeof(uint64_t));
		object->super_object->dims[object->num_dims] = 0;
	}
	
	if(object->matlab_internal_attributes.MATLAB_sparse == TRUE)
	{
		object->matlab_sparse_type = object->matlab_internal_type;
		object->matlab_internal_type = mxSPARSE_CLASS;
	}
	else if(object->super_object->matlab_internal_attributes.MATLAB_sparse == TRUE)
	{
		object->matlab_internal_type = mxUINT64_CLASS;
	}
	
}


uint16_t interpretMessages(Data* object, uint64_t msgs_start_address, uint32_t msgs_length, uint16_t message_num, uint16_t num_msgs, uint16_t repeat_tracker)
{
	
	mapObject* msgs_start_map_obj = st_navigateTo(msgs_start_address, msgs_length);
	byte* msgs_start_pointer = msgs_start_map_obj->address_ptr;
	
	uint64_t cont_header_address = UNDEF_ADDR;
	uint32_t cont_header_length = 0;
	
	uint16_t msg_type = 0;
	uint16_t msg_size = 0;
	uint64_t msg_address = 0;
	byte* msg_pointer = NULL;
	uint32_t bytes_read = 0;
	uint8_t msg_sub_header_size = 8;
	
	//interpret messages in header
	for(; message_num < num_msgs && bytes_read < msgs_length; message_num++)
	{
		msg_type = (uint16_t)getBytesAsNumber(msgs_start_pointer + bytes_read, 2, META_DATA_BYTE_ORDER);
		//msg_address = msgs_start_address + bytes_read;
		msg_size = (uint16_t)getBytesAsNumber(msgs_start_pointer + bytes_read + 2, 2, META_DATA_BYTE_ORDER);
		msg_pointer = msgs_start_pointer + bytes_read + msg_sub_header_size;
		msg_address = msgs_start_address + bytes_read + msg_sub_header_size;
		
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
				
				st_releasePages(msgs_start_map_obj);
				message_num = interpretMessages(object, cont_header_address, cont_header_length, message_num, num_msgs, repeat_tracker);
				msgs_start_map_obj = st_navigateTo(msgs_start_address, msgs_length);
				msgs_start_pointer = msgs_start_map_obj->address_ptr;
				break;
			default:
				//ignore message
				//case 17 -- B tree already traversed and in queue
				break;
		}
		
		bytes_read += msg_size + msg_sub_header_size;
		
	}
	
	st_releasePages(msgs_start_map_obj);
	return (uint16_t)(message_num - 1);
	
}

