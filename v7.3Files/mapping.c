#include "mapping.h"

Data* getDataObject(char* filename, char variable_name[])
{
	char *header_pointer;
	uint32_t header_length;
	uint64_t header_address;
	int num_objs = 0;
	Data* data_objects = (Data *)malloc(MAX_OBJS*sizeof(Data));
	Object obj;
	

	//init maps
	maps[0].used = FALSE;
	maps[1].used = FALSE;

	//init queue
	flushQueue();
	flushHeaderQueue();

	printf("Object header for variable %s is at 0x", variable_name);
	findHeaderAddress(filename, variable_name);
	printf("%lx\n", header_queue.objects[header_queue.front].obj_header_address);


	//interpret the header messages
	while (header_queue.length > 0)
	{
		obj = dequeueObject();
		header_address = obj.obj_header_address;
		

		//by only asking for enough bytes to get the header length there is a chance a mapping can be reused
		header_pointer = navigateTo(header_address, 16, TREE);

		//prevent error due to crossing of a page boundary
		header_length = getBytesAsNumber(header_pointer + 8, 4);
		if (header_address + header_length >= maps[TREE].offset + maps[TREE].bytes_mapped)
		{
			header_pointer = navigateTo(header_address, header_length, TREE);
		}
		data_objects[num_objs] = *collectMetaData(header_address, header_pointer);
		strcpy(data_objects[num_objs].name, obj.name);
		
		num_objs++;
	}

	return data_objects;
}

Data* collectMetaData(uint64_t header_address, char* header_pointer)
{
	Data *object = (Data *)malloc(sizeof(Data));
	object->double_data = NULL;
	object->udouble_data = NULL;
	object->char_data = NULL;
	object->ushort_data = NULL;
	
	uint16_t num_msgs = getBytesAsNumber(header_pointer + 2, 2);

	uint8_t layout_class;
	uint16_t name_size, datatype_size, dataspace_size;
	uint16_t msg_type = 0;
	uint16_t msg_size = 0;
	uint32_t attribute_data_size, header_length;
	uint64_t msg_address = 0;
	char* msg_pointer, *data_pointer;
	int index, num_elems = 1;
	char name[NAME_LENGTH];
	int elem_size;

	uint64_t bytes_read = 0;

	//interpret messages in header
	for (int i = 0; i < num_msgs; i++)
	{
		msg_type = getBytesAsNumber(header_pointer + 16 + bytes_read, 2);
		msg_address = header_address + 16 + bytes_read;
		msg_size = getBytesAsNumber(header_pointer + 16 + bytes_read + 2, 2);
		msg_pointer = header_pointer + 16 + bytes_read + 8;
		msg_address = header_address + 16 + bytes_read + 8;

		switch(msg_type)
		{
			case 1: 
				// Dataspace message
				object->dims = readDataSpaceMessage(msg_pointer, msg_size);
					
				index = 0;
				while (object->dims[index] > 0)
				{
					num_elems *= object->dims[index];
					index++;
				}
				break;
			case 3:
				// Datatype message
				object->type = readDataTypeMessage(msg_pointer, msg_size);
				break;
			case 8:
				// Data Layout message
				//assume version 3
				layout_class = *(msg_pointer + 1);
				data_pointer = msg_pointer + 4;
				break;
			case 12:
				//attribute message
				name_size = getBytesAsNumber(msg_pointer + 2, 2);
				datatype_size = getBytesAsNumber(msg_pointer + 4, 2);
				dataspace_size = getBytesAsNumber(msg_pointer + 6, 2);
				strncpy(name, msg_pointer + 8, name_size);
				if (strncmp(name, "MATLAB_class", 11) == 0)
				{
					attribute_data_size = getBytesAsNumber(msg_pointer + 8 + roundUp(name_size) + 4, 4);
					strncpy(object->matlab_class, msg_pointer + 8 + roundUp(name_size) + roundUp(datatype_size) + roundUp(dataspace_size), attribute_data_size);
					object->matlab_class[attribute_data_size] = 0x0;
					if(strcmp("struct", object->matlab_class) == 0)
					{
						object->type = STRUCT;
					}
				}
				break;
			case 16:
				//object header continuation message
				header_address = getBytesAsNumber(msg_pointer, s_block.size_of_offsets) + s_block.base_address;
				header_length = getBytesAsNumber(msg_pointer + s_block.size_of_offsets, s_block.size_of_lengths);
				header_pointer = navigateTo(header_address - 16, header_length + 16, TREE);
				bytes_read =  0 - msg_size - 8;
			default:
				//ignore message
				;
		}
			
		bytes_read += msg_size + 8;	
	}

	//allocate space for data
	switch(object->type)
	{
		case DOUBLE:
			object->double_data = (double *)malloc(num_elems*sizeof(double));
			elem_size = sizeof(double);
			break;
		case UINT16_T:
			object->ushort_data = (uint16_t *)malloc(num_elems*sizeof(uint16_t));
			elem_size = sizeof(uint16_t);
			break;
		case REF:
			object->udouble_data = (uint64_t *)malloc(num_elems*sizeof(uint64_t));
			elem_size = sizeof(uint64_t);
			break;
		case CHAR:
			object->char_data = (char *)malloc(num_elems*sizeof(char));
			elem_size = sizeof(char);
			break;
		case STRUCT:
			//
			break;
		default:
			printf("Unknown data type encountered with header at address 0x%lx\n", header_address);
			exit(EXIT_FAILURE);
	}

	//fetch data
	switch(layout_class)
	{
		case 0:
			//compact storage
			for (int j = 0; j < num_elems; j++)
			{
				if (object->double_data != NULL)
				{
					object->double_data[j] = convertHexToFloatingPoint(getBytesAsNumber(data_pointer + j*elem_size, elem_size));
				}
				else if (object->ushort_data != NULL)
				{
					object->ushort_data[j] = getBytesAsNumber(data_pointer + j*elem_size, elem_size);
				}
				else if (object->udouble_data != NULL)
				{
					object->udouble_data[j] = getBytesAsNumber(data_pointer + j*elem_size, elem_size) + s_block.base_address; //these are addresses so we have to add the offset
				}
				else if (object->char_data != NULL)
				{
					object->char_data[j] = getBytesAsNumber(data_pointer + j*elem_size, elem_size);
				}
			}
			break;
		default:
			printf("Unknown Layout class encountered with header at address 0x%lx\n", header_address);
			exit(EXIT_FAILURE);
	}

	//if we have encountered a cell array, queue up headers for its elements
	if (object->udouble_data != NULL && object->type == REF)
	{
		for (int i = 0; i < num_elems; i++)
		{
			Object obj;
			obj.obj_header_address = object->udouble_data[i];
			strcpy(obj.name, "__CELL");
			enqueueObject(obj);
		}
	}
	return object;
} 

void findHeaderAddress(char* filename, char variable_name[])
{
	//printf("Object header for variable %s is at ", variable_name);
	char* delim = ".";
	char* tree_pointer;
	char* heap_pointer;
	char* token;

	//uint64_t header_address = 0;

	default_bytes = sysconf(_SC_PAGE_SIZE);

	token = strtok(variable_name, delim);
	//open the file descriptor
	fd = open(filename, O_RDWR);
	if (fd < 0)
	{
		printf("open() unsuccessful, Check errno: %d\n", errno);
		exit(EXIT_FAILURE);
	}

	//get file size
	size_t file_size = lseek(fd, 0, SEEK_END);

	//find superblock
	s_block = getSuperblock(fd, file_size);

	//search for the object header for the variable
	while (queue.length > 0)
	{
		tree_pointer = navigateTo(queue.pairs[queue.front].tree_address, default_bytes, TREE);
		heap_pointer = navigateTo(queue.pairs[queue.front].heap_address, default_bytes, HEAP);
		assert(strncmp("HEAP", heap_pointer, 4) == 0);

		if (strncmp("TREE", tree_pointer, 4) == 0)
		{
			readTreeNode(tree_pointer);
			dequeuePair();
		}
		else if (strncmp("SNOD", tree_pointer, 4) == 0)
		{
			dequeuePair();
			readSnod(tree_pointer, heap_pointer, token);

			token = strtok(NULL, delim);
		}
	}
	//printf("0x%lx\n", header_address);
}
