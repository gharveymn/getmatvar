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
	Addr_Pair root_pair;
	root_pair.tree_address = getBytesAsNumber(sps_start, s_block.size_of_offsets) + s_block.base_address;
	root_pair.heap_address =
		   getBytesAsNumber(sps_start + s_block.size_of_offsets, s_block.size_of_offsets) + s_block.base_address;
	s_block.root_tree_address = root_pair.tree_address;
	enqueuePair(root_pair);
	
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


void readTreeNode(char *tree_pointer)
{
	Addr_Pair pair;
	uint16_t entries_used = 0;
	uint64_t left_sibling, right_sibling;
	
	entries_used = getBytesAsNumber(tree_pointer + 6, 2);
	
	//assuming keys do not contain pertinent information
	left_sibling = getBytesAsNumber(tree_pointer + 8, s_block.size_of_offsets);
	if (left_sibling != UNDEF_ADDR)
	{
		pair.tree_address = left_sibling + s_block.base_address;
		pair.heap_address = queue.pairs[queue.front].heap_address;
		enqueuePair(pair);
	}
	
	right_sibling = getBytesAsNumber(tree_pointer + 8 + s_block.size_of_offsets, s_block.size_of_offsets);
	if (right_sibling != UNDEF_ADDR)
	{
		pair.tree_address = right_sibling + s_block.base_address;
		pair.heap_address = queue.pairs[queue.front].heap_address;
		enqueuePair(pair);
	}
	
	for (int i = 0; i < entries_used; i++)
	{
		pair.tree_address =
			   getBytesAsNumber(tree_pointer + 2 * s_block.size_of_offsets + 16 * (i + 1), s_block.size_of_offsets) +
			   s_block.base_address;
		pair.heap_address = queue.pairs[queue.front].heap_address;
		enqueuePair(pair);
	}
	
}


void readSnod(char *snod_pointer, char *heap_pointer, char *var_name, uint64_t prev_tree_address)
{
	uint16_t num_symbols = getBytesAsNumber(snod_pointer + 6, 2);
	Object *objects = (Object *) malloc(sizeof(Object) * num_symbols);
	uint32_t cache_type;
	Addr_Pair pair;
	
	//get to entries
	snod_pointer += 8;
	
	for (int i = 0; i < num_symbols; i++)
	{
		objects[i].this_tree_address = 0;
		objects[i].name_offset = getBytesAsNumber(snod_pointer + SYM_TABLE_ENTRY_SIZE * i, s_block.size_of_offsets);
		objects[i].obj_header_address =
			   getBytesAsNumber(snod_pointer + SYM_TABLE_ENTRY_SIZE * i + s_block.size_of_offsets,
							s_block.size_of_offsets) + s_block.base_address;
		strcpy(objects[i].name,
			  heap_pointer + 8 + 2 * s_block.size_of_lengths + s_block.size_of_offsets + objects[i].name_offset);
		cache_type = getBytesAsNumber(snod_pointer + 2 * s_block.size_of_offsets + SYM_TABLE_ENTRY_SIZE * i, 4);
		objects[i].prev_tree_address = prev_tree_address;
		
		//check if we have found the object we're looking for
		if (var_name != NULL && strcmp(var_name, objects[i].name) == 0)
		{
			flushHeaderQueue();
			
			//if another tree exists for this object, put it on the queue
			if (cache_type == 1)
			{
				pair.tree_address =
					   getBytesAsNumber(snod_pointer + 2 * s_block.size_of_offsets + 8 + SYM_TABLE_ENTRY_SIZE * i,
									s_block.size_of_offsets) + s_block.base_address;
				pair.heap_address =
					   getBytesAsNumber(snod_pointer + 3 * s_block.size_of_offsets + 8 + SYM_TABLE_ENTRY_SIZE * i,
									s_block.size_of_offsets) + s_block.base_address;
				objects[i].this_tree_address = pair.tree_address;
				flushQueue();
				priorityEnqueuePair(pair);
			}
			enqueueObject(objects[i]);
			break;
		} else if (var_name == NULL)
		{
			//objects[i].this_tree_address = UNDEF_ADDR;
			enqueueObject(objects[i]);
		}
	}
	
	free(objects);
}


uint32_t *readDataSpaceMessage(char *msg_pointer, uint16_t msg_size)
{
	//assume version 1 and ignore max dims and permutation indices
	uint8_t num_dims = *(msg_pointer + 1);
	uint32_t *dims = (uint32_t *) malloc(sizeof(int) * (num_dims + 1));
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
	uint8_t class = *(msg_pointer) & 7; //only want bottom 4 bits
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
					type = UINT16_T;
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