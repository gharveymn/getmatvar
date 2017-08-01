#include "mapping.h"

Data* mapping (char* filename, char variable_name[])
{
	//char* filename = "my_struct.mat";
	//char variable_name[] = "cell"; //must be [] not * in order for strtok() to work
	printf("Object header for variable %s is at 0x", variable_name);
	char* delim = ".";
	char* tree_pointer;
	char* heap_pointer;
	char* token;

	uint64_t header_address = 0;

	is_string = FALSE;

	token = strtok(variable_name, delim);

	//init maps
	maps[0].used = FALSE;
	maps[1].used = FALSE;

	//init queue
	flushQueue();

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
		tree_pointer = navigateTo(queue.pairs[queue.front].tree_address, TREE);
		heap_pointer = navigateTo(queue.pairs[queue.front].heap_address, HEAP);
		assert(strncmp("HEAP", heap_pointer, 4) == 0);

		if (strncmp("TREE", tree_pointer, 4) == 0)
		{
			readTreeNode(tree_pointer);
			dequeuePair();
		}
		else if (strncmp("SNOD", tree_pointer, 4) == 0)
		{
			header_address = readSnod(tree_pointer, heap_pointer, token);

			token = strtok(NULL, delim);
			if (token == NULL)
			{
				break;
			}

			if (header_address == UNDEF_ADDR)
			{
				dequeuePair();
			}
		}
	}
	printf("%lx\n", header_address);
	enqueueAddress(header_address);

	Data* data_objects = (Data *)malloc(MAX_OBJS*sizeof(Data));
	int num_objs = 0;

	//interpret the header messages
	char* header_pointer;
	uint16_t num_msgs;
	uint16_t msg_type = 0;
	uint16_t msg_size = 0;
	uint64_t bytes_read;
	int num_elems = 1;
	int elem_size;
	int index;
	uint8_t layout_class;
	char* msg_pointer;
	char* data_pointer;
	uint16_t name_size, datatype_size, dataspace_size;
	uint32_t attribute_data_size;
	char name[20];

	while (header_queue.length > 0)
	{
		data_objects[num_objs].double_data = NULL;
		data_objects[num_objs].ushort_data = NULL;
		data_objects[num_objs].udouble_data = NULL;
		data_objects[num_objs].char_data = NULL;
		header_address = dequeueAddress();
		uint64_t msg_address = 0;
		header_pointer = navigateTo(header_address, TREE);
		num_msgs = getBytesAsNumber(header_pointer + 2, 2);

		bytes_read = 0;

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
					data_objects[num_objs].dims = readDataSpaceMessage(msg_pointer, msg_size);
					
					index = 0;
					while (data_objects[num_objs].dims[index] > 0)
					{
						num_elems *= data_objects[num_objs].dims[index];
						index++;
					}
					break;
				case 3:
					// Datatype message
					data_objects[num_objs].type = readDataTypeMessage(msg_pointer, msg_size);
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
						strncpy(data_objects[num_objs].matlab_class, msg_pointer + 8 + roundUp(name_size) + roundUp(datatype_size) + roundUp(dataspace_size), attribute_data_size);
						data_objects[num_objs].matlab_class[attribute_data_size] = 0x0;
					}
				default:
					//ignore message
					;
			}
			bytes_read += msg_size + 8;		
		}

		//allocate space for data
		switch(data_objects[num_objs].type)
		{
			case DOUBLE:
				data_objects[num_objs].double_data = (double *)malloc(num_elems*sizeof(double));
				elem_size = sizeof(double);
				break;
			case UINT16_T:
				data_objects[num_objs].ushort_data = (uint16_t *)malloc(num_elems*sizeof(uint16_t));
				elem_size = sizeof(uint16_t);
				break;
			case REF:
				data_objects[num_objs].udouble_data = (uint64_t *)malloc(num_elems*sizeof(uint64_t));
				elem_size = sizeof(uint64_t);
				break;
			case CHAR:
				data_objects[num_objs].char_data = (char *)malloc(num_elems*sizeof(char));
				elem_size = sizeof(char);
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
					if (data_objects[num_objs].double_data != NULL)
					{
						data_objects[num_objs].double_data[j] = convertHexToFloatingPoint(getBytesAsNumber(data_pointer + j*elem_size, elem_size));
					}
					else if (data_objects[num_objs].ushort_data != NULL)
					{
						data_objects[num_objs].ushort_data[j] = getBytesAsNumber(data_pointer + j*elem_size, elem_size);
					}
					else if (data_objects[num_objs].udouble_data != NULL)
					{
						data_objects[num_objs].udouble_data[j] = getBytesAsNumber(data_pointer + j*elem_size, elem_size) + s_block.base_address; //these are addresses so we have to add the offset
					}
					else if (data_objects[num_objs].char_data != NULL)
					{
						data_objects[num_objs].char_data[j] = getBytesAsNumber(data_pointer + j*elem_size, elem_size);
					}
				}
				break;
			default:
				printf("Unknown Layout class encountered with header at address 0x%lx\n", header_address);
				exit(EXIT_FAILURE);
		}
		if (data_objects[num_objs].udouble_data != NULL && data_objects[num_objs].type == REF)
		{
			for (int i = 0; i < num_elems; i++)
			{
				enqueueAddress(data_objects[num_objs].udouble_data[i]);
			}
		} 
		num_objs++;
	}

	return data_objects;
}
