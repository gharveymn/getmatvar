#include "getmatvar.h"


Superblock getSuperblock(int fd, size_t file_size)
{
	char* superblock_pointer = findSuperblock(fd, file_size);
	Superblock s_block = fillSuperblock(superblock_pointer);
	
	//unmap superblock
	if(munmap(maps[0].map_start, maps[0].bytes_mapped) != 0)
	{
		readMXError("getmatvar:internalError", "munmap() unsuccessful in getSuperblock()\n\n");
		//fprintf(stderr, "munmap() unsuccessful in getSuperblock(), Check errno: %d\n", errno);
		//exit(EXIT_FAILURE);
	}
	maps[0].used = FALSE;
	
	return s_block;
}


char* findSuperblock(int fd, size_t file_size)
{
	char* chunk_start = "";
	size_t alloc_gran = getAllocGran();
	
	//Assuming that superblock is in first 8 512 byte chunks
	maps[0].map_start = mmap(NULL, alloc_gran, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	maps[0].bytes_mapped = alloc_gran;
	maps[0].offset = 0;
	maps[0].used = TRUE;
	
	if(maps[0].map_start == NULL || maps[0].map_start == MAP_FAILED)
	{
		readMXError("getmatvar:internalError", "mmap() unsuccessful\n\n");
		//fprintf(stderr, "mmap() unsuccessful, Check errno: %d\n", errno);
		//exit(EXIT_FAILURE);
	}
	
	chunk_start = maps[0].map_start;
	
	while(strncmp(FORMAT_SIG, chunk_start, 8) != 0 && (chunk_start - maps[0].map_start) < alloc_gran)
	{
		chunk_start += 512;
	}
	
	if((chunk_start - maps[0].map_start) >= alloc_gran)
	{
		readMXError("getmatvar:internalError", "Couldn't find superblock in first 8 512-byte chunks\n\n");
		//fprintf(stderr, "Couldn't find superblock in first 8 512-byte chunks. I am quitting.\n");
		//exit(EXIT_FAILURE);
	}
	
	return chunk_start;
}


Superblock fillSuperblock(char* superblock_pointer)
{
	Superblock s_block;
	//get stuff from superblock, for now assume consistent versions of stuff
	s_block.size_of_offsets = (uint8_t)getBytesAsNumber(superblock_pointer + 13, 1, META_DATA_BYTE_ORDER);
	s_block.size_of_lengths = (uint8_t)getBytesAsNumber(superblock_pointer + 14, 1, META_DATA_BYTE_ORDER);
	s_block.leaf_node_k = (uint16_t)getBytesAsNumber(superblock_pointer + 16, 2, META_DATA_BYTE_ORDER);
	s_block.internal_node_k = (uint16_t)getBytesAsNumber(superblock_pointer + 18, 2, META_DATA_BYTE_ORDER);
	s_block.base_address = getBytesAsNumber(superblock_pointer + 24, s_block.size_of_offsets, META_DATA_BYTE_ORDER);
	
	//read scratchpad space
	char* sps_start = superblock_pointer + 80;
	Addr_Trio root_trio;
	root_trio.parent_obj_header_address = UNDEF_ADDR;
	root_trio.tree_address = getBytesAsNumber(sps_start, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
	root_trio.heap_address = getBytesAsNumber(sps_start + s_block.size_of_offsets, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
	s_block.root_tree_address = root_trio.tree_address;
	enqueueTrio(root_trio);
	
	return s_block;
}


char* navigateTo(uint64_t address, uint64_t bytes_needed, int map_index)
{
	
	if(!(maps[map_index].used && address >= maps[map_index].offset && address + bytes_needed < maps[map_index].offset + maps[map_index].bytes_mapped))
	{
		//unmap current page if used
		if(maps[map_index].used)
		{
			if(munmap(maps[map_index].map_start, maps[map_index].bytes_mapped) != 0)
			{
				readMXError("getmatvar:internalError", "munmap() unsuccessful in navigateTo()\n\n");
				//fprintf(stderr, "munmap() unsuccessful in navigateTo(), Check errno: %d\n", errno);
				//fprintf(stderr, "1st arg: %s\n2nd arg: %lu\nUsed: %d\n", maps[map_index].map_start, maps[map_index].bytes_mapped, maps[map_index].used);
				//exit(EXIT_FAILURE);
			}
			maps[map_index].used = FALSE;
		}
		
		//map new page at needed location
		size_t alloc_gran = getAllocGran();
		maps[map_index].offset = (OffsetType)((address / alloc_gran) * alloc_gran);
		maps[map_index].bytes_mapped = address - maps[map_index].offset + bytes_needed;
		maps[map_index].map_start = mmap(NULL, maps[map_index].bytes_mapped, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, maps[map_index].offset);
		
		maps[map_index].used = TRUE;
		if(maps[map_index].map_start == NULL || maps[map_index].map_start == MAP_FAILED)
		{
			readMXError("getmatvar:internalError", "mmap() unsuccessful\n\n");
			//fprintf(stderr, "mmap() unsuccessful, Check errno: %d\n", errno);
			//exit(EXIT_FAILURE);
		}
	}
	return maps[map_index].map_start + address - maps[map_index].offset;
}


char* navigateTo_map(MemMap map, uint64_t address, uint64_t bytes_needed, int map_index)
{
	if(!(map.used && address >= map.offset && address + bytes_needed < map.offset + map.bytes_mapped))
	{
		//unmap current page if used
		if(map.used)
		{
			if(munmap(map.map_start, map.bytes_mapped) != 0)
			{
				readMXError("getmatvar:internalError", "munmap() unsuccessful\n\n");
				//fprintf(stderr, "munmap() unsuccessful in navigateTo(), Check errno: %d\n", errno);
				//fprintf(stderr, "1st arg: %s\n2nd arg: %lu\nUsed: %d\n", map.map_start, map.bytes_mapped, map.used);
				//exit(EXIT_FAILURE);
			}
			map.used = FALSE;
		}
		
		//map new page at needed location
		size_t alloc_gran = getAllocGran();
		map.offset = (off_t)(address / alloc_gran) * alloc_gran;
		map.bytes_mapped = address - map.offset + bytes_needed;
		map.map_start = mmap(NULL, map.bytes_mapped, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, map.offset);
		
		map.used = TRUE;
		if(map.map_start == NULL || map.map_start == MAP_FAILED)
		{
			readMXError("getmatvar:internalError", "mmap() unsuccessful\n\n");
			//fprintf(stderr, "mmap() unsuccessful, Check errno: %d\n", errno);
			//exit(EXIT_FAILURE);
		}
	}
	return map.map_start + address - map.offset;
}


void readTreeNode(char* tree_pointer, Addr_Trio this_trio)
{
	Addr_Trio trio;
	uint16_t entries_used = 0;
	uint64_t left_sibling_address, right_sibling_address;
	
	entries_used = (uint16_t)getBytesAsNumber(tree_pointer + 6, 2, META_DATA_BYTE_ORDER);
	
	//assuming keys do not contain pertinent information
	left_sibling_address = getBytesAsNumber(tree_pointer + 8, s_block.size_of_offsets, META_DATA_BYTE_ORDER);
	if(left_sibling_address != UNDEF_ADDR)
	{
		trio.parent_obj_header_address = this_trio.parent_obj_header_address;
		trio.tree_address = left_sibling_address + s_block.base_address;
		trio.heap_address = this_trio.heap_address;
		enqueueTrio(trio);
	}
	
	right_sibling_address = getBytesAsNumber(tree_pointer + 8 + s_block.size_of_offsets, s_block.size_of_offsets, META_DATA_BYTE_ORDER);
	if(right_sibling_address != UNDEF_ADDR)
	{
		trio.parent_obj_header_address = this_trio.parent_obj_header_address;;
		trio.tree_address = right_sibling_address + s_block.base_address;
		trio.heap_address = this_trio.heap_address;
		enqueueTrio(trio);
	}
	
	//group node B-Tree traversal (version 0)
	int key_size = s_block.size_of_lengths;
	char* key_pointer = tree_pointer + 8 + 2 * s_block.size_of_offsets;
	for(int i = 0; i < entries_used; i++)
	{
		
		trio.parent_obj_header_address = this_trio.parent_obj_header_address;
		trio.tree_address = getBytesAsNumber(key_pointer + key_size, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
		trio.heap_address = this_trio.heap_address;
		
		//in the case where we encounter a struct but have more items ahead
		priorityEnqueueTrio(trio);
		
		key_pointer += key_size + s_block.size_of_offsets;
		
	}
	
}


void readSnod(char* snod_pointer, char* heap_pointer, Addr_Trio parent_trio, Addr_Trio this_trio)
{
	uint16_t num_symbols = (uint16_t)getBytesAsNumber(snod_pointer + 6, 2, META_DATA_BYTE_ORDER);
	Object* objects = malloc(sizeof(Object) * num_symbols);
	uint32_t cache_type;
	Addr_Trio trio;
	char* var_name = peekVariableName();
	
	uint64_t heap_data_segment_size = getBytesAsNumber(heap_pointer + 8, s_block.size_of_lengths, META_DATA_BYTE_ORDER);
	uint64_t heap_data_segment_address = getBytesAsNumber(heap_pointer + 8 + 2 * s_block.size_of_lengths, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
	char* heap_data_segment_pointer = navigateTo(heap_data_segment_address, heap_data_segment_size, HEAP);
	
	//get to entries
	snod_pointer += 8;
	int sym_table_entry_size = 2*s_block.size_of_offsets + 4 + 4 + 16;
	
	for(int i = 0; i < num_symbols; i++)
	{
		objects[i].name_offset = getBytesAsNumber(snod_pointer + i*sym_table_entry_size, s_block.size_of_offsets, META_DATA_BYTE_ORDER);
		objects[i].parent_obj_header_address = this_trio.parent_obj_header_address;
		objects[i].this_obj_header_address = getBytesAsNumber(snod_pointer + i*sym_table_entry_size + s_block.size_of_offsets, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
		strcpy(objects[i].name, heap_data_segment_pointer + objects[i].name_offset);
		cache_type = (uint32_t)getBytesAsNumber(snod_pointer + 2 * s_block.size_of_offsets + sym_table_entry_size * i, 4, META_DATA_BYTE_ORDER);
		objects[i].parent_tree_address = parent_trio.tree_address;
		objects[i].this_tree_address = this_trio.tree_address;
		objects[i].sub_tree_address = UNDEF_ADDR;
		
		//check if we have found the object we're looking for or if we are just adding subobjects
		if((var_name != NULL && strcmp(var_name, objects[i].name) == 0) || variable_found == TRUE)
		{
			if(variable_found == FALSE)
			{
				flushHeaderQueue();
				flushQueue();
			}
			//if the variable has been found we should keep going down the tree for that variable
			//all items in the queue should only be subobjects so this is safe
			
			
			if(cache_type == 1)
			{
				//if another tree exists for this object, put it on the queue
				trio.parent_obj_header_address = objects[i].this_obj_header_address;
				trio.tree_address = getBytesAsNumber(snod_pointer + 2 * s_block.size_of_offsets + 8 + sym_table_entry_size * i, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
				trio.heap_address = getBytesAsNumber(snod_pointer + 3 * s_block.size_of_offsets + 8 + sym_table_entry_size * i, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
				objects[i].sub_tree_address = trio.tree_address;
				priorityEnqueueTrio(trio);
			}
			else if(cache_type == 2)
			{
				//this object is a symbolic link, the name is stored in the heap at the address indicated in the scratch pad
				objects[i].name_offset = getBytesAsNumber(snod_pointer + 2*s_block.size_of_offsets + 8 + sym_table_entry_size * i, 4, META_DATA_BYTE_ORDER);
				strcpy(objects[i].name, heap_pointer + 8 + 2 * s_block.size_of_lengths + s_block.size_of_offsets + objects[i].name_offset);
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

void freeDataObjects(Data** objects)
{

	int i = 0;
	while (objects[i]->type != SENTINEL)
	{

		if(objects[i]->data_arrays.is_mx_used != TRUE && objects[i]->data_arrays.ui8_data != NULL)
		{
			mxFree(objects[i]->data_arrays.ui8_data);
			objects[i]->data_arrays.ui8_data = NULL;
		}
		
		if(objects[i]->data_arrays.is_mx_used != TRUE && objects[i]->data_arrays.i8_data != NULL)
		{
			mxFree(objects[i]->data_arrays.i8_data);
			objects[i]->data_arrays.i8_data = NULL;
		}
		
		if(objects[i]->data_arrays.is_mx_used != TRUE && objects[i]->data_arrays.ui16_data != NULL)
		{
			mxFree(objects[i]->data_arrays.ui16_data);
			objects[i]->data_arrays.ui16_data = NULL;
		}
		
		if(objects[i]->data_arrays.is_mx_used != TRUE && objects[i]->data_arrays.i16_data != NULL)
		{
			mxFree(objects[i]->data_arrays.i16_data);
			objects[i]->data_arrays.i16_data = NULL;
		}
		
		if(objects[i]->data_arrays.is_mx_used != TRUE && objects[i]->data_arrays.ui32_data != NULL)
		{
			mxFree(objects[i]->data_arrays.ui32_data);
			objects[i]->data_arrays.ui32_data = NULL;
		}
		
		if(objects[i]->data_arrays.is_mx_used != TRUE && objects[i]->data_arrays.i32_data != NULL)
		{
			mxFree(objects[i]->data_arrays.i32_data);
			objects[i]->data_arrays.i32_data = NULL;
		}
		
		if(objects[i]->data_arrays.is_mx_used != TRUE && objects[i]->data_arrays.ui64_data != NULL)
		{
			mxFree(objects[i]->data_arrays.ui64_data);
			objects[i]->data_arrays.ui64_data = NULL;
		}
		
		if(objects[i]->data_arrays.is_mx_used != TRUE && objects[i]->data_arrays.i64_data != NULL)
		{
			mxFree(objects[i]->data_arrays.i64_data);
			objects[i]->data_arrays.i64_data = NULL;
		}
		
		if(objects[i]->data_arrays.is_mx_used != TRUE && objects[i]->data_arrays.single_data != NULL)
		{
			mxFree(objects[i]->data_arrays.single_data);
			objects[i]->data_arrays.single_data = NULL;
		}
		
		if(objects[i]->data_arrays.is_mx_used != TRUE && objects[i]->data_arrays.double_data != NULL)
		{
			mxFree(objects[i]->data_arrays.double_data);
			objects[i]->data_arrays.double_data = NULL;
		}
		
		if(objects[i]->data_arrays.udouble_data != NULL)
		{
			free(objects[i]->data_arrays.udouble_data);
			objects[i]->data_arrays.udouble_data = NULL;
		}
		
		if(objects[i]->dims != NULL)
		{
			free(objects[i]->dims);
			objects[i]->dims = NULL;
		}
		
		if(objects[i]->chunked_info.chunked_dims != NULL)
		{
			free(objects[i]->chunked_info.chunked_dims);
			objects[i]->chunked_info.chunked_dims = NULL;
		}
		
		for(int j = 0; j < objects[i]->chunked_info.num_filters; j++)
		{
			free(objects[i]->chunked_info.filters[j].client_data);
			objects[i]->chunked_info.filters[j].client_data = NULL;
		}
		
		if(objects[i]->sub_objects != NULL)
		{
			free(objects[i]->sub_objects);
			objects[i]->sub_objects = NULL;
		}
		
		
		free(objects[i]);
		objects[i] = NULL;

		i++;

	}

	//note that nothing was malloced inside the sentinel object
	free(objects[i]);
	objects[i] = NULL;
	free(objects);

}

void freeDataObjectTree(Data* super_object)
{

	if(super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.ui8_data != NULL)
	{
		mxFree(super_object->data_arrays.ui8_data);
		super_object->data_arrays.ui8_data = NULL;
	}
	
	if(super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.i8_data != NULL)
	{
		mxFree(super_object->data_arrays.i8_data);
		super_object->data_arrays.i8_data = NULL;
	}
	
	if(super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.ui16_data != NULL)
	{
		mxFree(super_object->data_arrays.ui16_data);
		super_object->data_arrays.ui16_data = NULL;
	}
	
	if(super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.i16_data != NULL)
	{
		mxFree(super_object->data_arrays.i16_data);
		super_object->data_arrays.i16_data = NULL;
	}
	
	if(super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.ui32_data != NULL)
	{
		mxFree(super_object->data_arrays.ui32_data);
		super_object->data_arrays.ui32_data = NULL;
	}
	
	if(super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.i32_data != NULL)
	{
		mxFree(super_object->data_arrays.i32_data);
		super_object->data_arrays.i32_data = NULL;
	}
	
	if(super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.ui64_data != NULL)
	{
		mxFree(super_object->data_arrays.ui64_data);
		super_object->data_arrays.ui64_data = NULL;
	}
	
	if(super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.i64_data != NULL)
	{
		mxFree(super_object->data_arrays.i64_data);
		super_object->data_arrays.i64_data = NULL;
	}
	
	if(super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.single_data != NULL)
	{
		mxFree(super_object->data_arrays.single_data);
		super_object->data_arrays.single_data = NULL;
	}
	
	if(super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.double_data != NULL)
	{
		mxFree(super_object->data_arrays.double_data);
		super_object->data_arrays.double_data = NULL;
	}
	
	if(super_object->data_arrays.udouble_data != NULL)
	{
		free(super_object->data_arrays.udouble_data);
		super_object->data_arrays.udouble_data = NULL;
	}
	
	if(super_object->dims != NULL)
	{
		free(super_object->dims);
		super_object->dims = NULL;
	}
	
	if(super_object->chunked_info.chunked_dims != NULL)
	{
		free(super_object->chunked_info.chunked_dims);
		super_object->chunked_info.chunked_dims = NULL;
	}
	
	for(int j = 0; j < super_object->chunked_info.num_filters; j++)
	{
		free(super_object->chunked_info.filters[j].client_data);
		super_object->chunked_info.filters[j].client_data = NULL;
	}
	
	if(super_object->sub_objects != NULL)
	{
        for(int j = 0; j < super_object->num_sub_objs; j++)
        {
            freeDataObjectTree(super_object->sub_objects[j]);
        }
		free(super_object->sub_objects);
		super_object->sub_objects = NULL;
	}
	
	
	free(super_object);
	super_object = NULL;
	
}
