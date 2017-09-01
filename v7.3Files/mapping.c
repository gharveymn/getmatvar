#include "mapping.h"


Data* findDataObject(const char* filename, const char variable_name[])
{
	Data** objects = getDataObjects(filename, variable_name);
	Data* hi_objects = organizeObjects(objects);
	
	//free the end object because we aren't going to be using the linear object freeing system
	int i = 0;
	while(objects[i]->type != UNDEF)
	{
		i++;
	}
	free(objects[i]);
	return hi_objects;
}

Data** getDataObjects(const char* filename, const char variable_name[])
{
	char* header_pointer;
	uint32_t header_length;
	uint64_t header_address;
	//uint64_t root_tree_address;
	Data** data_objects_ptrs = malloc((MAX_OBJS + 1) * sizeof(Data*));
	Object obj;
	
	//init maps
	maps[0].used = FALSE;
	maps[1].used = FALSE;
	
	//init queue
	flushQueue();
	flushHeaderQueue();
	
	//open the file descriptor
	fd = open(filename, O_RDWR);
	if(fd < 0)
	{
		printf("open() unsuccessful, Check errno: %d\n", errno);
		exit(EXIT_FAILURE);
	}
	
	//get file size
	size_t file_size = (size_t)lseek(fd, 0, SEEK_END);
	
	//find superblock
	s_block = getSuperblock(fd, file_size);
	
	//root_tree_address = queue.trios[queue.front].tree_address;
	
	//printf("\nObject header for variable %s is at 0x", variable_name);
	findHeaderAddress(variable_name);
	//printf("%llu\n", header_queue.objects[header_queue.front].this_obj_header_address);
	
	
	//interpret the header messages
	int num_objs = 0;
	while(header_queue.length > 0)
	{
		
		data_objects_ptrs[num_objs] = malloc(sizeof(Data));
		
		//initialize elements subject to malloc
		data_objects_ptrs[num_objs]->type = UNDEF;
		data_objects_ptrs[num_objs]->double_data = NULL;
		data_objects_ptrs[num_objs]->udouble_data = NULL;
		data_objects_ptrs[num_objs]->char_data = NULL;
		data_objects_ptrs[num_objs]->ushort_data = NULL;
		data_objects_ptrs[num_objs]->sub_objects = NULL;
		data_objects_ptrs[num_objs]->chunked_info.chunked_dims = NULL;
		
		obj = dequeueObject();
		header_address = obj.this_obj_header_address;
		
		//by only asking for enough bytes to get the header length there is a chance a mapping can be reused
		header_pointer = navigateTo(header_address, 16, TREE);
		
		//prevent error due to crossing of a page boundary
		header_length = (uint32_t)getBytesAsNumber(header_pointer + 8, 4, META_DATA_BYTE_ORDER);
		if(header_address + header_length >= maps[TREE].offset + maps[TREE].bytes_mapped)
		{
			header_pointer = navigateTo(header_address, header_length, TREE);
		}
		strcpy(data_objects_ptrs[num_objs]->name, obj.name);
		data_objects_ptrs[num_objs]->parent_obj_address = obj.parent_obj_header_address;
		data_objects_ptrs[num_objs]->this_obj_address = obj.this_obj_header_address;
		collectMetaData(data_objects_ptrs[num_objs], header_address, header_pointer);
		
		num_objs++;
		
	}
	
	//set object at the end to trigger sentinel
	data_objects_ptrs[num_objs] = malloc(sizeof(Data));
	data_objects_ptrs[num_objs]->type = UNDEF;
	data_objects_ptrs[num_objs]->parent_obj_address = UNDEF_ADDR;
	
	close(fd);
	return data_objects_ptrs;
}


void collectMetaData(Data* object, uint64_t header_address, char* header_pointer)
{
	
	uint16_t num_msgs = (uint16_t)getBytesAsNumber(header_pointer + 2, 2, META_DATA_BYTE_ORDER);
	
	uint8_t layout_class = 0;
	uint16_t name_size, datatype_size, dataspace_size;
	uint16_t msg_type = 0;
	uint16_t msg_size = 0;
	uint32_t attribute_data_size, header_length;
	uint64_t msg_address = 0;
	char* msg_pointer, * data_pointer = 0;
	int index, num_elems = 1;
	char name[NAME_LENGTH];
	object->elem_size = 0;
	
	char* helper_pointer;
	
	int32_t bytes_read = 0;
	
	//interpret messages in header
	for(int i = 0; i < num_msgs; i++)
	{
		msg_type = (uint16_t)getBytesAsNumber(header_pointer + 16 + bytes_read, 2, META_DATA_BYTE_ORDER);
		msg_address = header_address + 16 + bytes_read;
		msg_size = (uint16_t)getBytesAsNumber(header_pointer + 16 + bytes_read + 2, 2, META_DATA_BYTE_ORDER);
		msg_pointer = header_pointer + 16 + bytes_read + 8;
		msg_address = header_address + 16 + bytes_read + 8;
		
		switch(msg_type)
		{
			case 1:
				// Dataspace message
				object->dims = readDataSpaceMessage(msg_pointer, msg_size);
				
				index = 0;
				while(object->dims[index] > 0)
				{
					num_elems *= object->dims[index];
					index++;
				}
				break;
			case 3:
				// DataType message
				object->type = readDataTypeMessage(msg_pointer, msg_size);
				object->datatype_bit_field = (uint32_t)getBytesAsNumber(msg_pointer + 1, 3, META_DATA_BYTE_ORDER);
				switch(object->type)
				{
					case CHAR:
					case UNSIGNEDINT16:
					case UNDEF:
					case DOUBLE:
						//assume that the bytes wont be VAX-endian because I don't know what that is
						object->byte_order = (ByteOrder)(object->datatype_bit_field & 1);
						break;
					default:
						object->byte_order = LITTLE_ENDIAN;
						break;
				}
				break;
			case 8:
				// Data Layout message
				//assume version 3
				if(*msg_pointer != 3)
				{
					printf("Data layout version at address 0x%llu is %d; expected version 3.\n", msg_address, *msg_pointer);
					exit(EXIT_FAILURE);
				}
				
				layout_class = (uint8_t)*(msg_pointer + 1);
				switch(layout_class)
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
						printf("Unknown data layout class %d at address 0x%llu.\n", layout_class, msg_address + 1);
						exit(EXIT_FAILURE);
				}
				break;
			case 11:
				//data storage pipeline message
				
				object->chunked_info.num_filters = (uint8_t)*(msg_pointer + 1);
				
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
			case 12:
				//attribute message
				name_size = (uint16_t)getBytesAsNumber(msg_pointer + 2, 2, META_DATA_BYTE_ORDER);
				datatype_size = (uint16_t)getBytesAsNumber(msg_pointer + 4, 2, META_DATA_BYTE_ORDER);
				dataspace_size = (uint16_t)getBytesAsNumber(msg_pointer + 6, 2, META_DATA_BYTE_ORDER);
				strncpy(name, msg_pointer + 8, name_size);
				if(strcmp(name, "MATLAB_class") == 0)
				{
					attribute_data_size = (uint32_t)getBytesAsNumber(msg_pointer + 8 + roundUp(name_size) + 4, 4, META_DATA_BYTE_ORDER);
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
				break;
			case 16:
				//object header continuation message
				header_address = getBytesAsNumber(msg_pointer, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
				header_length = (uint32_t)getBytesAsNumber(msg_pointer + s_block.size_of_offsets, s_block.size_of_lengths, META_DATA_BYTE_ORDER);
				header_pointer = navigateTo(header_address - 16, header_length + 16, TREE);
				header_address -= 16;
				bytes_read = 0 - msg_size - 8;
			default:
				//ignore message
				//case 17 -- B tree already traversed and in queue
				;
		}
		
		bytes_read += msg_size + 8;
	}
	
	//allocate space for data
	switch(object->type)
	{
		case DOUBLE:
			object->double_data = malloc(num_elems * sizeof(double));
			object->elem_size = sizeof(double);
			break;
		case UNSIGNEDINT16:
			object->ushort_data = malloc(num_elems * sizeof(uint16_t));
			object->elem_size = sizeof(uint16_t);
			break;
		case REF:
			//STORE ADDRESSES IN THE UDOUBLE_DATA ARRAY; THESE ARE NOT NOT ACTUAL ELEMENTS
			object->udouble_data = malloc(num_elems * sizeof(uint64_t));
			object->elem_size = sizeof(uint64_t);
			break;
		case CHAR:
			object->char_data = malloc(num_elems * sizeof(char));
			object->elem_size = sizeof(char);
			break;
		case STRUCT:
		case FUNCTION_HANDLE:
			object->dims = malloc(sizeof(int) * 3);
			object->dims[0] = 1;
			object->dims[1] = 1;
			object->dims[2] = 0;
			break;
		default:
			printf("Unknown data type encountered with header at address 0x%llu\n", header_address);
			exit(EXIT_FAILURE);
	}
	
	//fetch data
	switch(layout_class)
	{
		case 0:
		case 1:
			//compact storage or contiguous storage
			for(int j = 0; j < num_elems; j++)
			{
				if(object->double_data != NULL)
				{
					object->double_data[j] = convertHexToFloatingPoint(getBytesAsNumber(data_pointer + j * object->elem_size, object->elem_size, object->byte_order));
				}
				else if(object->ushort_data != NULL)
				{
					object->ushort_data[j] = (uint16_t)getBytesAsNumber(data_pointer + j * object->elem_size, object->elem_size, object->byte_order);
				}
				else if(object->udouble_data != NULL)
				{
					//these are addresses so we have to add the offset
					object->udouble_data[j] = getBytesAsNumber(data_pointer + j * object->elem_size, object->elem_size, object->byte_order) + s_block.base_address;
				}
				else if(object->char_data != NULL)
				{
					object->char_data[j] = (char)getBytesAsNumber(data_pointer + j * object->elem_size, object->elem_size, object->byte_order);
				}
			}
			break;
		case 2:
			//chunked storage
			getChunkedData(object);
			break;
	}
	
	//if we have encountered a cell array, queue up headers for its elements
	if(object->udouble_data != NULL && object->type == REF)
	{
		for(int i = num_elems - 1; i >= 0; i--)
		{
			Object obj;
			obj.this_obj_header_address = object->udouble_data[i];
			obj.parent_obj_header_address = object->this_obj_address;
			strcpy(obj.name, object->name);
			priorityEnqueueObject(obj);
		}
	}
}


void findHeaderAddress(const char variable_name[])
{
	char* delim = ".";
	char* tree_pointer;
	char* heap_pointer;
	char* token;
	variable_found = FALSE;
	
	default_bytes = (uint64_t)getAllocGran();
	
	flushVariableNameQueue();
	token = strtok(strdup(variable_name), delim);
	while(token != NULL)
	{
		enqueueVariableName(token);
		token = strtok(NULL, delim);
	}
	
	//aka the parent address for a snod
	Addr_Trio parent_trio = {.parent_obj_header_address = UNDEF_ADDR, .tree_address = UNDEF_ADDR, .heap_address = UNDEF_ADDR};
	Addr_Trio this_trio;
	
	//search for the object header for the variable
	while(queue.length > 0)
	{
		tree_pointer = navigateTo(queue.trios[queue.front].tree_address, default_bytes, TREE);
		heap_pointer = navigateTo(queue.trios[queue.front].heap_address, default_bytes, HEAP);
		assert(strncmp("HEAP", heap_pointer, 4) == 0);
		
		if(strncmp("TREE", tree_pointer, 4) == 0)
		{
			readTreeNode(tree_pointer);
			parent_trio = dequeueTrio();
		}
		else if(strncmp("SNOD", tree_pointer, 4) == 0)
		{
			this_trio = dequeueTrio();
			readSnod(tree_pointer, heap_pointer, parent_trio, this_trio);
		}
	}
	//printf("0x%lx\n", header_address);
}


Data* organizeObjects(Data** objects)
{
	
	if(objects[0]->type == UNDEF)
	{
		return NULL;
	}
	
	int num_total_objs = 0;
	while(objects[num_total_objs]->type != UNDEF)
	{
		num_total_objs++;
	}
	
	int index = 0;
	Data* super_object = objects[0];
	index++;
	
	if(super_object[0].type == STRUCT || super_object[0].type == REF)
	{
		placeInSuperObject(super_object, objects, num_total_objs, &index);
	}
	
	return super_object;
	
}


void placeInSuperObject(Data* super_object, Data** objects, int num_total_objs, int* index)
{
	//note: index should be at the starting index of the subobjects
	
	uint8_t num_curr_level_objs = 0;
	int idx = *index;
	Data** sub_objects = malloc((num_total_objs - idx) * sizeof(Data*));
	
	while(super_object->this_obj_address == objects[idx]->parent_obj_address)
	{
		sub_objects[num_curr_level_objs] = objects[idx];
		idx++;
		if(sub_objects[num_curr_level_objs]->type == STRUCT || sub_objects[num_curr_level_objs]->type == REF)
		{
			//since this is a depth-first traversal
			placeInSuperObject(sub_objects[num_curr_level_objs], objects, num_total_objs, &idx);
		}
		num_curr_level_objs++;
	}
	
	super_object->num_sub_objs = num_curr_level_objs;
	super_object->sub_objects = sub_objects;
	*index = idx;
	
}
