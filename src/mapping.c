#include "getmatvar_.h"


Data* findDataObject(const char* filename, const char variable_name[])
{
	Data** objects = getDataObjects(filename, &variable_name, 1);
	Data* hi_objects = organizeObjects(objects, 0);
	
	//free the end object because we aren't going to be using the linear object freeing system
	int i = 0;
	while((DELIMITER & objects[i]->type) != DELIMITER)
	{
		i++;
	}
	free(objects[i]);
	free(objects);
	return hi_objects;
}


Data** getDataObjects(const char* filename, const char* variable_names[], int num_names)
{
	byte* header_pointer;
	uint32_t header_length;
	uint64_t header_address;
	uint16_t num_msgs;
	//uint64_t root_tree_address;
	Data** objects = malloc(MAX_OBJS * sizeof(Data*));
	Object obj;
	char errmsg[NAME_LENGTH];
	
	//init maps
	maps[TREE].used = FALSE;
	maps[HEAP].used = FALSE;
	
	//init queue
	flushQueue();
	flushHeaderQueue();
	
	//open the file descriptor
	fd = open(filename, O_RDONLY);
	if(fd < 0)
	{
		objects[0] = malloc(sizeof(Data));
		objects[0]->type = ERROR | END_SENTINEL;
		sprintf(objects[0]->name, "getmatvar:fileNotFoundError");
		sprintf(objects[0]->matlab_class, "No file found with name \'%s\'.\n\n", filename);
		return objects;
	}
	
	//get file size
	file_size = (size_t)lseek(fd, 0, SEEK_END);
	if(file_size == (size_t)-1)
	{
		objects[0] = malloc(sizeof(Data));
		objects[0]->type = ERROR | END_SENTINEL;
		sprintf(objects[0]->name, "getmatvar:internalError");
		sprintf(objects[0]->matlab_class, "lseek failed, check errno %d\n\n", errno);
		return objects;
	}
	
	//find superblock
	s_block = getSuperblock();
	
	int num_objs = 0;
	for(int name_index = 0; name_index < num_names; name_index++)
	{
		
		flushQueue();
		flushHeaderQueue();
		enqueueTrio(root_trio);
		
		//fprintf(stderr, "\nObject header for variable %s is at 0x", variable_names);
		findHeaderAddress(variable_names[name_index]);
		//fprintf(stderr, "%llu\n", header_queue.objects[header_queue.front].this_obj_header_address);
		
		if(header_queue.length == 0)
		{
			objects[num_objs] = malloc(sizeof(Data));
			initializeObject(objects[num_objs]);
			objects[num_objs]->type = UNDEF;
			
			strcpy(objects[num_objs]->name, variable_name_queue.variable_names[variable_name_queue.back - 1]);
			num_objs++;
			sprintf(errmsg, "Variable \'%s\' was not found.", variable_names[name_index]);
			readMXWarn("getmatvar:variableNotFound", errmsg);
		}
		
		//interpret the header messages
		while(header_queue.length > 0)
		{
			
			obj = dequeueObject();
			header_address = obj.this_obj_header_address;
			
			//initialize elements since we have nested deallocation
			objects[num_objs] = malloc(sizeof(*(objects[num_objs])));
			initializeObject(objects[num_objs]);
			
			//by only asking for enough bytes to get the header length there is a chance a mapping can be reused
			header_pointer = navigateTo(header_address, 16, TREE);
			header_length = (uint32_t)getBytesAsNumber(header_pointer + 8, 4, META_DATA_BYTE_ORDER);
			num_msgs = (uint16_t)getBytesAsNumber(header_pointer + 2, 2, META_DATA_BYTE_ORDER);
			
			strcpy(objects[num_objs]->name, obj.name);
			objects[num_objs]->parent_obj_address = obj.parent_obj_header_address;
			objects[num_objs]->this_obj_address = obj.this_obj_header_address;
			collectMetaData(objects[num_objs], header_address, num_msgs, header_length);
			
			if(objects[num_objs]->type == ERROR)
			{
				objects[0]->type = ERROR;
				strcpy(objects[0]->name, objects[num_objs]->name);
				strcpy(objects[0]->matlab_class, objects[num_objs]->matlab_class);
				
				num_objs++;
				
				objects[num_objs] = malloc(sizeof(Data));
				initializeObject(objects[num_objs]);
				objects[num_objs]->type = DELIMITER | END_SENTINEL;
				objects[num_objs]->parent_obj_address = UNDEF_ADDR;
				
				return objects;
			}
			
			num_objs++;
			
		}
		
		//set object at the end to trigger sentinel
		objects[num_objs] = malloc(sizeof(Data));
		initializeObject(objects[num_objs]);
		objects[num_objs]->type = DELIMITER;
		objects[num_objs]->parent_obj_address = UNDEF_ADDR;
		num_objs++;
		
	}
	objects[num_objs - 1]->type |= END_SENTINEL;
	
	freeMap(TREE);
	freeMap(HEAP);
	
	close(fd);
	return objects;
}

void initializeMaps(void)
{
	for(int i = 0; i < NUM_MAPS; i++)
	{
		maps[i].used = FALSE;
		maps[i].bytes_mapped = 0;
		maps[i].map_start = NULL;
		maps[i].offset = 0;
	}
}


void initializeObject(Data* object)
{
	object->data_arrays.is_mx_used = FALSE;
	object->data_arrays.ui8_data = NULL;
	object->data_arrays.i8_data = NULL;
	object->data_arrays.ui16_data = NULL;
	object->data_arrays.i16_data = NULL;
	object->data_arrays.ui32_data = NULL;
	object->data_arrays.i32_data = NULL;
	object->data_arrays.ui64_data = NULL;
	object->data_arrays.i64_data = NULL;
	object->data_arrays.single_data = NULL;
	object->data_arrays.double_data = NULL;
	object->data_arrays.udouble_data = NULL;
	
	object->chunked_info.num_filters = 0;
	object->chunked_info.num_chunked_dims = 0;
	
	object->sub_objects = NULL;
	
	//zero by default for REF data
	object->layout_class = 0;
	object->datatype_bit_field = 0;
	object->byte_order = LITTLE_ENDIAN;
	object->type = ERROR;
	
	object->num_dims = 0;
	object->num_elems = 0;
	object->elem_size = 0;
	object->data_address = 0;
	object->num_sub_objs = 0;
	
}


void collectMetaData(Data* object, uint64_t header_address, uint16_t num_msgs, uint32_t header_length)
{

	interpretMessages(object, header_address, header_length, 0, num_msgs, 0);
	
	//you can't be one dimensional and have two elements, so matlab stores nulls like this
	if(object->num_elems == 2 && object->num_dims == 1)
	{
		object->num_elems = 0;
		for(int i = 0; i < object->num_dims; i++)
		{
			object->dims[i] = 0;
		}
		object->num_dims = 0;
		return;
	}
	

	if(object->type == ERROR)
	{
		sprintf(object->name, "getmatvar:internalError");
		sprintf(object->matlab_class, "Unknown data type encountered.\n\n");
		return;
	}
	
	//allocate space for data
	allocateSpace(object);
	
	
	//fetch data
	switch(object->layout_class)
	{
		case 0:
		case 1:
			//compact storage or contiguous storage
			placeData(object, object->data_pointer, 0, object->num_elems, object->elem_size, object->byte_order);
			break;
		case 2:
			//chunked storage
			getChunkedData(object);
			break;
	}
	
	//if we have encountered a cell array, queue up headers for its elements
	if(object->data_arrays.udouble_data != NULL && object->type == REF)
	{

		for(int i = object->num_elems - 1; i >= 0; i--)
		{
			Object obj;
			obj.this_obj_header_address = object->data_arrays.udouble_data[i] + s_block.base_address;
			obj.parent_obj_header_address = object->this_obj_address;
			strcpy(obj.name, object->name);
			priorityEnqueueObject(obj);
		}
	}
}


uint16_t interpretMessages(Data* object, uint64_t header_address, uint32_t header_length, uint16_t message_num, uint16_t num_msgs, uint16_t repeat_tracker)
{
	
	byte* header_pointer = navigateTo(header_address, header_length, TREE);
	
	uint64_t cont_header_address;
	uint32_t cont_header_length;
	
	uint16_t msg_type = 0;
	uint16_t msg_size = 0;
	uint64_t msg_address = 0;
	byte* msg_pointer = NULL;
	int32_t bytes_read = 0;
	
	//interpret messages in header
	for(; message_num < num_msgs && bytes_read < header_length; message_num++)
	{
		msg_type = (uint16_t)getBytesAsNumber(header_pointer + 16 + bytes_read, 2, META_DATA_BYTE_ORDER);
		msg_address = header_address + 16 + bytes_read;
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
				message_num = interpretMessages(object, cont_header_address - 16, cont_header_length + 16, message_num, num_msgs, repeat_tracker);
				//renavigate in case the continuation message was far away (automatically checks if we need to)
				header_pointer = navigateTo(header_address, header_length, TREE);
				
				break;
			default:
				//ignore message
				//case 17 -- B tree already traversed and in queue
				break;
		}
		
		bytes_read += msg_size + 8;
	}
	
	return message_num - 1;
	
}


void allocateSpace(Data* object)
{
	//why not just put it into one array and cast it later? To save memory.
	switch(object->type)
	{
		case INT8:
			object->data_arrays.i8_data = mxMalloc(object->num_elems * object->elem_size);
			break;
		case UINT8:
			object->data_arrays.ui8_data = mxMalloc(object->num_elems * object->elem_size);
			break;
		case INT16:
			object->data_arrays.i16_data = mxMalloc(object->num_elems * object->elem_size);
			break;
		case UINT16:
			object->data_arrays.ui16_data = mxMalloc(object->num_elems * object->elem_size);
			break;
		case INT32:
			object->data_arrays.i32_data = mxMalloc(object->num_elems * object->elem_size);
			break;
		case UINT32:
			object->data_arrays.ui32_data = mxMalloc(object->num_elems * object->elem_size);
			break;
		case INT64:
			object->data_arrays.i64_data = mxMalloc(object->num_elems * object->elem_size);
			break;
		case UINT64:
			object->data_arrays.ui64_data = mxMalloc(object->num_elems * object->elem_size);
			break;
		case SINGLE:
			object->data_arrays.single_data = mxMalloc(object->num_elems * object->elem_size);
			break;
		case DOUBLE:
			object->data_arrays.double_data = mxMalloc(object->num_elems * object->elem_size);
			break;
		case REF:
			//STORE ADDRESSES IN THE UDOUBLE_DATA ARRAY; THESE ARE NOT ACTUAL ELEMENTS
			object->data_arrays.udouble_data = malloc(object->num_elems * object->elem_size);
			break;
		case STRUCT:
		case FUNCTION_HANDLE:
			if(object->dims != NULL)
			{
				free(object->dims);
			}
			object->num_elems = 1;
			object->num_dims = 2;
			object->dims[0] = 1;
			object->dims[1] = 1;
			object->dims[2] = 0;
			break;
		case TABLE:
			//do nothing
			break;
		case NULLTYPE:
		default:
			//this shouldn't happen
			readMXError("getmatvar:thisShouldntHappen", "Allocate space ran with an NULLTYPE for some reason.\n\n");
			//object->type = NULLTYPE;
			//sprintf(object->name, "getmatvar:internalError");
			//sprintf(object->matlab_class, "Unknown data type encountered.\n\n");
	}
}


void placeData(Data* object, byte* data_pointer, uint64_t starting_index, uint64_t condition, size_t elem_size, ByteOrder data_byte_order)
{
	
	//reverse the bytes if the byte order doesn't match the cpu architecture
	if(__BYTE_ORDER != data_byte_order)
	{
		for(uint64_t j = 0; j < condition - starting_index; j += elem_size)
		{
			reverseBytes(data_pointer + j, elem_size);
		}
	}
	
	
	int object_data_index = 0;
	switch(object->type)
	{
		case INT8:
			for(uint64_t j = starting_index; j < condition; j++)
			{
				memcpy(&object->data_arrays.i8_data[j], data_pointer + object_data_index * elem_size, elem_size);
				object_data_index++;
			}
			break;
		case UINT8:
			for(uint64_t j = starting_index; j < condition; j++)
			{
				memcpy(&object->data_arrays.ui8_data[j], data_pointer + object_data_index * elem_size, elem_size);
				object_data_index++;
			}
			break;
		case INT16:
			for(uint64_t j = starting_index; j < condition; j++)
			{
				memcpy(&object->data_arrays.i16_data[j], data_pointer + object_data_index * elem_size, elem_size);
				object_data_index++;
			}
			break;
		case UINT16:
			for(uint64_t j = starting_index; j < condition; j++)
			{
				memcpy(&object->data_arrays.ui16_data[j], data_pointer + object_data_index * elem_size, elem_size);
				object_data_index++;
			}
			break;
		case INT32:
			for(uint64_t j = starting_index; j < condition; j++)
			{
				memcpy(&object->data_arrays.i32_data[j], data_pointer + object_data_index * elem_size, elem_size);
				object_data_index++;
			}
			break;
		case UINT32:
			for(uint64_t j = starting_index; j < condition; j++)
			{
				memcpy(&object->data_arrays.ui32_data[j], data_pointer + object_data_index * elem_size, elem_size);
				object_data_index++;
			}
			break;
		case INT64:
			for(uint64_t j = starting_index; j < condition; j++)
			{
				memcpy(&object->data_arrays.i64_data[j], data_pointer + object_data_index * elem_size, elem_size);
				object_data_index++;
			}
			break;
		case UINT64:
			for(uint64_t j = starting_index; j < condition; j++)
			{
				memcpy(&object->data_arrays.ui64_data[j], data_pointer + object_data_index * elem_size, elem_size);
				object_data_index++;
			}
			break;
		case SINGLE:
			for(uint64_t j = starting_index; j < condition; j++)
			{
				memcpy(&object->data_arrays.single_data[j], data_pointer + object_data_index * elem_size, elem_size);
				object_data_index++;
			}
			break;
		case DOUBLE:
			for(uint64_t j = starting_index; j < condition; j++)
			{
				memcpy(&object->data_arrays.double_data[j], data_pointer + object_data_index * elem_size, elem_size);
				object_data_index++;
			}
			break;
		case REF:
			for(uint64_t j = starting_index; j < condition; j++)
			{
				memcpy(&object->data_arrays.udouble_data[j], data_pointer + object_data_index * elem_size, elem_size);
				object_data_index++;
			}
			break;
		case STRUCT:
		case FUNCTION_HANDLE:
		case TABLE:
		default:
			//nothing to be done
			break;
		
	}
	
}


void findHeaderAddress(const char variable_name[])
{
	char* delim = ".";
	char* token;
	default_bytes = (uint64_t)getAllocGran();
	default_bytes = default_bytes < file_size ? default_bytes : file_size;
	char varname[NAME_LENGTH];
	strcpy(varname, variable_name);
	variable_found = FALSE;
	
	flushVariableNameQueue();
	token = strtok(varname, delim);
	while(token != NULL)
	{
		enqueueVariableName(token);
		token = strtok(NULL, delim);
	}
	
	parseHeaderTree();
}


void parseHeaderTree(void)
{
	byte* tree_pointer, * heap_pointer;
	
	//aka the parent address for a snod
	Addr_Trio parent_trio = {.parent_obj_header_address = UNDEF_ADDR, .tree_address = UNDEF_ADDR, .heap_address = UNDEF_ADDR};
	Addr_Trio this_trio;
	
	//search for the object header for the variable
	while(queue.length > 0)
	{
		tree_pointer = navigateTo(queue.trios[queue.front].tree_address, default_bytes, TREE);
		heap_pointer = navigateTo(queue.trios[queue.front].heap_address, default_bytes, HEAP);
		if(strncmp("HEAP", heap_pointer, 4) != 0)
		{
			readMXError("getmatvar:internalError", "Incorrect heap_pointer address in queue.\n\n", "");
		}
		
		if(strncmp("TREE", tree_pointer, 4) == 0)
		{
			parent_trio = dequeueTrio();
			readTreeNode(tree_pointer, parent_trio);
		}
		else if(strncmp("SNOD", tree_pointer, 4) == 0)
		{
			this_trio = dequeueTrio();
			readSnod(tree_pointer, heap_pointer, parent_trio, this_trio);
		}
	}
}


Data* organizeObjects(Data** objects, int* starting_pos)
{
	
	if((DELIMITER & objects[*starting_pos]->type) == DELIMITER)
	{
		return NULL;
	}
	
	int num_total_objs = *starting_pos;
	while((DELIMITER & objects[num_total_objs]->type) != DELIMITER)
	{
		num_total_objs++;
	}
	
	int index = *starting_pos;
	Data* super_object = objects[*starting_pos];
	index++;
	
	if(super_object->type == STRUCT || super_object->type == REF)
	{
		placeInSuperObject(super_object, objects, num_total_objs, &index);
	}
	
	*starting_pos = num_total_objs + 1;
	
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