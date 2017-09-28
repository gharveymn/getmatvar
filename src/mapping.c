#include "mapping.h"
//#pragma message ("getmatvar is compiling on WINDOWS")


void getDataObjects(const char* filename, char** variable_names, const int num_names)
{
	
	super_object = malloc(sizeof(Data));
	initializeObject(super_object);
	enqueue(object_queue, super_object);
	
	threads_are_started = FALSE;
	__byte_order__ = getByteOrder();
	alloc_gran = getAllocGran();

#ifdef DO_MEMDUMP
	pthread_cond_init(&dump_ready, NULL);
	pthread_mutex_init(&dump_lock, NULL);
	dump = fopen("memdump.log", "w+");
#endif
	
	num_avail_threads = getNumProcessors() - 1;
	
	//init queues
	varname_queue = initQueue(freeVarname);
	
	//open the file descriptor
	fd = open(filename, O_RDONLY);
	if(fd < 0)
	{
		error_flag = TRUE;
		sprintf(error_id, "getmatvar:fileNotFoundError");
		sprintf(error_message, "No file found with name \'%s\'.\n\n", filename);
		return;
	}
	
	//get file size
	file_size = (size_t)lseek(fd, 0, SEEK_END);
	if(file_size == (size_t)-1)
	{
		error_flag = TRUE;
		sprintf(error_id, "getmatvar:lseekFailureError");
		sprintf(error_message, "lseek failed, check errno %s\n\n", strerror(errno));
		return;
	}
	
	byte* head = navigateTo(0, MATFILE_SIG_LEN, TREE);
	if(memcmp(head, MATFILE_7_3_SIG, MATFILE_SIG_LEN) != 0)
	{
		char filetype[MATFILE_SIG_LEN];
		memcpy(filetype, head, MATFILE_SIG_LEN);
		error_flag = TRUE;
		sprintf(error_id, "getmatvar:wrongFormatError");
		if(memcmp(filetype, "MATLAB", 6) == 0)
		{
			sprintf(error_message, "The input file must be a Version 7.3+ MAT-file. This is a %s.\n\n", filetype);
		}
		else
		{
			sprintf(error_message, "The input file must be a Version 7.3+ MAT-file. This is an unknown file format.\n\n");
		}
		return;
	}
	
	num_pages = file_size/alloc_gran + 1;
	
	initializePageObjects();
	
	//find superblock
	s_block = getSuperblock();
	
	makeObjectTreeSkeleton();
	
	for(int name_index = 0; name_index < num_names; name_index++)
	{
		findAndFillVariable(variable_names[name_index]);
	}
	
	close(fd);
	if(threads_are_started == TRUE)
	{
		pthread_mutex_destroy(&thread_acquisition_lock);
		thpool_destroy(threads);
	}
	
	freeQueue(varname_queue);
	varname_queue = NULL;
	
	
	destroyPageObjects();
	freeAllMaps();

#ifdef DO_MEMDUMP
	pthread_cond_destroy(&dump_ready);
	pthread_mutex_destroy(&dump_lock);
#endif
	
	//remove unwanted/unfilled top level objects
	for(int i = super_object->num_sub_objs - 1; i >= 0; i--)
	{
		if(super_object->sub_objects[i]->is_filled == FALSE)
		{
			super_object->sub_objects[i] = NULL; //ok because objects are kept track of in a queue
			super_object->num_sub_objs--;
		}
	}

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
	page_objects = malloc(num_pages*sizeof(pageObject));
	for(int i = 0; i < num_pages; i++)
	{
		pthread_mutex_init(&page_objects[i].lock, NULL);
		//page_objects[i].ready = PTHREAD_COND_INITIALIZER;//initialize these later if we need to?
		//page_objects[i].lock = PTHREAD_MUTEX_INITIALIZER;
		page_objects[i].is_mapped = FALSE;
		page_objects[i].pg_start_a = alloc_gran*i;
		page_objects[i].pg_end_a = MIN(alloc_gran*(i + 1), file_size);
		page_objects[i].map_base = UNDEF_ADDR;
		page_objects[i].map_end = UNDEF_ADDR;
		page_objects[i].pg_start_p = NULL;
		page_objects[i].num_using = 0;
		page_objects[i].last_use_time_stamp = 0;
	}
	usage_iterator = 0;
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
				readMXError("getmatvar:badMunmapError", "munmap() unsuccessful in freeMap(). Check errno %s\n\n", strerror(errno));
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


Data* connectSubObject(Data* object, uint64_t sub_obj_address, char* name)
{
	
	Data* sub_object = malloc(sizeof(Data));
	initializeObject(sub_object);
	sub_object->this_obj_address = sub_obj_address;
	sub_object->names.short_name_length = (uint16_t)strlen(name);
	sub_object->names.short_name = malloc((sub_object->names.short_name_length + 1) * sizeof(char));
	strcpy(sub_object->names.short_name, name);
	object->sub_objects[object->num_sub_objs] = sub_object;
	object->num_sub_objs++;
	

	//append the short name to the data object long name
	if(object->names.long_name_length != 0)
	{
		//+1 because of the '.' delimiter
		sub_object->names.long_name_length = object->names.long_name_length + (uint16_t)1 + sub_object->names.short_name_length;
		sub_object->names.long_name = malloc((sub_object->names.long_name_length + 1) * sizeof(char));
		strcpy(sub_object->names.long_name, object->names.long_name);
		sub_object->names.long_name[object->names.long_name_length] = '.';
		strcpy(&sub_object->names.long_name[object->names.long_name_length + 1], sub_object->names.short_name);
	}
	else
	{
		sub_object->names.long_name_length = sub_object->names.short_name_length;
		sub_object->names.long_name = malloc((sub_object->names.long_name_length + 1) * sizeof(char));
		strcpy(sub_object->names.long_name, sub_object->names.short_name);
	}
	
	return sub_object;

}


void fillObject(Data* data_object, uint64_t this_obj_address)
{
	
	if(data_object->is_filled == TRUE)
	{
		return;
	}
	
	data_object->is_filled = TRUE;
	
	//by only asking for enough bytes to get the header length there is a chance a mapping can be reused
	byte* header_pointer = navigateTo(this_obj_address, 16, TREE);
	uint32_t header_length = (uint32_t)getBytesAsNumber(header_pointer + 8, 4, META_DATA_BYTE_ORDER);
	uint16_t num_msgs = (uint16_t)getBytesAsNumber(header_pointer + 2, 2, META_DATA_BYTE_ORDER);
	
	data_object->this_obj_address = this_obj_address;
	collectMetaData(data_object, this_obj_address, num_msgs, header_length);
	
}


void initializeObject(Data* object)
{
	
	object->is_filled = FALSE;
	object->is_finalized = FALSE;
	
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
	
	object->super_object = NULL;
	object->sub_objects = NULL;
	
	
	object->names.long_name_length = 0;
	object->names.long_name = NULL;
	object->names.short_name_length = 0;
	object->names.short_name = NULL;
	
	//zero by default for REF_DATA data
	object->layout_class = 0;
	object->datatype_bit_field = 0;
	object->byte_order = LITTLE_ENDIAN;
	object->type = UNDEF_DATA;
	object->complexity_flag = mxREAL;
	
	object->num_dims = 0;
	object->num_elems = 0;
	object->elem_size = 0;
	object->num_sub_objs = 0;
	object->data_address = UNDEF_ADDR;
	object->this_obj_address = UNDEF_ADDR;
	
}


void collectMetaData(Data* object, uint64_t header_address, uint16_t num_msgs, uint32_t header_length)
{
	
	interpretMessages(object, header_address, header_length, 0, num_msgs, 0);
	
	//TODO make this more robust at some point
	//matlab stores nulls like this
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
		error_flag = TRUE;
		sprintf(error_id, "getmatvar:unknownDataTypeError");
		sprintf(error_message, "Unknown data type encountered.\n\n");
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
			error_flag = TRUE;
			sprintf(error_id, "getmatvar:unknownLayoutClassError");
			sprintf(error_message, "Unknown layout class encountered.\n\n");
			return;
	}
	
	// we have encountered a cell array
	if(object->data_arrays.sub_object_header_offsets != NULL && object->type == REF_DATA)
	{
		object->sub_objects = malloc(object->num_elems * sizeof(Data*));
		object->num_sub_objs = object->num_elems;
		for(int i = object->num_elems - 1; i >= 0; i--)
		{
			address_t new_obj_address = object->data_arrays.sub_object_header_offsets[i] + s_block.base_address;
			//search from super_object since the reference might be in #refs#
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
			
			free(ref->names.long_name);
			ref->names.long_name_length = (uint16_t)(object->names.long_name_length + 1 + num_digits + 1);
			ref->names.long_name = malloc((ref->names.long_name_length + 1) * sizeof(char));
			sprintf(ref->names.long_name, "%s{%d}", object->names.long_name, i+1);
			ref->names.long_name[ref->names.long_name_length] = '\0';
			
		}
	}
}


uint16_t interpretMessages(Data* object, uint64_t header_address, uint32_t header_length, uint16_t message_num, uint16_t num_msgs, uint16_t repeat_tracker)
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
				message_num = interpretMessages(object, cont_header_address - 16, cont_header_length, message_num, num_msgs, repeat_tracker);
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
	
	return (uint16_t)(message_num - 1);
	
}


errno_t allocateSpace(Data* object)
{
	//maybe figure out a way to just pass this to a single array
	switch(object->type)
	{
		case INT8_DATA:
			object->data_arrays.i8_data = mxMalloc(object->num_elems*object->elem_size);
			break;
		case UINT8_DATA:
			object->data_arrays.ui8_data = mxMalloc(object->num_elems*object->elem_size);
			break;
		case INT16_DATA:
			object->data_arrays.i16_data = mxMalloc(object->num_elems*object->elem_size);
			break;
		case UINT16_DATA:
			object->data_arrays.ui16_data = mxMalloc(object->num_elems*object->elem_size);
			break;
		case INT32_DATA:
			object->data_arrays.i32_data = mxMalloc(object->num_elems*object->elem_size);
			break;
		case UINT32_DATA:
			object->data_arrays.ui32_data = mxMalloc(object->num_elems*object->elem_size);
			break;
		case INT64_DATA:
			object->data_arrays.i64_data = mxMalloc(object->num_elems*object->elem_size);
			break;
		case UINT64_DATA:
			object->data_arrays.ui64_data = mxMalloc(object->num_elems*object->elem_size);
			break;
		case SINGLE_DATA:
			object->data_arrays.single_data = mxMalloc(object->num_elems*object->elem_size);
			break;
		case DOUBLE_DATA:
			object->data_arrays.double_data = mxMalloc(object->num_elems*object->elem_size);
			break;
		case REF_DATA:
			//STORE ADDRESSES IN THE UDOUBLE_DATA ARRAY; THESE ARE NOT ACTUAL ELEMENTS
			object->data_arrays.sub_object_header_offsets = malloc(object->num_elems*object->elem_size);
			break;
		case STRUCT_DATA:
		case FUNCTION_HANDLE_DATA:
			//Don't allocate anything yet. This will be handled later
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
			error_flag = TRUE;
			sprintf(error_id, "getmatvar:thisShouldntHappen");
			sprintf(error_message, "Allocated space ran with an NULLTYPE_DATA for some reason.\n\n");
			return 1;
		
	}
	
	return 0;
	
}


void placeData(Data* object, byte* data_pointer, uint64_t starting_index, uint64_t condition, size_t elem_size, ByteOrder data_byte_order)
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
			memcpy(&object->data_arrays.i8_data[starting_index], data_pointer, (condition - starting_index)*elem_size);
			break;
		case UINT8_DATA:
			memcpy(&object->data_arrays.ui8_data[starting_index], data_pointer, (condition - starting_index)*elem_size);
			break;
		case INT16_DATA:
			memcpy(&object->data_arrays.i16_data[starting_index], data_pointer, (condition - starting_index)*elem_size);
			break;
		case UINT16_DATA:
			memcpy(&object->data_arrays.ui16_data[starting_index], data_pointer, (condition - starting_index)*elem_size);
			break;
		case INT32_DATA:
			memcpy(&object->data_arrays.i32_data[starting_index], data_pointer, (condition - starting_index)*elem_size);
			break;
		case UINT32_DATA:
			memcpy(&object->data_arrays.ui32_data[starting_index], data_pointer, (condition - starting_index)*elem_size);
			break;
		case INT64_DATA:
			memcpy(&object->data_arrays.i64_data[starting_index], data_pointer, (condition - starting_index)*elem_size);
			break;
		case UINT64_DATA:
			memcpy(&object->data_arrays.ui64_data[starting_index], data_pointer, (condition - starting_index)*elem_size);
			break;
		case SINGLE_DATA:
			memcpy(&object->data_arrays.single_data[starting_index], data_pointer, (condition - starting_index)*elem_size);
			break;
		case DOUBLE_DATA:
			memcpy(&object->data_arrays.double_data[starting_index], data_pointer, (condition - starting_index)*elem_size);
			break;
		case REF_DATA:
			memcpy(&object->data_arrays.sub_object_header_offsets[starting_index], data_pointer, (condition - starting_index)*elem_size);
			break;
		case STRUCT_DATA:
		case FUNCTION_HANDLE_DATA:
		case TABLE_DATA:
		default:
			//nothing to be done
			break;
		
	}
	
}


void placeDataWithIndexMap(Data* object, byte* data_pointer, uint64_t num_elems, size_t elem_size, ByteOrder data_byte_order, const uint64_t* index_map)
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
				memcpy(&object->data_arrays.i8_data[index_map[j]], data_pointer + object_data_index*elem_size, elem_size);
				object_data_index++;
			}
			break;
		case UINT8_DATA:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.ui8_data[index_map[j]], data_pointer + object_data_index*elem_size, elem_size);
				object_data_index++;
			}
			break;
		case INT16_DATA:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.i16_data[index_map[j]], data_pointer + object_data_index*elem_size, elem_size);
				object_data_index++;
			}
			break;
		case UINT16_DATA:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.ui16_data[index_map[j]], data_pointer + object_data_index*elem_size, elem_size);
				object_data_index++;
			}
			break;
		case INT32_DATA:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.i32_data[index_map[j]], data_pointer + object_data_index*elem_size, elem_size);
				object_data_index++;
			}
			break;
		case UINT32_DATA:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.ui32_data[index_map[j]], data_pointer + object_data_index*elem_size, elem_size);
				object_data_index++;
			}
			break;
		case INT64_DATA:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.i64_data[index_map[j]], data_pointer + object_data_index*elem_size, elem_size);
				object_data_index++;
			}
			break;
		case UINT64_DATA:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.ui64_data[index_map[j]], data_pointer + object_data_index*elem_size, elem_size);
				object_data_index++;
			}
			break;
		case SINGLE_DATA:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.single_data[index_map[j]], data_pointer + object_data_index*elem_size, elem_size);
				object_data_index++;
			}
			break;
		case DOUBLE_DATA:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				object->data_arrays.double_data[index_map[j]] = *(double*)(data_pointer + object_data_index*elem_size);
				//memcpy(&object->data_arrays.double_data[index_map[j]], data_pointer + object_data_index*elem_size, elem_size);
				object_data_index++;
			}
			break;
		case REF_DATA:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.sub_object_header_offsets[index_map[j]], data_pointer + object_data_index*elem_size, elem_size);
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


void findAndFillVariable(char* variable_name)
{
	char* delim = ".", * token;
	default_bytes = (uint64_t)getAllocGran();
	default_bytes = default_bytes < file_size? default_bytes : file_size;
	
	flushQueue(varname_queue);
	if(strcmp(variable_name, "\0") == 0)
	{
		for(int i = 0; i < super_object->num_sub_objs; i++)
		{
			if(super_object->sub_objects[i]->names.short_name[0] != '#')
			{
				fillDataTree(super_object->sub_objects[i]);
			}
		}
	}
	else
	{
		token = strtok(variable_name, delim);
		while(token != NULL)
		{
			char* vn = malloc((strlen(token) + 1)*sizeof(char));
			strcpy(vn, token);
			enqueue(varname_queue, vn);
			token = strtok(NULL, delim);
		}
		
		Data* object = super_object;
		do
		{
			object = findSubObjectByShortName(object, dequeue(varname_queue));
			if(object == NULL)
			{
				sprintf(warn_message, "Variable \'%s\' was not found.", variable_name);
				readMXWarn("getmatvar:variableNotFound", warn_message);
				return;
			}
		} while(varname_queue->length > 0);
		
		fillDataTree(object);
		
	}
	
}


/*for use on a single level*/
Data* findSubObjectByShortName(Data* object, char* name)
{
	for(int i = 0; i < object->num_sub_objs; i++)
	{
		if(strcmp(object->sub_objects[i]->names.short_name, name) == 0)
		{
			return object->sub_objects[i];
		}
	}
	return NULL;
}

/*searches entire tree*/
Data* findObjectByHeaderAddress(address_t address)
{
	
	restartQueue(object_queue);
	do
	{
		Data* object = dequeue(object_queue);
		if(object->this_obj_address == address)
		{
			return object;
		}
	} while(object_queue->length > 0);
	
	return NULL;
	
}

/*fill this super object and all below it*/
void fillDataTree(Data* object)
{
	fillObject(object, object->this_obj_address);
	if(object->type == FUNCTION_HANDLE_DATA)
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


void makeObjectTreeSkeleton(void)
{
	readTreeNode(super_object, s_block.root_tree_address, s_block.root_heap_address);
}


void readTreeNode(Data* object, uint64_t node_address, uint64_t heap_address)
{
	
	uint16_t entries_used = 0;
	byte* tree_pointer = navigateTo(node_address, default_bytes, META_DATA_BYTE_ORDER);
	
	entries_used = (uint16_t)getBytesAsNumber(tree_pointer + 6, 2, META_DATA_BYTE_ORDER);
	
	//group node B-Tree traversal (version 0)
	byte* key_pointer = tree_pointer + 8 + 2*s_block.size_of_offsets;
	
	
	//quickly get the total number of possible subobjects first so we can allocate the subobject array
	uint16_t max_num_sub_objs = 0; //may be fewer than this number by throwing away the refs and such things
	uint64_t* sub_node_address_list = malloc(entries_used * sizeof(uint64_t));
	for(int i = 0; i < entries_used; i++)
	{
		sub_node_address_list[i] = getBytesAsNumber(key_pointer + s_block.size_of_lengths, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
		max_num_sub_objs += getNumSymbols(sub_node_address_list[i]);
		key_pointer += s_block.size_of_lengths + s_block.size_of_offsets;
	}
	
	if(object->sub_objects == NULL)
	{
		object->sub_objects = malloc(max_num_sub_objs*sizeof(Data*));
	}
	
	for(int i = 0; i < entries_used; i++)
	{
		readSnod(object, sub_node_address_list[i], heap_address);
	}
	
	free(sub_node_address_list);
	
}


void readSnod(Data* object, uint64_t node_address, uint64_t heap_address)
{
	
	byte* snod_pointer = navigateTo(node_address, 8, META_DATA_BYTE_ORDER);
	uint16_t num_symbols = (uint16_t)getBytesAsNumber(snod_pointer + 6, 2, META_DATA_BYTE_ORDER);
	snod_pointer = navigateTo(node_address, s_block.sym_table_entry_size * num_symbols, META_DATA_BYTE_ORDER);
	byte* heap_pointer = navigateTo(heap_address, default_bytes, META_DATA_BYTE_ORDER);
	
	uint32_t cache_type;
	
	uint64_t heap_data_segment_size = getBytesAsNumber(heap_pointer + 8, s_block.size_of_lengths, META_DATA_BYTE_ORDER);
	uint64_t heap_data_segment_address = getBytesAsNumber(heap_pointer + 8 + 2*s_block.size_of_lengths, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
	byte* heap_data_segment_pointer = navigateTo(heap_data_segment_address, heap_data_segment_size, HEAP);
	
	for(int i = num_symbols - 1; i >= 0; i--)
	{
		uint64_t name_offset = getBytesAsNumber(snod_pointer + 8 + i*s_block.sym_table_entry_size, s_block.size_of_offsets, META_DATA_BYTE_ORDER);
		uint64_t sub_obj_address = getBytesAsNumber(snod_pointer + 8 + i*s_block.sym_table_entry_size + s_block.size_of_offsets, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
		char* name = (char*)(heap_data_segment_pointer + name_offset);
		cache_type = (uint32_t)getBytesAsNumber(snod_pointer + 8 + i*s_block.sym_table_entry_size + 2*s_block.size_of_offsets, 4, META_DATA_BYTE_ORDER);
		
		Data* sub_object = connectSubObject(object, sub_obj_address, name);
		enqueue(object_queue, sub_object);
		
		//if the variable has been found we should keep going down the tree for that variable
		//all items in the queue should only be subobjects so this is safe
		if(cache_type == 1)
		{
			uint64_t sub_tree_address =
					getBytesAsNumber(snod_pointer + 8 + i*s_block.sym_table_entry_size + 2*s_block.size_of_offsets + 8, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
			uint64_t sub_heap_address =
					getBytesAsNumber(snod_pointer + 8 + i*s_block.sym_table_entry_size + 3*s_block.size_of_offsets + 8, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
			
			//there is a specific subtree that we need to parse for function handles
			readTreeNode(sub_object, sub_tree_address, sub_heap_address);
			
			snod_pointer = navigateTo(node_address, default_bytes, TREE);
			heap_data_segment_pointer = navigateTo(heap_data_segment_address, heap_data_segment_size, HEAP);
			
		}
		else if(cache_type == 2)
		{
			//this object is a symbolic link, the name is stored in the heap at the address indicated in the scratch pad
			name_offset = getBytesAsNumber(snod_pointer + 8 + 2*s_block.size_of_offsets + 8 + s_block.sym_table_entry_size*i, 4, META_DATA_BYTE_ORDER);
			name = (char*)(heap_pointer + 8 + 2*s_block.size_of_lengths + s_block.size_of_offsets + name_offset);
		}
		
		
		
		
	}
	
}


uint32_t getNumSymbols(uint64_t address)
{
	byte* p = navigateTo(address, 8, TREE);
	return (uint16_t)getBytesAsNumber(p + 6, 2, META_DATA_BYTE_ORDER);
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
	
	//dont use strchr because we need to know the length of the copied string
	uint32_t fh_len = fh_data->num_elems;
	char* func_name = NULL;
	for(; fh_len > 0; fh_len--)
	{
		func_name = (char*)&fh_data->data_arrays.ui16_data[fh_data->num_elems - fh_len];
		if(*func_name == '@')
		{
			break;
		}
	}
	
	if(fh_len == 0)
	{
		//need to add the '@' character
		fh->data_arrays.ui16_data = mxMalloc((1 + fh_data->num_elems) * fh_data->elem_size);
		fh->data_arrays.ui16_data[0] = 64; //this is the '@' character
		memcpy(&fh->data_arrays.ui16_data[1], fh_data->data_arrays.ui16_data, fh_data->num_elems*fh_data->elem_size);
		fh->num_elems = 1 + fh_data->num_elems; //+ 1 for the seperator
	}
	else
	{
		fh->data_arrays.ui16_data = mxMalloc((fh_len) * fh_data->elem_size);
		memcpy(fh->data_arrays.ui16_data, func_name, fh_len * fh_data->elem_size);
		fh->num_elems = fh_len;
	}
	
	fh->num_dims = 2;
	fh->dims[0] = 1;
	fh->dims[1] = fh->num_elems;
	fh->dims[2] = 0;
	
}


void freeVarname(void* vn)
{
	char* varname = (char*)vn;
	if(varname != NULL && strcmp(varname, "\0") != 0)
	{
		free(varname);
	}
}


/*this intializer should be called in the entry function before anything else */
void initialize(void)
{
	//init maps, needed so we don't confuse ending hooks in the case of error
	initializeMaps();
	varname_queue = NULL;
	object_queue = NULL;
	eval_objects = NULL;
	fd = -1;
	num_threads_to_use = -1;
	will_multithread = TRUE;
	will_suppress_warnings = FALSE;
	max_depth = 0;
	error_flag = FALSE;
	super_object = NULL;
	parameters.full_variable_names = NULL;
	parameters.filename = NULL;
	parameters.num_vars = 0;
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
	fprintf(stdout, message_buffer);
	exit(1);
#else
	mexErrMsgIdAndTxt(error_id, message_buffer);
#endif

}


void readMXWarn(const char warn_id[], const char warn_message[], ...)
{
	
	if(will_suppress_warnings != TRUE)
	{
		char message_buffer[WARNING_BUFFER_SIZE];
		
		va_list va;
		va_start(va, warn_message);
		sprintf(message_buffer, warn_message, va);
		strcat(message_buffer, MATLAB_WARN_MESSAGE);
		va_end(va);

#ifdef NO_MEX
		fprintf(stdout, message_buffer);
#else
		mexWarnMsgIdAndTxt(warn_id, message_buffer);
#endif
	
	}
	
}