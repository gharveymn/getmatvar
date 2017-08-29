#include "mapping.h"


Data *getDataObject(char *filename, char variable_name[], int *num_objects)
{
	char *header_pointer;
	uint32_t header_length;
	uint64_t header_address, root_tree_address;
	int num_objs = 0;
	Data *data_objects = (Data *) malloc((MAX_OBJS+1) * sizeof(Data));
	Object obj;
	
	//init maps
	maps[0].used = FALSE;
	maps[1].used = FALSE;
	
	//init queue
	flushQueue();
	flushHeaderQueue();
	
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
	
	root_tree_address = queue.trios[queue.front].tree_address;
	
	printf("\nObject header for variable %s is at 0x", variable_name);
	findHeaderAddress(filename, variable_name);
	printf("%lx\n", header_queue.objects[header_queue.front].this_obj_header_address);
	
	
	//interpret the header messages
	while (header_queue.length > 0)
	{
		
		data_objects[num_objs].sub_objects = NULL;
		
		obj = dequeueObject();
		header_address = obj.this_obj_header_address;
		
		//by only asking for enough bytes to get the header length there is a chance a mapping can be reused
		header_pointer = navigateTo(header_address, 16, TREE);
		
		//prevent error due to crossing of a page boundary
		header_length = getBytesAsNumber(header_pointer + 8, 4);
		if (header_address + header_length >= maps[TREE].offset + maps[TREE].bytes_mapped)
		{
			header_pointer = navigateTo(header_address, header_length, TREE);
		}
		strcpy(data_objects[num_objs].name, obj.name);
		data_objects[num_objs].parent_obj_address = obj.parent_obj_header_address;
		data_objects[num_objs].this_obj_address = obj.this_obj_header_address;
		collectMetaData(&data_objects[num_objs], header_address, header_pointer);
		
		num_objs++;
		
	}
	
	//set object at the end to trigger sentinel
	data_objects[num_objs].type = UNDEF;
	data_objects[num_objs].parent_obj_address = UNDEF_ADDR;
	
	num_objects[0] = num_objs;
	close(fd);
	return data_objects;
}


void collectMetaData(Data *object, uint64_t header_address, char *header_pointer)
{
	object->type = UNDEF;
	object->double_data = NULL;
	object->udouble_data = NULL;
	object->char_data = NULL;
	object->ushort_data = NULL;
	
	uint16_t num_msgs = getBytesAsNumber(header_pointer + 2, 2);
	
	uint8_t layout_class = 0;
	uint16_t name_size, datatype_size, dataspace_size;
	uint16_t msg_type = 0;
	uint16_t msg_size = 0;
	uint32_t attribute_data_size, header_length;
	uint64_t msg_address = 0;
	uint64_t data_address = 0;
	char *msg_pointer, *data_pointer;
	int index, num_elems = 1;
	char name[NAME_LENGTH];
	int elem_size;
	int num_chunked_dims;
	int32_t* chunked_dim_sizes;
	
	Filter* filters;
	char* helper_pointer;
	int num_filters;
	
	uint64_t bytes_read = 0;
	
	//interpret messages in header
	for (int i = 0; i < num_msgs; i++)
	{
		msg_type = getBytesAsNumber(header_pointer + 16 + bytes_read, 2);
		msg_address = header_address + 16 + bytes_read;
		msg_size = getBytesAsNumber(header_pointer + 16 + bytes_read + 2, 2);
		msg_pointer = header_pointer + 16 + bytes_read + 8;
		msg_address = header_address + 16 + bytes_read + 8;
		
		switch (msg_type)
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
				if(*msg_pointer != 3)
				{
					printf("Data layout version at address 0x%lx is %d; expected version 3.\n", msg_address, *msg_pointer);
					exit(EXIT_FAILURE);
				}
				
				layout_class = *(msg_pointer + 1);
				switch(layout_class)
				{
					case 0:
						data_pointer = msg_pointer + 4;
						break;
					case 1:
						data_address = getBytesAsNumber(msg_pointer + 2, s_block.size_of_offsets) +  s_block.base_address;
						data_pointer = msg_pointer + (data_address - msg_address);
						break;
					case 2:
						num_chunked_dims = *(msg_pointer+2)-1; //??
						chunked_dim_sizes = malloc(num_chunked_dims * sizeof(uint32_t));
						data_address = getBytesAsNumber(msg_pointer + 3, s_block.size_of_offsets) + s_block.base_address;
						data_pointer = msg_pointer + (data_address - msg_address);
						for (int j = 0; j < num_chunked_dims; j++)
						{
							chunked_dim_sizes[j] = getBytesAsNumber(msg_pointer + 3 + s_block.size_of_offsets + 4*j, 4);
						}
						break;
					default:
						printf("Unknown data layout class %d at address 0x%lx.\n", layout_class, msg_address + 1);
						exit(EXIT_FAILURE);
				}
				break;
			case 11:
				//data storage pipeline message
				
				num_filters = *(msg_pointer + 1);
				filters = malloc(num_filters * sizeof(Filter));
				
				//version number
				switch(*msg_pointer)
				{
					case 1:
						helper_pointer = msg_pointer + 8;
						
						for (int j = 0; j < num_filters; j++)
						{
							filters[j].filter_id = getBytesAsNumber(helper_pointer, 2);
							name_size = getBytesAsNumber(helper_pointer + 2, 2);
							filters[j].optional_flag = getBytesAsNumber(helper_pointer + 4, 2) & 1;
							filters[j].num_client_vals = getBytesAsNumber(helper_pointer + 6, 2);
							filters[j].client_data = malloc(filters[j].num_client_vals * sizeof(uint32_t));
							helper_pointer += 8 + name_size;
							
							for (int k = 0; k < filters[j].num_client_vals; k++)
							{
								filters[j].client_data[k] = getBytesAsNumber(helper_pointer, 4);
								helper_pointer += 4;
							}
							
						}
						
						break;
					case 2:
						helper_pointer = msg_pointer + 2;
						
						for (int j = 0; j < num_filters; j++)
						{
							
							filters[j].filter_id = getBytesAsNumber(helper_pointer, 2);
							filters[j].optional_flag = getBytesAsNumber(helper_pointer + 2, 2) & 1;
							filters[j].num_client_vals = getBytesAsNumber(helper_pointer + 4, 2);
							filters[j].client_data = malloc(filters[j].num_client_vals * sizeof(uint32_t));
							helper_pointer += 6;
							
							for (int k = 0; k < filters[j].num_client_vals; k++)
							{
								filters[j].client_data[k] = getBytesAsNumber(helper_pointer, 4);
								helper_pointer += 4;
							}
							
						}
						
						break;
					default:
						printf("Unknown data storage pipeline version %d at address 0x%lx.\n", *msg_pointer, msg_address);
						exit(EXIT_FAILURE);
						
				}
			case 12:
				//attribute message
				name_size = getBytesAsNumber(msg_pointer + 2, 2);
				datatype_size = getBytesAsNumber(msg_pointer + 4, 2);
				dataspace_size = getBytesAsNumber(msg_pointer + 6, 2);
				strncpy(name, msg_pointer + 8, name_size);
				if (strncmp(name, "MATLAB_class", 11) == 0)
				{
					attribute_data_size = getBytesAsNumber(msg_pointer + 8 + roundUp(name_size) + 4, 4);
					strncpy(object->matlab_class, msg_pointer + 8 + roundUp(name_size) + roundUp(datatype_size) +
											roundUp(dataspace_size), attribute_data_size);
					object->matlab_class[attribute_data_size] = 0x0;
					if (strcmp("struct", object->matlab_class) == 0)
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
				bytes_read = 0 - msg_size - 8;
			default:
				//ignore message
				;
		}
		
		bytes_read += msg_size + 8;
	}
	
	//allocate space for data
	switch (object->type)
	{
		case DOUBLE:
			object->double_data = (double *) malloc(num_elems * sizeof(double));
			elem_size = sizeof(double);
			break;
		case UNSIGNEDINT16:
			object->ushort_data = (uint16_t *) malloc(num_elems * sizeof(uint16_t));
			elem_size = sizeof(uint16_t);
			break;
		case REF:
			//STORE ADDRESSES IN THE UDOUBLE_DATA ARRAY; THESE ARE NOT NOT ACTUAL ELEMENTS
			object->udouble_data = (uint64_t *) malloc(num_elems * sizeof(uint64_t));
			elem_size = sizeof(uint64_t);
			break;
		case CHAR:
			object->char_data = (char *) malloc(num_elems * sizeof(char));
			elem_size = sizeof(char);
			break;
		case STRUCT:
			object->dims = malloc(sizeof(int) * 3);
			object->dims[0] = 1;
			object->dims[1] = 1;
			object->dims[2] = 0;
			break;
		default:
			printf("Unknown data type encountered with header at address 0x%lx\n", header_address);
			exit(EXIT_FAILURE);
	}
	
	//fetch data
	switch (layout_class)
	{
		case 0:
			//compact storage or contiguous storage
			for (int j = 0; j < num_elems; j++)
			{
				if (object->double_data != NULL)
				{
					object->double_data[j] = convertHexToFloatingPoint(getBytesAsNumber(data_pointer + j * elem_size, elem_size));
				} else if (object->ushort_data != NULL)
				{
					object->ushort_data[j] = (uint16_t )getBytesAsNumber(data_pointer + j * elem_size, elem_size);
				} else if (object->udouble_data != NULL)
				{
					object->udouble_data[j] = getBytesAsNumber(data_pointer + j * elem_size, elem_size) +
										 s_block.base_address; //these are addresses so we have to add the offset
				} else if (object->char_data != NULL)
				{
					object->char_data[j] = getBytesAsNumber(data_pointer + j * elem_size, elem_size);
				}
			}
			break;
		case 2:
			//chunked storage
			//TreeNode root;
			//fillNode(&root,num_chunked_dims);
			//freeTree(&root);
			printf("Chunked layout class (not yet implemented) encountered with header at address 0x%lx\n", header_address);
			exit(EXIT_FAILURE);
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
			obj.this_obj_header_address = object->udouble_data[i];
			obj.parent_obj_header_address = object->this_obj_address;
			strcpy(obj.name, object->name);
			enqueueObject(obj);
		}
	}
}


void findHeaderAddress(char *filename, char variable_name[])
{
	//printf("Object header for variable %s is at ", variable_name);
	char *delim = ".";
	char *tree_pointer;
	char *heap_pointer;
	char *token;
	variable_found = FALSE;
	
	default_bytes = getAllocGran();
	
	token = strtok(variable_name, delim);
	
	//aka the parent address for a snod
	Addr_Trio parent_trio, this_trio;
	this_trio.parent_obj_header_address = UNDEF_ADDR;
	
	//search for the object header for the variable
	while (queue.length > 0)
	{
		tree_pointer = navigateTo(queue.trios[queue.front].tree_address, default_bytes, TREE);
		heap_pointer = navigateTo(queue.trios[queue.front].heap_address, default_bytes, HEAP);
		assert(strncmp("HEAP", heap_pointer, 4) == 0);
		
		if (strncmp("TREE", tree_pointer, 4) == 0)
		{
			readTreeNode(tree_pointer, this_trio);
			parent_trio = dequeueTrio();
		}
		else if (strncmp("SNOD", tree_pointer, 4) == 0)
		{
			this_trio = dequeueTrio();
			readSnod(tree_pointer, heap_pointer, token, parent_trio, this_trio);
		}
	}
	//printf("0x%lx\n", header_address);
}


Data* organizeObjects(Data* objects, int num_objs)
{
	int index = 0;
	Data* super_objects = malloc((num_objs + 1) * sizeof(Data));
	super_objects[0] = objects[0];
	super_objects[1].type = UNDEF;
	index++;
	
	placeInSuperObject(&super_objects[0], objects, num_objs, &index);
	
	return super_objects;
}

void placeInSuperObject(Data* super_object, Data* objects, int num_objs, int* index)
{
	//note: index should be at the starting index of the subobjects
	
	int j = 0;
	int idx = *index;
	Data* sub_objects = malloc((num_objs - idx + 1)*sizeof(Data));
	
	while(super_object->this_obj_address == objects[idx].parent_obj_address)
	{
		sub_objects[j] = objects[idx];
		idx++;
		if(sub_objects[j].type == STRUCT || sub_objects[j].type == REF)
		{
			//since this is a depth-first traversal
			placeInSuperObject(&sub_objects[j], objects, num_objs, &idx);
		}
		j++;
	}
	
	//signals the end of the objects
	sub_objects[j].type = UNDEF;
	super_object->sub_objects = sub_objects;
	*index = idx;
	
}
