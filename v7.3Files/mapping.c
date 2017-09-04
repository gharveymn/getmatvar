#include "mapping.h"


Data* findDataObject(const char* filename, const char variable_name[])
{
	Data** objects = getDataObjects(filename, variable_name);
	Data* hi_objects = organizeObjects(objects);
	
	//free the end object because we aren't going to be using the linear object freeing system
	int i = 0;
	while(objects[i]->type != SENTINEL)
	{
		i++;
	}
	free(objects[i]);
	free(objects);
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
		fprintf(stderr, "open() unsuccessful, Check errno: %d\n", errno);
		exit(EXIT_FAILURE);
	}
	
	//get file size
	size_t file_size = (size_t)lseek(fd, 0, SEEK_END);
	
	//find superblock
	s_block = getSuperblock(fd, file_size);
	
	//root_tree_address = queue.trios[queue.front].tree_address;
	
	//fprintf(stderr, "\nObject header for variable %s is at 0x", variable_name);
	findHeaderAddress(variable_name);
	//fprintf(stderr, "%llu\n", header_queue.objects[header_queue.front].this_obj_header_address);
	
	
	//interpret the header messages
	int num_objs = 0;
	while(header_queue.length > 0)
	{
		
		obj = dequeueObject();
		header_address = obj.this_obj_header_address;
		
		//initialize elements since we have nested deallocation
		data_objects_ptrs[num_objs] = malloc(sizeof(*(data_objects_ptrs[num_objs])));
		initializeObject(data_objects_ptrs[num_objs], obj);
		
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
	data_objects_ptrs[num_objs]->type = SENTINEL;
	data_objects_ptrs[num_objs]->parent_obj_address = UNDEF_ADDR;
	
	close(fd);
	return data_objects_ptrs;
}

void initializeObject(Data* object, Object obj)
{
	object->data_arrays.is_used = FALSE;
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
	object->dims = NULL;
	
	object->chunked_info.num_filters = 0;
	object->chunked_info.chunked_dims = NULL;
	object->chunked_info.num_chunked_dims = 0;
	
	object->sub_objects = NULL;
	
	//zero by default for REF data
	object->layout_class = 0;
	object->datatype_bit_field = 0;
	object->byte_order = LITTLE_ENDIAN;
	object->type = UNDEF;
	
	object->num_dims = 0;
	object->num_elems = 0;
	object->elem_size = 0;
	object->data_address = 0;
	object->num_sub_objs = 0;
	
}


void collectMetaData(Data* object, uint64_t header_address, char* header_pointer)
{
	
	uint16_t num_msgs = (uint16_t)getBytesAsNumber(header_pointer + 2, 2, META_DATA_BYTE_ORDER);
	
	uint16_t msg_type = 0;
	uint16_t msg_size = 0;
	uint32_t header_length;
	uint64_t msg_address = 0;
	char* msg_pointer, * data_pointer = 0;
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
				readDataSpaceMessage(object, msg_pointer, msg_address, msg_size);
				break;
			case 3:
				// DataType message
				readDataTypeMessage(object, msg_pointer, msg_address, msg_size);
				break;
			case 8:
				// Data Layout message
				data_pointer = readDataLayoutMessage(object, msg_pointer, msg_address, msg_size);
				break;
			case 11:
				//data storage pipeline message
				readDataStoragePipelineMessage(object, msg_pointer, msg_address, msg_size);
				break;
			case 12:
				//attribute message
				readAttributeMessage(object, msg_pointer, msg_address, msg_size);
				break;
			case 16:
				//object header continuation message
				//ie no info for the object
				header_address = getBytesAsNumber(msg_pointer, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
				header_length = (uint32_t)getBytesAsNumber(msg_pointer + s_block.size_of_offsets, s_block.size_of_lengths, META_DATA_BYTE_ORDER);
				header_pointer = navigateTo(header_address - 16, header_length + 16, TREE);
				header_address -= 16;
				bytes_read = 0 - msg_size - 8;
				break;
			default:
				//ignore message
				//case 17 -- B tree already traversed and in queue
				break;
		}
		
		bytes_read += msg_size + 8;
	}
	
	//allocate space for data
	allocateSpace(object);
	
	//fetch data
	switch(object->layout_class)
	{
		case 0:
		case 1:
			//compact storage or contiguous storage
			placeData(object, data_pointer, 0, object->num_elems, object->elem_size, object->byte_order);
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
			obj.this_obj_header_address = object->data_arrays.udouble_data[i];
			obj.parent_obj_header_address = object->this_obj_address;
			strcpy(obj.name, object->name);
			priorityEnqueueObject(obj);
		}
	}
}

void allocateSpace(Data* object)
{
	//why not just put it into one array and cast it later? To save memory.
	switch(object->type)
	{
		case INT8:
			object->data_arrays.i8_data = omalloc(object->num_elems * object->elem_size);
			break;
		case UINT8:
			object->data_arrays.ui8_data = omalloc(object->num_elems * object->elem_size);
			break;
		case INT16:
			object->data_arrays.i16_data = omalloc(object->num_elems * object->elem_size);
			break;
		case UINT16:
			object->data_arrays.ui16_data = omalloc(object->num_elems * object->elem_size);
			break;
		case INT32:
			object->data_arrays.i32_data = omalloc(object->num_elems * object->elem_size);
			break;
		case UINT32:
			object->data_arrays.ui32_data = omalloc(object->num_elems * object->elem_size);
			break;
		case INT64:
			object->data_arrays.i64_data = omalloc(object->num_elems * object->elem_size);
			break;
		case UINT64:
			object->data_arrays.ui64_data = omalloc(object->num_elems * object->elem_size);
			break;
		case SINGLE:
			object->data_arrays.single_data = omalloc(object->num_elems * object->elem_size);
			break;
		case DOUBLE:
			object->data_arrays.double_data = omalloc(object->num_elems * object->elem_size);
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
			object->dims = malloc(3 * sizeof(int));
			object->dims[0] = 1;
			object->dims[1] = 1;
			object->dims[2] = 0;
			break;
		case TABLE:
			//do nothing
			break;
		default:
			fprintf(stderr, "Unknown data type encountered");
			exit(EXIT_FAILURE);
	}
}

void placeData(Data* object, char* data_pointer, uint64_t starting_index, uint64_t condition, size_t elem_size, ByteOrder byte_order)
{
	int object_data_index = 0;
	switch(object->type)
	{
		case INT8:
			for(uint64_t j = starting_index; j < condition; j++)
			{
				object->data_arrays.i8_data[j] = (int8_t)getBytesAsNumber(data_pointer + object_data_index * elem_size, elem_size, byte_order);
				object_data_index++;
			}
			break;
		case UINT8:
			for(uint64_t j = starting_index; j < condition; j++)
			{
				object->data_arrays.ui8_data[j] = (char)getBytesAsNumber(data_pointer + object_data_index * elem_size, elem_size, byte_order);
				object_data_index++;
			}
			break;
		case INT16:
			for(uint64_t j = starting_index; j < condition; j++)
			{
				object->data_arrays.i16_data[j] = (int16_t)getBytesAsNumber(data_pointer + object_data_index * elem_size, elem_size, byte_order);
				object_data_index++;
			}
			break;
		case UINT16:
			for(uint64_t j = starting_index; j < condition; j++)
			{
				object->data_arrays.ui16_data[j] = (uint16_t)getBytesAsNumber(data_pointer + object_data_index * elem_size, elem_size, byte_order);
				object_data_index++;
			}
			break;
		case INT32:
			for(uint64_t j = starting_index; j < condition; j++)
			{
				object->data_arrays.i32_data[j] = (int32_t)getBytesAsNumber(data_pointer + object_data_index * elem_size, elem_size, byte_order);
				object_data_index++;
			}
			break;
		case UINT32:
			for(uint64_t j = starting_index; j < condition; j++)
			{
				object->data_arrays.ui32_data[j] = (uint32_t)getBytesAsNumber(data_pointer + object_data_index * elem_size, elem_size, byte_order);
				object_data_index++;
			}
			break;
		case INT64:
			for(uint64_t j = starting_index; j < condition; j++)
			{
				object->data_arrays.i64_data[j] = (int64_t)getBytesAsNumber(data_pointer + object_data_index * elem_size, elem_size, byte_order);
				object_data_index++;
			}
			break;
		case UINT64:
			for(uint64_t j = starting_index; j < condition; j++)
			{
				object->data_arrays.ui64_data[j] = (uint64_t)getBytesAsNumber(data_pointer + object_data_index * elem_size, elem_size, byte_order);
				object_data_index++;
			}
			break;
		case SINGLE:
			for(uint64_t j = starting_index; j < condition; j++)
			{
				object->data_arrays.single_data[j] = convertHexToSingle((uint32_t)getBytesAsNumber(data_pointer + object_data_index * elem_size, elem_size, byte_order));
				object_data_index++;
			}
			break;
		case DOUBLE:
			for(uint64_t j = starting_index; j < condition; j++)
			{
				object->data_arrays.double_data[j] = convertHexToDouble(getBytesAsNumber(data_pointer + object_data_index * elem_size, elem_size, byte_order));
				object_data_index++;
			}
			break;
		case REF:
			for(uint64_t j = starting_index; j < condition; j++)
			{
				//these are addresses so we have to add the offset
				object->data_arrays.udouble_data[j] = getBytesAsNumber(data_pointer + object_data_index * elem_size, elem_size, byte_order) + s_block.base_address;
				object_data_index++;
			}
			break;
		case STRUCT:
		case FUNCTION_HANDLE:
		case TABLE:
			//nothing to be done
			break;
		default:
			fprintf(stderr, "Unknown data type encountered");
			exit(EXIT_FAILURE);
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
	char varname[NAME_LENGTH];
	strcpy(varname, variable_name);
	
	flushVariableNameQueue();
	token = strtok(varname, delim);
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
			parent_trio = dequeueTrio();
			readTreeNode(tree_pointer, parent_trio);
		}
		else if(strncmp("SNOD", tree_pointer, 4) == 0)
		{
			this_trio = dequeueTrio();
			readSnod(tree_pointer, heap_pointer, parent_trio, this_trio);
		}
	}
	//fprintf(stderr, "0x%lx\n", header_address);
}


Data* organizeObjects(Data** objects)
{
	
	if(objects[0]->type == SENTINEL)
	{
		return NULL;
	}
	
	int num_total_objs = 0;
	while(objects[num_total_objs]->type != SENTINEL)
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
