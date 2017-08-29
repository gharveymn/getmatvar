#include "mapping.h"


Superblock getSuperblock(int fd, size_t file_size)
{
	char *superblock_pointer = findSuperblock(fd, file_size);
	Superblock s_block = fillSuperblock(superblock_pointer);
	
	//unmap superblock
	if (munmap(maps[0].map_start, maps[0].bytes_mapped) != 0)
	{
		printf("munmap() unsuccessful in getSuperblock(), Check errno: %d\n", errno);
		exit(EXIT_FAILURE);
	}
	maps[0].used = FALSE;
	
	return s_block;
}


char *findSuperblock(int fd, size_t file_size)
{
	char *chunk_start = "";
	size_t alloc_gran = getAllocGran();
	
	//Assuming that superblock is in first 8 512 byte chunks
	maps[0].map_start = mmap(NULL, alloc_gran, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	maps[0].bytes_mapped = alloc_gran;
	maps[0].offset = 0;
	maps[0].used = TRUE;
	
	if (maps[0].map_start == NULL || maps[0].map_start == MAP_FAILED)
	{
		printf("mmap() unsuccessful, Check errno: %d\n", errno);
		exit(EXIT_FAILURE);
	}
	
	chunk_start = maps[0].map_start;
	
	while (strncmp(FORMAT_SIG, chunk_start, 8) != 0 && (chunk_start - maps[0].map_start) < alloc_gran)
	{
		chunk_start += 512;
	}
	
	if ((chunk_start - maps[0].map_start) >= alloc_gran)
	{
		printf("Couldn't find superblock in first 8 512-byte chunks. I am quitting.\n");
		exit(EXIT_FAILURE);
	}
	
	return chunk_start;
}


Superblock fillSuperblock(char *superblock_pointer)
{
	Superblock s_block;
	//get stuff from superblock, for now assume consistent versions of stuff
	s_block.size_of_offsets = getBytesAsNumber(superblock_pointer + 13, 1);
	s_block.size_of_lengths = getBytesAsNumber(superblock_pointer + 14, 1);
	s_block.leaf_node_k = getBytesAsNumber(superblock_pointer + 16, 2);
	s_block.internal_node_k = getBytesAsNumber(superblock_pointer + 18, 2);
	s_block.base_address = getBytesAsNumber(superblock_pointer + 24, s_block.size_of_offsets);
	
	//read scratchpad space
	char *sps_start = superblock_pointer + 80;
	Addr_Trio root_trio;
	root_trio.parent_obj_header_address = UNDEF_ADDR;
	root_trio.tree_address = getBytesAsNumber(sps_start, s_block.size_of_offsets) + s_block.base_address;
	root_trio.heap_address = getBytesAsNumber(sps_start + s_block.size_of_offsets, s_block.size_of_offsets) + s_block.base_address;
	s_block.root_tree_address = root_trio.tree_address;
	enqueueTrio(root_trio);
	
	return s_block;
}


char *navigateTo(uint64_t address, uint64_t bytes_needed, int map_index)
{
	
	if (!(maps[map_index].used && address >= maps[map_index].offset &&
		 address + bytes_needed < maps[map_index].offset + maps[map_index].bytes_mapped))
	{
		//unmap current page if used
		if (maps[map_index].used)
		{
			if (munmap(maps[map_index].map_start, maps[map_index].bytes_mapped) != 0)
			{
				printf("munmap() unsuccessful in navigateTo(), Check errno: %d\n", errno);
				printf("1st arg: %s\n2nd arg: %lu\nUsed: %d\n", maps[map_index].map_start,
					  maps[map_index].bytes_mapped, maps[map_index].used);
				exit(EXIT_FAILURE);
			}
			maps[map_index].used = FALSE;
		}
		
		//map new page at needed location
		size_t alloc_gran = getAllocGran();
		maps[map_index].offset = (OffsetType) ((address / alloc_gran) * alloc_gran);
		maps[map_index].bytes_mapped = address - maps[map_index].offset + bytes_needed;
		maps[map_index].map_start = mmap(NULL, maps[map_index].bytes_mapped, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, maps[map_index].offset);
		
		maps[map_index].used = TRUE;
		if (maps[map_index].map_start == NULL || maps[map_index].map_start == MAP_FAILED)
		{
			printf("mmap() unsuccessful, Check errno: %d\n", errno);
			exit(EXIT_FAILURE);
		}
	}
	return maps[map_index].map_start + address - maps[map_index].offset;
}


char* navigateTo_map(MemMap map, uint64_t address, uint64_t bytes_needed, int map_index)
{
	if (!(map.used && address >= map.offset && address + bytes_needed < map.offset + map.bytes_mapped))
	{
		//unmap current page if used
		if (map.used)
		{
			if (munmap(map.map_start, map.bytes_mapped) != 0)
			{
				printf("munmap() unsuccessful in navigateTo(), Check errno: %d\n", errno);
				printf("1st arg: %s\n2nd arg: %lu\nUsed: %d\n", map.map_start, map.bytes_mapped, map.used);
				exit(EXIT_FAILURE);
			}
			map.used = FALSE;
		}
		
		//map new page at needed location
		size_t alloc_gran = getAllocGran();
		map.offset = (off_t)(address/alloc_gran)*alloc_gran;
		map.bytes_mapped = address - map.offset + bytes_needed;
		map.map_start = mmap(NULL, map.bytes_mapped, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, map.offset);
		
		map.used = TRUE;
		if (map.map_start == NULL || map.map_start == MAP_FAILED)
		{
			printf("mmap() unsuccessful, Check errno: %d\n", errno);
			exit(EXIT_FAILURE);
		}
	}
	return map.map_start + address - map.offset;
}


void readTreeNode(char *tree_pointer)
{
	Addr_Trio trio;
	uint16_t entries_used = 0;
	uint64_t left_sibling_address, right_sibling_address;
	
	entries_used = getBytesAsNumber(tree_pointer + 6, 2);
	
	//assuming keys do not contain pertinent information
	left_sibling_address = getBytesAsNumber(tree_pointer + 8, s_block.size_of_offsets);
	if (left_sibling_address != UNDEF_ADDR)
	{
		trio.parent_obj_header_address = queue.trios[queue.front].parent_obj_header_address;
		trio.tree_address = left_sibling_address + s_block.base_address;
		trio.heap_address = queue.trios[queue.front].heap_address;
		enqueueTrio(trio);
	}
	
	right_sibling_address = getBytesAsNumber(tree_pointer + 8 + s_block.size_of_offsets, s_block.size_of_offsets);
	if (right_sibling_address != UNDEF_ADDR)
	{
		trio.parent_obj_header_address = queue.trios[queue.front].parent_obj_header_address;;
		trio.tree_address = right_sibling_address + s_block.base_address;
		trio.heap_address = queue.trios[queue.front].heap_address;
		enqueueTrio(trio);
	}
	
	//group node B-Tree traversal (version 0)
	int key_size = s_block.size_of_lengths;
	char* key_pointer = tree_pointer + 8 + 2 * s_block.size_of_offsets;
	for (int i = 0; i < entries_used; i++)
	{
		trio.parent_obj_header_address = queue.trios[queue.front].parent_obj_header_address;
		trio.tree_address = getBytesAsNumber(key_pointer + key_size, s_block.size_of_offsets) + s_block.base_address;
		trio.heap_address = queue.trios[queue.front].heap_address;
		enqueueTrio(trio);
		key_pointer += key_size + s_block.size_of_offsets;
	}
	
}


void readSnod(char *snod_pointer, char *heap_pointer, Addr_Trio parent_trio, Addr_Trio this_trio)
{
	uint16_t num_symbols = getBytesAsNumber(snod_pointer + 6, 2);
	Object *objects = (Object *) malloc(sizeof(Object) * num_symbols);
	uint32_t cache_type;
	Addr_Trio trio;
	char* var_name = peekVariableName();
	
	//get to entries
	snod_pointer += 8;
	
	for (int i = 0; i < num_symbols; i++)
	{
		objects[i].name_offset = getBytesAsNumber(snod_pointer + SYM_TABLE_ENTRY_SIZE * i, s_block.size_of_offsets);
		objects[i].parent_obj_header_address = this_trio.parent_obj_header_address;
		objects[i].this_obj_header_address = getBytesAsNumber(snod_pointer + SYM_TABLE_ENTRY_SIZE * i + s_block.size_of_offsets, s_block.size_of_offsets) + s_block.base_address;
		strcpy(objects[i].name, heap_pointer + 8 + 2 * s_block.size_of_lengths + s_block.size_of_offsets + objects[i].name_offset);
		cache_type = getBytesAsNumber(snod_pointer + 2 * s_block.size_of_offsets + SYM_TABLE_ENTRY_SIZE * i, 4);
		objects[i].parent_tree_address = parent_trio.tree_address;
		objects[i].this_tree_address = this_trio.tree_address;
		objects[i].sub_tree_address = UNDEF_ADDR;
		
		//check if we have found the object we're looking for or if we are just adding subobjects
		if ((var_name != NULL && strcmp(var_name, objects[i].name) == 0) || variable_found == TRUE)
		{
			if(variable_found == FALSE)
			{
				flushHeaderQueue();
				flushQueue();
			}
			//if the variable has been found we should keep going down the tree for that variable
			//all items in the queue should only be subobjects so this is safe
			
			
			//if another tree exists for this object, put it on the queue
			if (cache_type == 1)
			{
				trio.parent_obj_header_address = objects[i].this_obj_header_address;
				trio.tree_address = getBytesAsNumber(snod_pointer + 2 * s_block.size_of_offsets + 8 + SYM_TABLE_ENTRY_SIZE * i, s_block.size_of_offsets) + s_block.base_address;
				trio.heap_address = getBytesAsNumber(snod_pointer + 3 * s_block.size_of_offsets + 8 + SYM_TABLE_ENTRY_SIZE * i, s_block.size_of_offsets) + s_block.base_address;
				objects[i].sub_tree_address = trio.tree_address;
				priorityEnqueueTrio(trio);
			}
			enqueueObject(objects[i]);
			
			//we found a token, but not the end variable, so we dont need to look at the rest of the variables at this level
			if(variable_found == FALSE)
			{
				dequeueVariableName();
				if(variable_name_queue.length == 0)
				{
					//means this was the last token, so we've found the variable we want
					variable_found = TRUE;
				}
				break;
			}
			
		}
	}
	
	free(objects);
}


uint32_t *readDataSpaceMessage(char *msg_pointer, uint16_t msg_size)
{
	//assume version 1 and ignore max dims and permutation indices
	uint8_t num_dims = *(msg_pointer + 1);
	uint32_t *dims = malloc(sizeof(int) * (num_dims + 1));
	//uint64_t bytes_read = 0;
	
	for (int i = 0; i < num_dims; i++)
	{
		dims[i] = getBytesAsNumber(msg_pointer + 8 + i * s_block.size_of_lengths, 4);
	}
	dims[num_dims] = 0;
	return dims;
}


Datatype readDataTypeMessage(char *msg_pointer, uint16_t msg_size)
{
	//assume version 1
	uint8_t class = *(msg_pointer) & 0x0F; //only want bottom 4 bits
	uint32_t size = *(msg_pointer + 4);
	Datatype type = UNDEF;
	
	switch (class)
	{
		case 0:
			//fixed point (string)
			
			switch (size)
			{
				case 1:
					//"char"
					type = CHAR;
					break;
				case 2:
					//"uint16_t"
					type = UNSIGNEDINT16;
					break;
				default:
					type = UNDEF;
					break;
			}
			
			break;
		case 1:
			//floating point
			//assume double precision
			type = DOUBLE;
			break;
		case 7:
			//reference (cell), data consists of addresses aka references
			type = REF;
			break;
		default:
			//ignore
			type = UNDEF;
			break;
	}
	return type;
	
}


void freeDataObjects(Data *objects, int num)
{
	int num_subs = 0;
	int j;
	for (int i = 0; i < num; i++)
	{
		if (objects[i].char_data != NULL)
		{
			free(objects[i].char_data);
		} else if (objects[i].double_data != NULL)
		{
			free(objects[i].double_data);
		} else if (objects[i].udouble_data != NULL)
		{
			free(objects[i].udouble_data);
		} else if (objects[i].ushort_data != NULL)
		{
			free(objects[i].ushort_data);
		}
		free(objects[i].dims);
		j = 0;
		if (objects[i].sub_objects != NULL)
		{
			while (objects->sub_objects[j].type != UNDEF)
			{
				num_subs++;
				j++;
			}
			freeDataObjects(objects[i].sub_objects, num_subs);
		}
	}
	free(objects);
	
}