#include "mapping.h"
#include <stdarg.h>


Queue* getDataObjects(const char* filename, char** variable_names, int num_names)
{

	__byte_order__ = getByteOrder();

	Queue* objects = initQueue(freeDataObject);
	SNODEntry* snod_entry;
	char errmsg[NAME_LENGTH];
	num_avail_threads = getNumProcessors() - 1;
	
	initializeMaps();
	is_multithreading = FALSE;
	addr_queue = NULL;
	varname_queue = NULL;
	header_queue = NULL;
	fd = -1;
	num_threads_to_use = -1;

	//init queues
	addr_queue = initQueue(NULL);
	varname_queue = initQueue(NULL);
	header_queue = initQueue(NULL);
	
	//open the file descriptor
	fd = open(filename, O_RDONLY);
	if(fd < 0)
	{
		Data* data_object = malloc(sizeof(Data));
		initializeObject(data_object);
		data_object->type = ERROR | END_SENTINEL;
		sprintf(data_object->name, "getmatvar:fileNotFoundError");
		sprintf(data_object->matlab_class, "No file found with name \'%s\'.\n\n", filename);
		enqueue(objects, data_object);
		return objects;
	}
	
	//get file size
	file_size = (size_t)lseek(fd, 0, SEEK_END);
	if(file_size == (size_t)-1)
	{
		Data* data_object = malloc(sizeof(Data));
		initializeObject(data_object);
		data_object->type = ERROR | END_SENTINEL;
		sprintf(data_object->name, "getmatvar:lseekFailedError");
		sprintf(data_object->matlab_class, "lseek failed, check errno %s\n\n", strerror(errno));
		return objects;
	}
	
	//find superblock
	s_block = getSuperblock();
	
	bool_t load_all = FALSE;
	for(int i = 0; i < num_names; i++)
	{
		if(strcmp(variable_names[i],"\0") == 0)
		{
			findHeaderAddress(variable_names[i], TRUE);
			load_all = TRUE;
			break;
		}
	}

	if(load_all)
	{
		for (int i = 0; i < num_names; i++)
		{
			if(variable_names[i] != NULL && strcmp(variable_names[i], "\0") != 0)
			{
				free(variable_names[i]);
			}
		}

		num_names = header_queue->length;

		variable_names = malloc(num_names * sizeof(char*));
		for(int j = 0; j < num_names; j++)
		{
			snod_entry = dequeue(header_queue);
			variable_names[j] = malloc(strlen(snod_entry->name) * sizeof(char));
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
			data_object->type = UNDEF;
			strcpy(data_object->name, peekQueue(varname_queue, QUEUE_BACK));
			enqueue(objects, data_object);
			sprintf(errmsg, "Variable \'%s\' was not found.", variable_names[name_index]);
			readMXWarn("getmatvar:variableNotFound", errmsg);
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
			uint32_t header_length = (uint32_t)getBytesAsNumber(header_pointer + 8, 4, META_DATA_BYTE_ORDER);
			uint16_t num_msgs = (uint16_t)getBytesAsNumber(header_pointer + 2, 2, META_DATA_BYTE_ORDER);
			
			strcpy(data_object->name, snod_entry->name);
			data_object->parent_obj_address = snod_entry->parent_obj_header_address;
			data_object->this_obj_address = snod_entry->this_obj_header_address;
			collectMetaData(data_object, header_address, num_msgs, header_length);
			enqueue(objects, data_object);
			
			//this block handles errors and sets up signals for controlled shutdown
			if(data_object->type == ERROR)
			{
				Data* front_object = peekQueue(objects, QUEUE_FRONT);
				front_object->type = ERROR;
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
	
	if(load_all)
	{
		for (int i = 0; i < num_names; i++)
		{
			free(variable_names[i]);
		}
		free(variable_names);
	}

	close(fd);
	freeQueue(addr_queue);
	freeQueue(varname_queue);
	freeQueue(header_queue);
	freeAllMaps();
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
	
	for(int i = 0; i < NUM_THREAD_MAPS; i++)
	{
		thread_maps[i].used = FALSE;
		thread_maps[i].bytes_mapped = 0;
		thread_maps[i].map_start = NULL;
		thread_maps[i].offset = 0;
	}
	
	map_nums[TREE] = NUM_TREE_MAPS;
	map_nums[HEAP] = NUM_HEAP_MAPS;
	map_nums[THREAD] = NUM_THREAD_MAPS;
	map_queue_fronts[TREE] = 0;
	map_queue_fronts[HEAP] = 0;
	map_queue_fronts[THREAD] = 0;
	
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
	
	object->sub_objects = NULL;
	
	//zero by default for REF data
	object->layout_class = 0;
	object->datatype_bit_field = 0;
	object->byte_order = LITTLE_ENDIAN;
	object->type = UNDEF;
	object->complexity_flag = 0;
	
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
	
	
	if(object->type == UNDEF)
	{
		object->type = ERROR;
		sprintf(object->name, "getmatvar:unknownDataTypeError");
		sprintf(object->matlab_class, "Unknown data type encountered.\n\n");
		return;
	}
	
	//allocate space for data
	if(allocateSpace(object) != 0){return;}
	
	
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
			object->type = ERROR;
			sprintf(object->name, "getmatvar:unknownLayoutClassError");
			sprintf(object->matlab_class, "Unknown layout class encountered.\n\n");
			return;
	}
	
	//if we have encountered a cell array, queue up headers for its elements
	if(object->data_arrays.sub_object_header_offsets != NULL && object->type == REF)
	{
		
		for(int i = object->num_elems - 1; i >= 0; i--)
		{
			SNODEntry* snod_entry = malloc(sizeof(SNODEntry));
			snod_entry->this_obj_header_address = object->data_arrays.sub_object_header_offsets[i] + s_block.base_address;
			snod_entry->parent_obj_header_address = object->this_obj_address;
			strcpy(snod_entry->name, object->name);
			priorityEnqueue(header_queue, snod_entry);
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
	
	return (uint16_t)(message_num - 1);
	
}


errno_t allocateSpace(Data* object)
{
	//maybe figure out a way to just pass this to a single array
	switch(object->type)
	{
		case INT8:
			object->data_arrays.i8_data = malloc(object->num_elems * object->elem_size);
			break;
		case UINT8:
			object->data_arrays.ui8_data = malloc(object->num_elems * object->elem_size);
			break;
		case INT16:
			object->data_arrays.i16_data = malloc(object->num_elems * object->elem_size);
			break;
		case UINT16:
			object->data_arrays.ui16_data = malloc(object->num_elems * object->elem_size);
			break;
		case INT32:
			object->data_arrays.i32_data = malloc(object->num_elems * object->elem_size);
			break;
		case UINT32:
			object->data_arrays.ui32_data = malloc(object->num_elems * object->elem_size);
			break;
		case INT64:
			object->data_arrays.i64_data = malloc(object->num_elems * object->elem_size);
			break;
		case UINT64:
			object->data_arrays.ui64_data = malloc(object->num_elems * object->elem_size);
			break;
		case SINGLE:
			object->data_arrays.single_data = malloc(object->num_elems * object->elem_size);
			break;
		case DOUBLE:
			object->data_arrays.double_data = malloc(object->num_elems * object->elem_size);
			break;
		case REF:
			//STORE ADDRESSES IN THE UDOUBLE_DATA ARRAY; THESE ARE NOT ACTUAL ELEMENTS
			object->data_arrays.sub_object_header_offsets = malloc(object->num_elems * object->elem_size);
			break;
		case STRUCT:
		case FUNCTION_HANDLE:
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
			object->type = ERROR;
			sprintf(object->name, "getmatvar:thisShouldntHappen");
			sprintf(object->matlab_class, "Allocated space ran with an NULLTYPE for some reason.\n\n");
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
		case INT8:
			memcpy(&object->data_arrays.i8_data[starting_index], data_pointer, (condition - starting_index) * elem_size);
			break;
		case UINT8:
			memcpy(&object->data_arrays.ui8_data[starting_index], data_pointer, (condition - starting_index) * elem_size);
			break;
		case INT16:
			memcpy(&object->data_arrays.i16_data[starting_index], data_pointer, (condition - starting_index) * elem_size);
			break;
		case UINT16:
			memcpy(&object->data_arrays.ui16_data[starting_index], data_pointer, (condition - starting_index) * elem_size);
			break;
		case INT32:
			memcpy(&object->data_arrays.i32_data[starting_index], data_pointer, (condition - starting_index) * elem_size);
			break;
		case UINT32:
			memcpy(&object->data_arrays.ui32_data[starting_index], data_pointer, (condition - starting_index) * elem_size);
			break;
		case INT64:
			memcpy(&object->data_arrays.i64_data[starting_index], data_pointer, (condition - starting_index) * elem_size);
			break;
		case UINT64:
			memcpy(&object->data_arrays.ui64_data[starting_index], data_pointer, (condition - starting_index) * elem_size);
			break;
		case SINGLE:
			memcpy(&object->data_arrays.single_data[starting_index], data_pointer, (condition - starting_index) * elem_size);
			break;
		case DOUBLE:
			memcpy(&object->data_arrays.double_data[starting_index], data_pointer, (condition - starting_index) * elem_size);
			break;
		case REF:
			memcpy(&object->data_arrays.sub_object_header_offsets[starting_index], data_pointer, (condition - starting_index) * elem_size);
			break;
		case STRUCT:
		case FUNCTION_HANDLE:
		case TABLE:
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
		case INT8:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.i8_data[index_map[j]], data_pointer + object_data_index * elem_size, elem_size);
				object_data_index++;
			}
			break;
		case UINT8:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.ui8_data[index_map[j]], data_pointer + object_data_index * elem_size, elem_size);
				object_data_index++;
			}
			break;
		case INT16:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.i16_data[index_map[j]], data_pointer + object_data_index * elem_size, elem_size);
				object_data_index++;
			}
			break;
		case UINT16:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.ui16_data[index_map[j]], data_pointer + object_data_index * elem_size, elem_size);
				object_data_index++;
			}
			break;
		case INT32:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.i32_data[index_map[j]], data_pointer + object_data_index * elem_size, elem_size);
				object_data_index++;
			}
			break;
		case UINT32:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.ui32_data[index_map[j]], data_pointer + object_data_index * elem_size, elem_size);
				object_data_index++;
			}
			break;
		case INT64:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.i64_data[index_map[j]], data_pointer + object_data_index * elem_size, elem_size);
				object_data_index++;
			}
			break;
		case UINT64:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.ui64_data[index_map[j]], data_pointer + object_data_index * elem_size, elem_size);
				object_data_index++;
			}
			break;
		case SINGLE:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.single_data[index_map[j]], data_pointer + object_data_index * elem_size, elem_size);
				object_data_index++;
			}
			break;
		case DOUBLE:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.double_data[index_map[j]], data_pointer + object_data_index * elem_size, elem_size);
				object_data_index++;
			}
			break;
		case REF:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.sub_object_header_offsets[index_map[j]], data_pointer + object_data_index * elem_size, elem_size);
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


void findHeaderAddress(char* variable_name, bool_t get_top_level)
{
	char* delim = ".",* token;
	AddrTrio* root_trio_copy = malloc(sizeof(AddrTrio));
	root_trio_copy->tree_address = root_trio.tree_address;
	root_trio_copy->heap_address = root_trio.heap_address;
	root_trio_copy->parent_obj_header_address = root_trio.parent_obj_header_address;
	enqueue(addr_queue, root_trio_copy);
	default_bytes = (uint64_t)getAllocGran();
	default_bytes = default_bytes < file_size ? default_bytes : file_size;
	
	flushQueue(varname_queue);
	if(strcmp(variable_name,"\0") == 0)
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
			char* vn = malloc(sizeof(token));
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
	AddrTrio* parent_trio,* this_trio;
	
	//search for the object header for the variable
	while(addr_queue->length > 0)
	{
		this_trio = peekQueue(addr_queue, QUEUE_FRONT);
		tree_pointer = navigateTo(this_trio->tree_address, default_bytes, TREE);
		heap_pointer = navigateTo(this_trio->heap_address, default_bytes, HEAP);
		
		if(strncmp("TREE", (char*)tree_pointer, 4) == 0)
		{
			parent_trio = dequeue(addr_queue);
			readTreeNode(tree_pointer, parent_trio);
		}
		else if(strncmp("SNOD", (char*)tree_pointer, 4) == 0)
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
	
	if(super_object->type == STRUCT || super_object->type == REF)
	{
		placeInSuperObject(super_object, objects, objects->length);
	}
	
	return super_object;
	
}


void placeInSuperObject(Data* super_object, Queue* objects, int num_objs_left)
{
	//note: index should be at the starting index of the subobjects
	
	super_object->num_sub_objs = 0;
	super_object->sub_objects = malloc(num_objs_left* sizeof(Data*));
	Data* curr = dequeue(objects);
	
	while(super_object->this_obj_address == curr->parent_obj_address)
	{
		super_object->sub_objects[super_object->num_sub_objs] = curr;
		if(super_object->sub_objects[super_object->num_sub_objs]->type == STRUCT || super_object->sub_objects[super_object->num_sub_objs]->type == REF)
		{
			//since this is a depth-first traversal
			placeInSuperObject(super_object->sub_objects[super_object->num_sub_objs], objects, objects->length);
		}
		curr = dequeue(objects);
		super_object->num_sub_objs++;
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
	printf("%s",message_buffer);
	exit(1);
}

void readMXWarn(const char warn_id[], const char warn_message[], ...)
{
	char message_buffer[WARNING_BUFFER_SIZE];

	va_list va;
	va_start(va, warn_message);
	sprintf(message_buffer, warn_message, va);
	strcat(message_buffer, MATLAB_WARN_MESSAGE);
	va_end(va);
	printf("%s",message_buffer);
	exit(1);
}
