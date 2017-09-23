#include "mapping.h"


Queue* getDataObjects(const char* filename, char** variable_names, int num_names)
{
	
	threads_are_started = FALSE;
	__byte_order__ = getByteOrder();
	alloc_gran = getAllocGran();

#ifdef DO_MEMDUMP
	pthread_cond_init(&dump_ready, NULL);
	pthread_mutex_init(&dump_lock, NULL);
	dump = fopen("memdump.log", "w+");
#endif
	
	Queue* objects = initQueue(freeDataObject);
	SNODEntry* snod_entry;
	char warn_msg[WARNING_BUFFER_SIZE];
	num_avail_threads = getNumProcessors() - 1;
	
	//init queues
	addr_queue = initQueue(NULL);
	varname_queue = initQueue(freeVarname);
	header_queue = initQueue(NULL);
	
	//open the file descriptor
	fd = open(filename, O_RDONLY);
	if(fd < 0)
	{
		Data* data_object = malloc(sizeof(Data));
		initializeObject(data_object);
		data_object->type = ERROR_DATA | END_SENTINEL;
		sprintf(data_object->name, "getmatvar:fileNotFoundError");
		sprintf(data_object->matlab_class, "No file found with name \'%s\'.\n\n", filename);
		enqueue(objects, data_object);
		return objects;
	}
	
	//get file size
	file_size = (size_t) lseek(fd, 0, SEEK_END);
	if(file_size == (size_t) -1)
	{
		Data* data_object = malloc(sizeof(Data));
		initializeObject(data_object);
		data_object->type = ERROR_DATA | END_SENTINEL;
		sprintf(data_object->name, "getmatvar:lseekFailureError");
		sprintf(data_object->matlab_class, "lseek failed, check errno %s\n\n", strerror(errno));
		enqueue(objects, data_object);
		return objects;
	}
	
	byte* head = navigateTo(0, MATFILE_SIG_LENGTH, TREE);
	if(memcmp(head, MATFILE_7_3_SIG, MATFILE_SIG_LENGTH) != 0)
	{
		char filetype[MATFILE_SIG_LENGTH];
		memcpy(filetype, head, MATFILE_SIG_LENGTH);
		Data* data_object = malloc(sizeof(Data));
		initializeObject(data_object);
		data_object->type = ERROR_DATA | END_SENTINEL;
		sprintf(data_object->name, "getmatvar:wrongFormatError");
		if(memcmp(filetype, "MATLAB", 6) == 0)
		{
			sprintf(data_object->matlab_class, "The input file must be a Version 7.3+ MAT-file. This is a %s.\n\n",
				   filetype);
		}
		else
		{
			sprintf(data_object->matlab_class,
				   "The input file must be a Version 7.3+ MAT-file. This is an unknown file format.\n\n");
		}
		enqueue(objects, data_object);
		return objects;
	}
	
	num_pages = file_size / alloc_gran + 1;
	
	initializePageObjects();
	
	//find superblock
	s_block = getSuperblock();
	
	bool_t load_all = FALSE;
	for(int i = 0; i < num_names; i++)
	{
		if(strcmp(variable_names[i], "\0") == 0)
		{
			findHeaderAddress(variable_names[i], TRUE);
			load_all = TRUE;
			break;
		}
	}
	
	if(load_all)
	{
		for(int i = 0; i < num_names; i++)
		{
			if(variable_names[i] != NULL && strcmp(variable_names[i], "\0") != 0)
			{
				free(variable_names[i]);
			}
		}
		free(variable_names);
		
		num_names = header_queue->length;
		
		variable_names = malloc(num_names * sizeof(char*));
		for(int j = 0; j < num_names; j++)
		{
			snod_entry = dequeue(header_queue);
			variable_names[j] = malloc(NAME_LENGTH * sizeof(char));
			strcpy(variable_names[j], snod_entry->name);
		}
	}
	
	for(int name_index = 0; name_index < num_names; name_index++)
	{
		flushQueue(addr_queue);
		flushQueue(header_queue);
		
		findHeaderAddress(variable_names[name_index], FALSE);
		
		if(header_queue->length == 0)
		{
			Data* data_object = malloc(sizeof(Data));
			initializeObject(data_object);
			data_object->type = UNDEF_DATA;
			strcpy(data_object->name, peekQueue(varname_queue, QUEUE_BACK));
			enqueue(objects, data_object);
			sprintf(warn_msg, "Variable \'%s\' was not found.", variable_names[name_index]);
			readMXWarn("getmatvar:variableNotFound", warn_msg);
		}
		
		//interpret the header messages
		while(header_queue->length > 0)
		{
			
			snod_entry = dequeue(header_queue);
			uint64_t header_address = snod_entry->this_obj_header_address;
			
			//initialize elements since we have nested deallocation
			Data* data_object = malloc(sizeof(Data));
			initializeObject(data_object);
			
			//by only asking for enough bytes to get the header length there is a chance a mapping can be reused
			byte* header_pointer = navigateTo(header_address, 16, TREE);
			uint32_t header_length = (uint32_t) getBytesAsNumber(header_pointer + 8, 4, META_DATA_BYTE_ORDER);
			uint16_t num_msgs = (uint16_t) getBytesAsNumber(header_pointer + 2, 2, META_DATA_BYTE_ORDER);
			
			strcpy(data_object->name, snod_entry->name);
			data_object->parent_obj_address = snod_entry->parent_obj_header_address;
			data_object->this_obj_address = snod_entry->this_obj_header_address;
			collectMetaData(data_object, header_address, num_msgs, header_length);
			enqueue(objects, data_object);
			
			//this block handles errors and sets up signals for controlled shutdown
			if(data_object->type == ERROR_DATA)
			{
				Data* front_object = peekQueue(objects, QUEUE_FRONT);
				front_object->type = ERROR_DATA;
				strcpy(front_object->name, front_object->name);
				strcpy(front_object->matlab_class, front_object->matlab_class);
				
				//note that num_objs is now the end sentinel
				
				Data* end_object = malloc(sizeof(Data));
				initializeObject(end_object);
				end_object->type = DELIMITER | END_SENTINEL;
				end_object->parent_obj_address = UNDEF_ADDR;
				enqueue(objects, end_object);
				
				return objects;
			}
			
		}
		
		//set object at the end to trigger sentinel
		Data* delimit_object = malloc(sizeof(Data));
		initializeObject(delimit_object);
		delimit_object->type = DELIMITER;
		delimit_object->parent_obj_address = UNDEF_ADDR;
		enqueue(objects, delimit_object);
		
		
	}
	Data* end_object = peekQueue(objects, QUEUE_BACK);
	end_object->type |= END_SENTINEL;
	
	for(int i = 0; i < num_names; i++)
	{
		free(variable_names[i]);
	}
	free(variable_names);
	
	close(fd);
	if(threads_are_started == TRUE)
	{
		pthread_mutex_destroy(&thread_acquisition_lock);
		thpool_destroy(threads);
	}
	
	destroyPageObjects();
	freeAllMaps();

#ifdef DO_MEMDUMP
	pthread_cond_destroy(&dump_ready);
	pthread_mutex_destroy(&dump_lock);
#endif
	
	return objects;
	
}


void initializeMaps(void)
{
	
	for(int i = 0; i < NUM_TREE_MAPS; i++)
	{
		tree_maps[i].used = FALSE;
		tree_maps[i].bytes_mapped = 0;
		tree_maps[i].map_start = NULL;
		tree_maps[i].offset = 0;
	}
	
	for(int i = 0; i < NUM_HEAP_MAPS; i++)
	{
		heap_maps[i].used = FALSE;
		heap_maps[i].bytes_mapped = 0;
		heap_maps[i].map_start = NULL;
		heap_maps[i].offset = 0;
	}
	
	map_nums[TREE] = NUM_TREE_MAPS;
	map_nums[HEAP] = NUM_HEAP_MAPS;
	map_queue_fronts[TREE] = 0;
	map_queue_fronts[HEAP] = 0;
	
}


void initializePageObjects(void)
{
	page_objects = malloc(num_pages * sizeof(pageObject));
	for(int i = 0; i < num_pages; i++)
	{
		pthread_mutex_init(&page_objects[i].lock, NULL);
		//page_objects[i].ready = PTHREAD_COND_INITIALIZER;//initialize these later if we need to?
		//page_objects[i].lock = PTHREAD_MUTEX_INITIALIZER;
		page_objects[i].is_mapped = FALSE;
		page_objects[i].pg_start_a = alloc_gran * i;
		page_objects[i].pg_end_a = MIN(alloc_gran * (i + 1), file_size);
		page_objects[i].map_base = UNDEF_ADDR;
		page_objects[i].map_end = UNDEF_ADDR;
		page_objects[i].pg_start_p = NULL;
		page_objects[i].num_using = 0;
	}
	pthread_spin_init(&if_lock, PTHREAD_PROCESS_SHARED);
}


void destroyPageObjects(void)
{
	
	for(int i = 0; i < num_pages; ++i)
	{
		if(page_objects[i].is_mapped == TRUE)
		{
			
			if(munmap(page_objects[i].pg_start_p, page_objects[i].map_end - page_objects[i].map_base) != 0)
			{
				readMXError("getmatvar:badMunmapError", "munmap() unsuccessful in freeMap(). Check errno %s\n\n",
						  strerror(errno));
			}
			
			page_objects[i].is_mapped = FALSE;
			page_objects[i].pg_start_p = NULL;
			page_objects[i].map_base = UNDEF_ADDR;
			page_objects[i].map_end = UNDEF_ADDR;
			
		}
		
		pthread_mutex_destroy(&page_objects[i].lock);
		
	}
	
	pthread_spin_destroy(&if_lock);
	free(page_objects);
	
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
	object->data_arrays.sub_object_header_offsets = NULL;
	
	object->chunked_info.num_filters = 0;
	object->chunked_info.num_chunked_dims = 0;
	object->chunked_info.num_chunked_elems = 0;
	for(int i = 0; i < MAX_NUM_FILTERS; i++)
	{
		object->chunked_info.filters[i].client_data = NULL;
	}
	
	object->sub_objects = NULL;
	
	//zero by default for REF_DATA data
	object->layout_class = 0;
	object->datatype_bit_field = 0;
	object->byte_order = LITTLE_ENDIAN;
	object->type = UNDEF_DATA;
	object->complexity_flag = mxREAL;
	
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
	
	
	if(object->type == UNDEF_DATA)
	{
		object->type = ERROR_DATA;
		sprintf(object->name, "getmatvar:unknownDataTypeError");
		sprintf(object->matlab_class, "Unknown data type encountered.\n\n");
		return;
	}
	
	//allocate space for data
	if(allocateSpace(object) != 0)
	{
		return;
	}
	
	
	//fetch data
	switch(object->layout_class)
	{
		case 0:
		case 1:
			//compact storage or contiguous storage
			//placeData will just segfault if it has an error, ie. if this encounters an error something is very wrong
			placeData(object, object->data_pointer, 0, object->num_elems, object->elem_size, object->byte_order);
			break;
		case 2:
			//chunked storage
			if(getChunkedData(object) != 0)
			{
				return;
			}
			break;
		default:
			object->type = ERROR_DATA;
			sprintf(object->name, "getmatvar:unknownLayoutClassError");
			sprintf(object->matlab_class, "Unknown layout class encountered.\n\n");
			return;
	}
	
	//if we have encountered a cell array, queue up headers for its elements
	if(object->data_arrays.sub_object_header_offsets != NULL && object->type == REF_DATA)
	{
		
		for(int i = object->num_elems - 1; i >= 0; i--)
		{
			SNODEntry* snod_entry = malloc(sizeof(SNODEntry));
			snod_entry->this_obj_header_address =
					object->data_arrays.sub_object_header_offsets[i] + s_block.base_address;
			snod_entry->parent_obj_header_address = object->this_obj_address;
			strcpy(snod_entry->name, object->name);
			priorityEnqueue(header_queue, snod_entry);
		}
	}
}


uint16_t interpretMessages(Data* object, uint64_t header_address, uint32_t header_length, uint16_t message_num,
					  uint16_t num_msgs, uint16_t repeat_tracker)
{
	
	byte* header_pointer = navigateTo(header_address, header_length, TREE);
	
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
		msg_type = (uint16_t) getBytesAsNumber(header_pointer + 16 + bytes_read, 2, META_DATA_BYTE_ORDER);
		//msg_address = header_address + 16 + bytes_read;
		msg_size = (uint16_t) getBytesAsNumber(header_pointer + 16 + bytes_read + 2, 2, META_DATA_BYTE_ORDER);
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
				cont_header_address = getBytesAsNumber(msg_pointer, s_block.size_of_offsets, META_DATA_BYTE_ORDER) +
								  s_block.base_address;
				cont_header_length = (uint32_t) getBytesAsNumber(msg_pointer + s_block.size_of_offsets,
													    s_block.size_of_lengths, META_DATA_BYTE_ORDER);
				message_num++;
				message_num = interpretMessages(object, cont_header_address - 16, cont_header_length, message_num,
										  num_msgs, repeat_tracker);
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
	
	return (uint16_t) (message_num - 1);
	
}


errno_t allocateSpace(Data* object)
{
	//maybe figure out a way to just pass this to a single array
	switch(object->type)
	{
		case INT8_DATA:
			object->data_arrays.i8_data = mxMalloc(object->num_elems * object->elem_size);
			break;
		case UINT8_DATA:
			object->data_arrays.ui8_data = mxMalloc(object->num_elems * object->elem_size);
			break;
		case INT16_DATA:
			object->data_arrays.i16_data = mxMalloc(object->num_elems * object->elem_size);
			break;
		case UINT16_DATA:
			object->data_arrays.ui16_data = mxMalloc(object->num_elems * object->elem_size);
			break;
		case INT32_DATA:
			object->data_arrays.i32_data = mxMalloc(object->num_elems * object->elem_size);
			break;
		case UINT32_DATA:
			object->data_arrays.ui32_data = mxMalloc(object->num_elems * object->elem_size);
			break;
		case INT64_DATA:
			object->data_arrays.i64_data = mxMalloc(object->num_elems * object->elem_size);
			break;
		case UINT64_DATA:
			object->data_arrays.ui64_data = mxMalloc(object->num_elems * object->elem_size);
			break;
		case SINGLE_DATA:
			object->data_arrays.single_data = mxMalloc(object->num_elems * object->elem_size);
			break;
		case DOUBLE_DATA:
			object->data_arrays.double_data = mxMalloc(object->num_elems * object->elem_size);
			break;
		case REF_DATA:
			//STORE ADDRESSES IN THE UDOUBLE_DATA ARRAY; THESE ARE NOT ACTUAL ELEMENTS
			object->data_arrays.sub_object_header_offsets = malloc(object->num_elems * object->elem_size);
			break;
		case STRUCT_DATA:
		case FUNCTION_HANDLE_DATA:
			object->num_elems = 1;
			object->num_dims = 2;
			object->dims[0] = 1;
			object->dims[1] = 1;
			object->dims[2] = 0;
			break;
		case TABLE_DATA:
			//do nothing
			break;
		case NULLTYPE_DATA:
		default:
			//this shouldn't happen
			object->type = ERROR_DATA;
			sprintf(object->name, "getmatvar:thisShouldntHappen");
			sprintf(object->matlab_class, "Allocated space ran with an NULLTYPE_DATA for some reason.\n\n");
			return 1;
		
	}
	
	return 0;
	
}


void placeData(Data* object, byte* data_pointer, uint64_t starting_index, uint64_t condition, size_t elem_size,
			ByteOrder data_byte_order)
{
	
	//reverse the bytes if the byte order doesn't match the cpu architecture
	if(__byte_order__ != data_byte_order)
	{
		for(uint64_t j = 0; j < condition - starting_index; j += elem_size)
		{
			reverseBytes(data_pointer + j, elem_size);
		}
	}
	
	switch(object->type)
	{
		case INT8_DATA:
			memcpy(&object->data_arrays.i8_data[starting_index], data_pointer,
				  (condition - starting_index) * elem_size);
			break;
		case UINT8_DATA:
			memcpy(&object->data_arrays.ui8_data[starting_index], data_pointer,
				  (condition - starting_index) * elem_size);
			break;
		case INT16_DATA:
			memcpy(&object->data_arrays.i16_data[starting_index], data_pointer,
				  (condition - starting_index) * elem_size);
			break;
		case UINT16_DATA:
			memcpy(&object->data_arrays.ui16_data[starting_index], data_pointer,
				  (condition - starting_index) * elem_size);
			break;
		case INT32_DATA:
			memcpy(&object->data_arrays.i32_data[starting_index], data_pointer,
				  (condition - starting_index) * elem_size);
			break;
		case UINT32_DATA:
			memcpy(&object->data_arrays.ui32_data[starting_index], data_pointer,
				  (condition - starting_index) * elem_size);
			break;
		case INT64_DATA:
			memcpy(&object->data_arrays.i64_data[starting_index], data_pointer,
				  (condition - starting_index) * elem_size);
			break;
		case UINT64_DATA:
			memcpy(&object->data_arrays.ui64_data[starting_index], data_pointer,
				  (condition - starting_index) * elem_size);
			break;
		case SINGLE_DATA:
			memcpy(&object->data_arrays.single_data[starting_index], data_pointer,
				  (condition - starting_index) * elem_size);
			break;
		case DOUBLE_DATA:
			memcpy(&object->data_arrays.double_data[starting_index], data_pointer,
				  (condition - starting_index) * elem_size);
			break;
		case REF_DATA:
			memcpy(&object->data_arrays.sub_object_header_offsets[starting_index], data_pointer,
				  (condition - starting_index) * elem_size);
			break;
		case STRUCT_DATA:
		case FUNCTION_HANDLE_DATA:
		case TABLE_DATA:
		default:
			//nothing to be done
			break;
		
	}
	
}


void
placeDataWithIndexMap(Data* object, byte* data_pointer, uint64_t num_elems, size_t elem_size, ByteOrder data_byte_order,
				  const uint64_t* index_map)
{
	
	//reverse the bytes if the byte order doesn't match the cpu architecture
	if(__byte_order__ != data_byte_order)
	{
		for(uint64_t j = 0; j < num_elems; j += elem_size)
		{
			reverseBytes(data_pointer + j, elem_size);
		}
	}
	
	
	int object_data_index = 0;
	switch(object->type)
	{
		case INT8_DATA:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.i8_data[index_map[j]], data_pointer + object_data_index * elem_size,
					  elem_size);
				object_data_index++;
			}
			break;
		case UINT8_DATA:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.ui8_data[index_map[j]], data_pointer + object_data_index * elem_size,
					  elem_size);
				object_data_index++;
			}
			break;
		case INT16_DATA:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.i16_data[index_map[j]], data_pointer + object_data_index * elem_size,
					  elem_size);
				object_data_index++;
			}
			break;
		case UINT16_DATA:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.ui16_data[index_map[j]], data_pointer + object_data_index * elem_size,
					  elem_size);
				object_data_index++;
			}
			break;
		case INT32_DATA:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.i32_data[index_map[j]], data_pointer + object_data_index * elem_size,
					  elem_size);
				object_data_index++;
			}
			break;
		case UINT32_DATA:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.ui32_data[index_map[j]], data_pointer + object_data_index * elem_size,
					  elem_size);
				object_data_index++;
			}
			break;
		case INT64_DATA:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.i64_data[index_map[j]], data_pointer + object_data_index * elem_size,
					  elem_size);
				object_data_index++;
			}
			break;
		case UINT64_DATA:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.ui64_data[index_map[j]], data_pointer + object_data_index * elem_size,
					  elem_size);
				object_data_index++;
			}
			break;
		case SINGLE_DATA:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.single_data[index_map[j]], data_pointer + object_data_index * elem_size,
					  elem_size);
				object_data_index++;
			}
			break;
		case DOUBLE_DATA:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				object->data_arrays.double_data[index_map[j]] = *(double*) (data_pointer +
																object_data_index * elem_size);
				//memcpy(&object->data_arrays.double_data[index_map[j]], data_pointer + object_data_index*elem_size, elem_size);
				object_data_index++;
			}
			break;
		case REF_DATA:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.sub_object_header_offsets[index_map[j]],
					  data_pointer + object_data_index * elem_size, elem_size);
				object_data_index++;
			}
			break;
		case STRUCT_DATA:
		case FUNCTION_HANDLE_DATA:
		case TABLE_DATA:
		default:
			//nothing to be done
			break;
		
	}
	
}


void findHeaderAddress(char* variable_name, bool_t get_top_level)
{
	char* delim = ".", * token;
	AddrTrio* root_trio_copy = malloc(sizeof(AddrTrio));
	root_trio_copy->tree_address = root_trio.tree_address;
	root_trio_copy->heap_address = root_trio.heap_address;
	root_trio_copy->parent_obj_header_address = root_trio.parent_obj_header_address;
	enqueue(addr_queue, root_trio_copy);
	default_bytes = (uint64_t) getAllocGran();
	default_bytes = default_bytes < file_size ? default_bytes : file_size;
	
	flushQueue(varname_queue);
	if(strcmp(variable_name, "\0") == 0)
	{
		enqueue(varname_queue, NULL);
		variable_found = TRUE;
	}
	else
	{
		variable_found = FALSE;
		
		token = strtok(variable_name, delim);
		while(token != NULL)
		{
			char* vn = malloc((strlen(token) + 1) * sizeof(char));
			strcpy(vn, token);
			enqueue(varname_queue, vn);
			token = strtok(NULL, delim);
		}
	}
	
	parseHeaderTree(get_top_level);
	
}


void parseHeaderTree(bool_t get_top_level)
{
	byte* tree_pointer, * heap_pointer;
	
	//aka the parent address for a snod
	AddrTrio* parent_trio, * this_trio;
	
	//search for the object header for the variable
	while(addr_queue->length > 0)
	{
		this_trio = peekQueue(addr_queue, QUEUE_FRONT);
		tree_pointer = navigateTo(this_trio->tree_address, default_bytes, TREE);
		heap_pointer = navigateTo(this_trio->heap_address, default_bytes, HEAP);
		
		if(strncmp("TREE", (char*) tree_pointer, 4) == 0)
		{
			parent_trio = dequeue(addr_queue);
			readTreeNode(tree_pointer, parent_trio);
		}
		else if(strncmp("SNOD", (char*) tree_pointer, 4) == 0)
		{
			this_trio = dequeue(addr_queue);
			readSnod(tree_pointer, heap_pointer, parent_trio, this_trio, get_top_level);
		}
		
	}
	
}


Data* organizeObjects(Queue* objects)
{
	Data* super_object = dequeue(objects);
	
	while((DELIMITER & super_object->type) == DELIMITER)
	{
		if(objects->length == 0)
		{
			return NULL;
		}
		super_object = dequeue(objects);
	}
	
	if(super_object->type == STRUCT_DATA || super_object->type == REF_DATA)
	{
		placeInSuperObject(super_object, objects, objects->length);
	}
	
	return super_object;
	
}


void placeInSuperObject(Data* super_object, Queue* objects, int num_objs_left)
{
	//note: index should be at the starting index of the subobjects
	
	super_object->num_sub_objs = 0;
	super_object->sub_objects = malloc(num_objs_left * sizeof(Data*));
	Data* curr = dequeue(objects);
	
	while(super_object->this_obj_address == curr->parent_obj_address)
	{
		super_object->sub_objects[super_object->num_sub_objs] = curr;
		if(super_object->sub_objects[super_object->num_sub_objs]->type == STRUCT_DATA ||
		   super_object->sub_objects[super_object->num_sub_objs]->type == REF_DATA)
		{
			//since this is a depth-first traversal
			placeInSuperObject(super_object->sub_objects[super_object->num_sub_objs], objects, objects->length);
		}
		curr = dequeue(objects);
		super_object->num_sub_objs++;
	}
	
}


void freeVarname(void* vn)
{
	char* varname = (char*) vn;
	if(varname != NULL && strcmp(varname, "\0") != 0)
	{
		free(varname);
	}
}

void readMXError(const char error_id[], const char error_message[], ...)
{
	
	char message_buffer[ERROR_BUFFER_SIZE];
	
	va_list va;
	va_start(va, error_message);
	sprintf(message_buffer, error_message, va);
	strcat(message_buffer, MATLAB_HELP_MESSAGE);
	endHooks();
	va_end(va);

#ifdef NO_MEX
	printf(message_buffer);
#else
	mexErrMsgIdAndTxt(error_id, message_buffer);
#endif

}


void readMXWarn(const char warn_id[], const char warn_message[], ...)
{
	char message_buffer[WARNING_BUFFER_SIZE];
	
	va_list va;
	va_start(va, warn_message);
	sprintf(message_buffer, warn_message, va);
	strcat(message_buffer, MATLAB_WARN_MESSAGE);
	va_end(va);

#ifdef NO_MEX
	printf(message_buffer);
#else
	mexWarnMsgIdAndTxt(warn_id, message_buffer);
#endif

}