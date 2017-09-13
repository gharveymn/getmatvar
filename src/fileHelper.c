#include "getmatvar_.h"


Superblock getSuperblock(void)
{
	byte *superblock_pointer = findSuperblock();
	Superblock s_block = fillSuperblock(superblock_pointer);

	//unmap superblock
	freeMap(TREE);

	return s_block;
}


byte *findSuperblock(void)
{
	size_t alloc_gran = getAllocGran();

	//Assuming that superblock is in first 8 512 byte chunks
	byte *chunk_start = navigateTo(0, alloc_gran, TREE);
	uint16_t chunk_address = 0;

	while (strncmp(FORMAT_SIG, chunk_start, 8) != 0 && chunk_address < alloc_gran)
	{
		chunk_start += 512;
		chunk_address += 512;
	}

	if (chunk_address >= alloc_gran)
	{
		readMXError("getmatvar:internalError", "Couldn't find superblock in first 8 512-byte chunks.\n\n");
	}

	return chunk_start;
}


Superblock fillSuperblock(byte *superblock_pointer)
{
	Superblock s_block;
	//get stuff from superblock, for now assume consistent versions of stuff
	s_block.size_of_offsets = (uint8_t) getBytesAsNumber(superblock_pointer + 13, 1, META_DATA_BYTE_ORDER);
	s_block.size_of_lengths = (uint8_t) getBytesAsNumber(superblock_pointer + 14, 1, META_DATA_BYTE_ORDER);
	s_block.leaf_node_k = (uint16_t) getBytesAsNumber(superblock_pointer + 16, 2, META_DATA_BYTE_ORDER);
	s_block.internal_node_k = (uint16_t) getBytesAsNumber(superblock_pointer + 18, 2, META_DATA_BYTE_ORDER);
	s_block.base_address = getBytesAsNumber(superblock_pointer + 24, s_block.size_of_offsets, META_DATA_BYTE_ORDER);

	//read scratchpad space
	byte *sps_start = superblock_pointer + 80;
	root_trio.parent_obj_header_address = UNDEF_ADDR;
	root_trio.tree_address =
			getBytesAsNumber(sps_start, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
	root_trio.heap_address =
			getBytesAsNumber(sps_start + s_block.size_of_offsets, s_block.size_of_offsets, META_DATA_BYTE_ORDER) +
			s_block.base_address;
	s_block.root_tree_address = root_trio.tree_address;

	return s_block;
}


void freeMap(int map_index)
{
	maps[map_index].used = FALSE;
	if (munmap(maps[map_index].map_start, maps[map_index].bytes_mapped) != 0)
	{
		readMXError("getmatvar:internalError", "munmap() unsuccessful.\n\n", "");
	}
}


byte *navigateTo(uint64_t address, uint64_t bytes_needed, int map_index)
{

	//only remap if we really need to (this removes the need for checks/headaches inside main functions)
	if (!(address >= maps[map_index].offset &&
		 address + bytes_needed <= maps[map_index].offset + maps[map_index].bytes_mapped) ||
	    maps[map_index].used == FALSE)
	{

		//unmap current page
		if (maps[map_index].used == TRUE)
		{
			freeMap(map_index);
		}

		//map new page at needed location
		size_t alloc_gran = getAllocGran();
		alloc_gran = alloc_gran < file_size ? alloc_gran : file_size;
		maps[map_index].offset = (OffsetType) ((address / alloc_gran) * alloc_gran);
		maps[map_index].bytes_mapped = address - maps[map_index].offset + bytes_needed;
		maps[map_index].bytes_mapped =
				maps[map_index].bytes_mapped < file_size - maps[map_index].offset ? maps[map_index].bytes_mapped :
				file_size - maps[map_index].offset;
		maps[map_index].map_start = mmap(NULL, maps[map_index].bytes_mapped, PROT_READ, MAP_PRIVATE, fd,
								   maps[map_index].offset);
		maps[map_index].used = TRUE;
		if (maps[map_index].map_start == NULL || maps[map_index].map_start == MAP_FAILED)
		{
			maps[map_index].used = FALSE;
			readMXError("getmatvar:internalError", "mmap() unsuccessful int navigateTo(). Check errno %d\n\n",
					  errno);
		}
	}
	return maps[map_index].map_start + address - maps[map_index].offset;
}


void readTreeNode(byte *tree_pointer, Addr_Trio this_trio)
{
	Addr_Trio trio;
	uint16_t entries_used = 0;
	uint64_t left_sibling_address, right_sibling_address;

	entries_used = (uint16_t) getBytesAsNumber(tree_pointer + 6, 2, META_DATA_BYTE_ORDER);

	//assuming keys do not contain pertinent information
	left_sibling_address = getBytesAsNumber(tree_pointer + 8, s_block.size_of_offsets, META_DATA_BYTE_ORDER);
	if (left_sibling_address != UNDEF_ADDR)
	{
		trio.parent_obj_header_address = this_trio.parent_obj_header_address;
		trio.tree_address = left_sibling_address + s_block.base_address;
		trio.heap_address = this_trio.heap_address;
		enqueueTrio(trio);
	}

	right_sibling_address = getBytesAsNumber(tree_pointer + 8 + s_block.size_of_offsets, s_block.size_of_offsets,
									 META_DATA_BYTE_ORDER);
	if (right_sibling_address != UNDEF_ADDR)
	{
		trio.parent_obj_header_address = this_trio.parent_obj_header_address;;
		trio.tree_address = right_sibling_address + s_block.base_address;
		trio.heap_address = this_trio.heap_address;
		enqueueTrio(trio);
	}

	//group node B-Tree traversal (version 0)
	int key_size = s_block.size_of_lengths;
	byte *key_pointer = tree_pointer + 8 + 2 * s_block.size_of_offsets;
	for (int i = 0; i < entries_used; i++)
	{

		trio.parent_obj_header_address = this_trio.parent_obj_header_address;
		trio.tree_address = getBytesAsNumber(key_pointer + key_size, s_block.size_of_offsets, META_DATA_BYTE_ORDER) +
						s_block.base_address;
		trio.heap_address = this_trio.heap_address;

		//in the case where we encounter a struct but have more items ahead
		priorityEnqueueTrio(trio);

		key_pointer += key_size + s_block.size_of_offsets;

	}

}


void readSnod(byte *snod_pointer, byte *heap_pointer, Addr_Trio parent_trio, Addr_Trio this_trio)
{
	uint16_t num_symbols = (uint16_t) getBytesAsNumber(snod_pointer + 6, 2, META_DATA_BYTE_ORDER);
	Object *objects = malloc(sizeof(Object) * num_symbols);
	uint32_t cache_type;
	Addr_Trio trio;
	char *var_name = peekVariableName();

	uint64_t heap_data_segment_size = getBytesAsNumber(heap_pointer + 8, s_block.size_of_lengths,
											 META_DATA_BYTE_ORDER);
	uint64_t heap_data_segment_address =
			getBytesAsNumber(heap_pointer + 8 + 2 * s_block.size_of_lengths, s_block.size_of_offsets,
						  META_DATA_BYTE_ORDER) + s_block.base_address;
	byte *heap_data_segment_pointer = navigateTo(heap_data_segment_address, heap_data_segment_size, HEAP);

	//get to entries
	int sym_table_entry_size = 2 * s_block.size_of_offsets + 4 + 4 + 16;

	for (int i = 0, is_done = FALSE; i < num_symbols && is_done != TRUE; i++)
	{
		objects[i].name_offset = getBytesAsNumber(snod_pointer + 8 + i * sym_table_entry_size,
										  s_block.size_of_offsets, META_DATA_BYTE_ORDER);
		objects[i].parent_obj_header_address = this_trio.parent_obj_header_address;
		objects[i].this_obj_header_address =
				getBytesAsNumber(snod_pointer + 8 + i * sym_table_entry_size + s_block.size_of_offsets,
							  s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
		strcpy(objects[i].name, heap_data_segment_pointer + objects[i].name_offset);
		cache_type = (uint32_t) getBytesAsNumber(
				snod_pointer + 8 + 2 * s_block.size_of_offsets + sym_table_entry_size * i, 4, META_DATA_BYTE_ORDER);
		objects[i].parent_tree_address = parent_trio.tree_address;
		objects[i].this_tree_address = this_trio.tree_address;
		objects[i].sub_tree_address = UNDEF_ADDR;

		//check if we have found the object we're looking for or if we are just adding subobjects
		if ((var_name != NULL && strcmp(var_name, objects[i].name) == 0) || variable_found == TRUE)
		{
			if (variable_found == FALSE)
			{
				flushHeaderQueue();
				flushQueue();
				dequeueVariableName();

				//means this was the last token, so we've found the variable we want
				if (variable_name_queue.length == 0)
				{
					variable_found = TRUE;
					is_done = TRUE;
				}

			}

			enqueueObject(objects[i]);

			//if the variable has been found we should keep going down the tree for that variable
			//all items in the queue should only be subobjects so this is safe
			if (cache_type == 1)
			{

				//if another tree exists for this object, put it on the queue
				trio.parent_obj_header_address = objects[i].this_obj_header_address;
				trio.tree_address =
						getBytesAsNumber(
								snod_pointer + 8 + 2 * s_block.size_of_offsets + 8 + sym_table_entry_size * i,
								s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
				trio.heap_address =
						getBytesAsNumber(
								snod_pointer + 8 + 3 * s_block.size_of_offsets + 8 + sym_table_entry_size * i,
								s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
				objects[i].sub_tree_address = trio.tree_address;
				priorityEnqueueTrio(trio);
				parseHeaderTree();
				snod_pointer = navigateTo(this_trio.tree_address, default_bytes, TREE);
				heap_pointer = navigateTo(this_trio.heap_address, default_bytes, HEAP);

			} else if (cache_type == 2)
			{
				//this object is a symbolic link, the name is stored in the heap at the address indicated in the scratch pad
				objects[i].name_offset = getBytesAsNumber(
						snod_pointer + 8 + 2 * s_block.size_of_offsets + 8 + sym_table_entry_size * i, 4,
						META_DATA_BYTE_ORDER);
				strcpy(objects[i].name, heap_pointer + 8 + 2 * s_block.size_of_lengths + s_block.size_of_offsets +
								    objects[i].name_offset);
			}

		}
	}

	free(objects);
}


void freeDataObjects(Data **objects)
{

	int i = 0;
	while ((END_SENTINEL & objects[i]->type) != END_SENTINEL)
	{

		if (objects[i]->data_arrays.is_mx_used != TRUE && objects[i]->data_arrays.ui8_data != NULL)
		{
			mxFree(objects[i]->data_arrays.ui8_data);
			objects[i]->data_arrays.ui8_data = NULL;
		}

		if (objects[i]->data_arrays.is_mx_used != TRUE && objects[i]->data_arrays.i8_data != NULL)
		{
			mxFree(objects[i]->data_arrays.i8_data);
			objects[i]->data_arrays.i8_data = NULL;
		}

		if (objects[i]->data_arrays.is_mx_used != TRUE && objects[i]->data_arrays.ui16_data != NULL)
		{
			mxFree(objects[i]->data_arrays.ui16_data);
			objects[i]->data_arrays.ui16_data = NULL;
		}

		if (objects[i]->data_arrays.is_mx_used != TRUE && objects[i]->data_arrays.i16_data != NULL)
		{
			mxFree(objects[i]->data_arrays.i16_data);
			objects[i]->data_arrays.i16_data = NULL;
		}

		if (objects[i]->data_arrays.is_mx_used != TRUE && objects[i]->data_arrays.ui32_data != NULL)
		{
			mxFree(objects[i]->data_arrays.ui32_data);
			objects[i]->data_arrays.ui32_data = NULL;
		}

		if (objects[i]->data_arrays.is_mx_used != TRUE && objects[i]->data_arrays.i32_data != NULL)
		{
			mxFree(objects[i]->data_arrays.i32_data);
			objects[i]->data_arrays.i32_data = NULL;
		}

		if (objects[i]->data_arrays.is_mx_used != TRUE && objects[i]->data_arrays.ui64_data != NULL)
		{
			mxFree(objects[i]->data_arrays.ui64_data);
			objects[i]->data_arrays.ui64_data = NULL;
		}

		if (objects[i]->data_arrays.is_mx_used != TRUE && objects[i]->data_arrays.i64_data != NULL)
		{
			mxFree(objects[i]->data_arrays.i64_data);
			objects[i]->data_arrays.i64_data = NULL;
		}

		if (objects[i]->data_arrays.is_mx_used != TRUE && objects[i]->data_arrays.single_data != NULL)
		{
			mxFree(objects[i]->data_arrays.single_data);
			objects[i]->data_arrays.single_data = NULL;
		}

		if (objects[i]->data_arrays.is_mx_used != TRUE && objects[i]->data_arrays.double_data != NULL)
		{
			mxFree(objects[i]->data_arrays.double_data);
			objects[i]->data_arrays.double_data = NULL;
		}

		if (objects[i]->data_arrays.udouble_data != NULL)
		{
			free(objects[i]->data_arrays.udouble_data);
			objects[i]->data_arrays.udouble_data = NULL;
		}

		for (int j = 0; j < objects[i]->chunked_info.num_filters; j++)
		{
			free(objects[i]->chunked_info.filters[j].client_data);
			objects[i]->chunked_info.filters[j].client_data = NULL;
		}

		if (objects[i]->sub_objects != NULL)
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


void freeDataObjectTree(Data *super_object)
{

	if (super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.ui8_data != NULL)
	{
		mxFree(super_object->data_arrays.ui8_data);
		super_object->data_arrays.ui8_data = NULL;
	}

	if (super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.i8_data != NULL)
	{
		mxFree(super_object->data_arrays.i8_data);
		super_object->data_arrays.i8_data = NULL;
	}

	if (super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.ui16_data != NULL)
	{
		mxFree(super_object->data_arrays.ui16_data);
		super_object->data_arrays.ui16_data = NULL;
	}

	if (super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.i16_data != NULL)
	{
		mxFree(super_object->data_arrays.i16_data);
		super_object->data_arrays.i16_data = NULL;
	}

	if (super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.ui32_data != NULL)
	{
		mxFree(super_object->data_arrays.ui32_data);
		super_object->data_arrays.ui32_data = NULL;
	}

	if (super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.i32_data != NULL)
	{
		mxFree(super_object->data_arrays.i32_data);
		super_object->data_arrays.i32_data = NULL;
	}

	if (super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.ui64_data != NULL)
	{
		mxFree(super_object->data_arrays.ui64_data);
		super_object->data_arrays.ui64_data = NULL;
	}

	if (super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.i64_data != NULL)
	{
		mxFree(super_object->data_arrays.i64_data);
		super_object->data_arrays.i64_data = NULL;
	}

	if (super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.single_data != NULL)
	{
		mxFree(super_object->data_arrays.single_data);
		super_object->data_arrays.single_data = NULL;
	}

	if (super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.double_data != NULL)
	{
		mxFree(super_object->data_arrays.double_data);
		super_object->data_arrays.double_data = NULL;
	}

	if (super_object->data_arrays.udouble_data != NULL)
	{
		free(super_object->data_arrays.udouble_data);
		super_object->data_arrays.udouble_data = NULL;
	}

	for (int j = 0; j < super_object->chunked_info.num_filters; j++)
	{
		free(super_object->chunked_info.filters[j].client_data);
		super_object->chunked_info.filters[j].client_data = NULL;
	}

	if (super_object->sub_objects != NULL)
	{
		for (int j = 0; j < super_object->num_sub_objs; j++)
		{
			freeDataObjectTree(super_object->sub_objects[j]);
		}
		free(super_object->sub_objects);
		super_object->sub_objects = NULL;
	}


	free(super_object);
	super_object = NULL;

}


void endHooks(void)
{
	if (maps[0].used == TRUE)
	{
		munmap(maps[0].map_start, maps[0].bytes_mapped);
		maps[0].used = FALSE;
	}

	if (maps[1].used == TRUE)
	{
		munmap(maps[1].map_start, maps[1].bytes_mapped);
		maps[1].used = FALSE;
	}

	if (fd >= 0)
	{
		close(fd);
	}

}