#include "mapping.h"
#include "mapping.h"


Superblock getSuperblock(void)
{
	byte* superblock_pointer = findSuperblock();
	Superblock s_block = fillSuperblock(superblock_pointer);
	return s_block;
}


byte* findSuperblock(void)
{
	//Assuming that superblock is in first 8 512 byte chunks
	byte* chunk_start = navigateTo(0, alloc_gran, TREE);
	uint16_t chunk_address = 0;
	
	while(strncmp(FORMAT_SIG, (char*) chunk_start, 8) != 0 && chunk_address < alloc_gran)
	{
		chunk_start += 512;
		chunk_address += 512;
	}
	
	if(chunk_address >= alloc_gran)
	{
		readMXError("getmatvar:superblockNotFoundError", "Couldn't find superblock in first 8 512-byte chunks.\n\n");
	}
	
	return chunk_start;
}


Superblock fillSuperblock(byte* superblock_pointer)
{
	Superblock s_block;
	//get stuff from superblock, for now assume consistent versions of stuff
	s_block.size_of_offsets = (uint8_t) getBytesAsNumber(superblock_pointer + 13, 1, META_DATA_BYTE_ORDER);
	s_block.size_of_lengths = (uint8_t) getBytesAsNumber(superblock_pointer + 14, 1, META_DATA_BYTE_ORDER);
	s_block.leaf_node_k = (uint16_t) getBytesAsNumber(superblock_pointer + 16, 2, META_DATA_BYTE_ORDER);
	s_block.internal_node_k = (uint16_t) getBytesAsNumber(superblock_pointer + 18, 2, META_DATA_BYTE_ORDER);
	s_block.base_address = getBytesAsNumber(superblock_pointer + 24, s_block.size_of_offsets, META_DATA_BYTE_ORDER);
	
	//read scratchpad space
	byte* sps_start = superblock_pointer + 80;
	root_trio.parent_obj_header_address = UNDEF_ADDR;
	root_trio.tree_address =
			getBytesAsNumber(sps_start, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
	root_trio.heap_address =
			getBytesAsNumber(sps_start + s_block.size_of_offsets, s_block.size_of_offsets, META_DATA_BYTE_ORDER) +
			s_block.base_address;
	s_block.root_tree_address = root_trio.tree_address;
	
	return s_block;
}


void freeAllMaps(void)
{
	for(int i = 0; i < NUM_TREE_MAPS; i++)
	{
		if(tree_maps[i].used == TRUE)
		{
			freeMap(tree_maps[i]);
		}
	}
	
	for(int i = 0; i < NUM_HEAP_MAPS; i++)
	{
		if(heap_maps[i].used == TRUE)
		{
			freeMap(heap_maps[i]);
		}
	}
	
	for(int i = 0; i < NUM_THREAD_MAPS; i++)
	{
		if(thread_maps[i].used == TRUE)
		{
			freeMap(thread_maps[i]);
		}
	}
}


void freeMap(MemMap map)
{
	map.used = FALSE;
	if(munmap(map.map_start, map.bytes_mapped) != 0)
	{
		readMXError("getmatvar:badMunmapError", "munmap() unsuccessful in freeMap(). Check errno %s\n\n",
				  strerror(errno));
	}
}


byte* navigateTo(uint64_t address, uint64_t bytes_needed, int map_type)
{
	
	MemMap* these_maps;
	switch(map_type)
	{
		case TREE:
			these_maps = tree_maps;
			break;
		case HEAP:
			these_maps = heap_maps;
			break;
		case THREAD:
			these_maps = thread_maps;
			break;
		default:
			these_maps = tree_maps;
	}
	
	for(int i = 0; i < map_nums[map_type]; i++)
	{
		if(address >= these_maps[i].offset &&
		   address + bytes_needed <= these_maps[i].offset + these_maps[i].bytes_mapped && these_maps[i].used == TRUE)
		{
			return these_maps[i].map_start + address - these_maps[i].offset;
		}
	}
	
	//only remap if we really need to (this removes the need for checks/headaches inside main functions)
	int map_index = map_queue_fronts[map_type];
	
	//unmap current page
	if(these_maps[map_index].used == TRUE)
	{
		freeMap(these_maps[map_index]);
	}
	
	//map new page at needed location
	//TODO check if we even need to do this... I don't think so
	size_t offset_denom = alloc_gran < file_size ? alloc_gran : file_size;
	these_maps[map_index].offset = (OffsetType) ((address / offset_denom) * offset_denom);
	these_maps[map_index].bytes_mapped = address - these_maps[map_index].offset + bytes_needed;
	these_maps[map_index].bytes_mapped = these_maps[map_index].bytes_mapped < file_size - these_maps[map_index].offset
								  ? these_maps[map_index].bytes_mapped : file_size -
																 these_maps[map_index].offset;
	these_maps[map_index].map_start = mmap(NULL, these_maps[map_index].bytes_mapped, PROT_READ, MAP_PRIVATE, fd,
								    these_maps[map_index].offset);
	
	these_maps[map_index].used = TRUE;
	if(these_maps[map_index].map_start == NULL || these_maps[map_index].map_start == MAP_FAILED)
	{
		these_maps[map_index].used = FALSE;
		readMXError("getmatvar:mmapUnsuccessfulError", "mmap() unsuccessful in navigateTo(). Check errno %s\n\n",
				  strerror(errno));
	}
	
	map_queue_fronts[map_type] = (map_queue_fronts[map_type] + 1) % map_nums[map_type];
	
	return these_maps[map_index].map_start + address - these_maps[map_index].offset;
}


byte* navigatePolitely(uint64_t address, uint64_t bytes_needed)
{
	
	size_t start_page = address / alloc_gran;
	size_t end_page = (address + bytes_needed) / alloc_gran; //INCLUSIVE
	
	
	/*-----------------------------------------WINDOWS-----------------------------------------*/
#if (defined(_WIN32) || defined(WIN32) || defined(_WIN64)) && !defined __CYGWIN__
	
	//in Windows the .is_mapped becomes a flag for if the mapping originally came from this object
	//if there is a map available the map_start and map_end addresses indicate where the start and end are
	//if the object is not associated to a map at all the map_start and map_end addresses will be at UNDEF_ADDR
	
	
	//TODO make a bypass here so we don't have like 5 threads waiting for 1 thread to decompress
	//check if we have continuous mapping available (if yes then return pointer)
	
	//acquire lock if we need to remap
	while(TRUE)
	{
		if(page_objects[start_page].map_base <= address && address + bytes_needed <= page_objects[start_page].map_end)
		{

#ifdef DO_MEMDUMP
			
				memdump("R");
#endif
			
			pthread_mutex_lock(&page_objects[start_page].lock);
			page_objects[start_page].num_using++;
			pthread_mutex_unlock(&page_objects[start_page].lock);
			
			return page_objects[start_page].pg_start_p + (address - page_objects[start_page].pg_start_a);
			
		}
		else
		{
			
			//acquire lock if we need to remap
			//lock the if so there isn't deadlock
			pthread_spin_lock(&if_lock);
			if(page_objects[start_page].num_using == 0)
			{
				pthread_mutex_lock(&page_objects[start_page].lock);
				
				//the state may have changed while acquiring the lock, so check again
				if(page_objects[start_page].num_using == 0)
				{
					page_objects[start_page].num_using++;
					pthread_spin_unlock(&if_lock);
					break;
				}
				else
				{
					pthread_mutex_unlock(&page_objects[start_page].lock);
				}
			}
			pthread_spin_unlock(&if_lock);
			
		}
		
	}
	
	
	
	//if this page has already been mapped unmap since we can't fit
	if(page_objects[start_page].is_mapped == TRUE)
	{
		if(munmap(page_objects[start_page].pg_start_p, 0) != 0)
		{
			readMXError("getmatvar:badMunmapError",
					  "munmap() unsuccessful in navigatePolitely(). Check errno %d\n\n",
					  errno);
		}
		
		page_objects[start_page].is_mapped = FALSE;
		page_objects[start_page].pg_start_p = NULL;
		page_objects[start_page].map_base = UNDEF_ADDR;
		page_objects[start_page].map_end = UNDEF_ADDR;

#ifdef DO_MEMDUMP
		
			memdump("U");
#endif
	
	}
	
	page_objects[start_page].pg_start_p = mmap(NULL,
									   page_objects[end_page].pg_end_a - page_objects[start_page].pg_start_a,
									   PROT_READ,
									   MAP_PRIVATE,
									   fd,
									   page_objects[start_page].pg_start_a);
	
	if(page_objects[start_page].pg_start_p == NULL || page_objects[start_page].pg_start_p == MAP_FAILED)
	{
		readMXError("getmatvar:mmapUnsuccessfulError",
				  "mmap() unsuccessful in navigatePolitely(). Check errno %d\n\n",
				  errno);
	}
	
	page_objects[start_page].is_mapped = TRUE;
	page_objects[start_page].map_base = page_objects[start_page].pg_start_a;
	page_objects[start_page].map_end = page_objects[end_page].pg_end_a;

#ifdef DO_MEMDUMP
	
		memdump("M");
#endif
	
	pthread_mutex_unlock(&page_objects[start_page].lock);
	
	return page_objects[start_page].pg_start_p + (address - page_objects[start_page].pg_start_a);

#else /*-----------------------------------------UNIX-----------------------------------------*/
	
	//acquire locks
	//locking this allows for a better chance of map reuse
	pthread_mutex_lock(&thread_acquisition_lock);
	for(uint64_t i = start_page; i <= end_page; i++)
	{
		pthread_mutex_lock(&page_objects[i].lock);
		//this is now my lock
	}
	pthread_mutex_unlock(&thread_acquisition_lock);
	
	//check if we have continuous mapping available (if yes then return pointer)
	
	//only check if it's actually possible (this will be mapped if these are not undef)
	if(page_objects[start_page].map_base <= address && address + bytes_needed <= page_objects[start_page].map_end)
	{
		bool_t is_continuous = TRUE;
		for(size_t i = start_page; i <= end_page; i++)
		{
			if(page_objects[i].map_base != page_objects[start_page].map_base || page_objects[i].is_mapped == FALSE)
			{
				is_continuous = FALSE;
				break;
			}
		}
		
		if(is_continuous)
		{
			
			#ifdef DO_MEMDUMP
			
				memdump("R");
			#endif
			
			return page_objects[start_page].pg_start_p + (address - page_objects[start_page].pg_start_a);
		}
	}
	
	//implicit else
	
	for(size_t i = start_page; i <= end_page; i++)
	{
		if(page_objects[i].is_mapped == TRUE && page_objects[i].pg_start_p != NULL)
		{
			
			if(munmap(page_objects[i].pg_start_p, page_objects[i].pg_end_a - page_objects[i].pg_start_a) != 0)
			{
				readMXError("getmatvar:badMunmapError", "munmap() unsuccessful in navigatePolitely(). Check errno %d\n\n", errno);
			}
			page_objects[i].is_mapped = FALSE;
			page_objects[i].pg_start_p = NULL;
			page_objects[i].map_base = UNDEF_ADDR;
			page_objects[i].map_end = UNDEF_ADDR;
		}
	}
	
#ifdef DO_MEMDUMP
	
		memdump("U");
#endif
	
	page_objects[start_page].pg_start_p = mmap(NULL, page_objects[end_page].pg_end_a - page_objects[start_page].pg_start_a, PROT_READ, MAP_PRIVATE, fd, page_objects[start_page].pg_start_a);
	
	if(page_objects[start_page].pg_start_p == NULL || page_objects[start_page].pg_start_p == MAP_FAILED)
	{
		readMXError("getmatvar:mmapUnsuccessfulError", "mmap() unsuccessful in navigatePolitely(). Check errno %d\n\n", errno);
	}
	
	page_objects[start_page].is_mapped = TRUE;
	page_objects[start_page].map_base = page_objects[start_page].pg_start_a;
	page_objects[start_page].map_end = page_objects[end_page].pg_end_a;
	
	for(size_t i = start_page + 1; i <= end_page; i++)
	{
		page_objects[i].is_mapped = TRUE;
		page_objects[i].pg_start_p = page_objects[i - 1].pg_start_p + alloc_gran;
		page_objects[i].map_base = page_objects[start_page].pg_start_a;
		page_objects[i].map_end = page_objects[end_page].pg_end_a;
    }
	
	#ifdef DO_MEMDUMP
	
		memdump("M");
	#endif
	
	return page_objects[start_page].pg_start_p + (address - page_objects[start_page].pg_start_a);

#endif

}


void releasePages(uint64_t address, uint64_t bytes_needed)
{
	
	//call this after done with using the pointer
	size_t start_page = address / alloc_gran;
	size_t end_page = (address + bytes_needed) / alloc_gran; //INCLUSIVE
	
	/*-----------------------------------------WINDOWS-----------------------------------------*/
#if (defined(_WIN32) || defined(WIN32) || defined(_WIN64)) && !defined __CYGWIN__
	
	pthread_mutex_lock(&page_objects[start_page].lock);
	page_objects[start_page].num_using--;
	pthread_mutex_unlock(&page_objects[start_page].lock);

#else /*-----------------------------------------UNIX-----------------------------------------*/
	
	//release locks
	for (uint64_t j = start_page; j <= end_page; j++)
	{
		pthread_mutex_unlock(&page_objects[j].lock);
	}

#endif

}


byte* navigateWithMapIndex(uint64_t address, uint64_t bytes_needed, int map_type, int map_index)
{
	//TODO fix this...
	//this is actually broken right now since maps aren't completely discrete from each other
	//perhaps implement proper thread pools and use locks?
	
	//but hey at least there's a chance you won't get a bad block
	
	MemMap* these_maps;
	switch(map_type)
	{
		case TREE:
			these_maps = tree_maps;
			break;
		case HEAP:
			these_maps = heap_maps;
			break;
		case THREAD:
			these_maps = thread_maps;
			break;
		default:
			these_maps = tree_maps;
	}
	
	if(!(address >= these_maps[map_index].offset &&
		address + bytes_needed <= these_maps[map_index].offset + these_maps[map_index].bytes_mapped) ||
	   these_maps[map_index].used == FALSE)
	{
		
		//unmap current page
		if(these_maps[map_index].used == TRUE)
		{
			freeMap(these_maps[map_index]);
		}
		
		//map new page at needed location
		size_t offset_denom = alloc_gran < file_size ? alloc_gran : file_size;
		these_maps[map_index].offset = (OffsetType) ((address / offset_denom) * offset_denom);
		these_maps[map_index].bytes_mapped = address - these_maps[map_index].offset + bytes_needed;
		these_maps[map_index].bytes_mapped =
				these_maps[map_index].bytes_mapped < file_size - these_maps[map_index].offset
				? these_maps[map_index].bytes_mapped : file_size - these_maps[map_index].offset;
		these_maps[map_index].map_start = mmap(NULL, these_maps[map_index].bytes_mapped, PROT_READ, MAP_PRIVATE, fd,
									    these_maps[map_index].offset);
		these_maps[map_index].used = TRUE;
		if(these_maps[map_index].map_start == NULL || these_maps[map_index].map_start == MAP_FAILED)
		{
			these_maps[map_index].used = FALSE;
			readMXError("getmatvar:mmapUnsuccessfulError",
					  "mmap() unsuccessful in navigateWithMapIndex(). Check errno %s\n\n", strerror(errno));
		}
		
	}
	
	return these_maps[map_index].map_start + address - these_maps[map_index].offset;
	
}


void readTreeNode(byte* tree_pointer, AddrTrio* this_trio)
{
	AddrTrio* trio;
	uint16_t entries_used = 0;
	uint64_t left_sibling_address, right_sibling_address;
	
	entries_used = (uint16_t) getBytesAsNumber(tree_pointer + 6, 2, META_DATA_BYTE_ORDER);
	
	//assuming keys do not contain pertinent information
	left_sibling_address = getBytesAsNumber(tree_pointer + 8, s_block.size_of_offsets, META_DATA_BYTE_ORDER);
	if(left_sibling_address != UNDEF_ADDR)
	{
		trio = malloc(sizeof(AddrTrio));
		trio->parent_obj_header_address = this_trio->parent_obj_header_address;
		trio->tree_address = left_sibling_address + s_block.base_address;
		trio->heap_address = this_trio->heap_address;
		enqueue(addr_queue, trio);
	}
	
	right_sibling_address = getBytesAsNumber(tree_pointer + 8 + s_block.size_of_offsets, s_block.size_of_offsets,
									 META_DATA_BYTE_ORDER);
	if(right_sibling_address != UNDEF_ADDR)
	{
		trio = malloc(sizeof(AddrTrio));
		trio->parent_obj_header_address = this_trio->parent_obj_header_address;;
		trio->tree_address = right_sibling_address + s_block.base_address;
		trio->heap_address = this_trio->heap_address;
		enqueue(addr_queue, trio);
	}
	
	//group node B-Tree traversal (version 0)
	int key_size = s_block.size_of_lengths;
	byte* key_pointer = tree_pointer + 8 + 2 * s_block.size_of_offsets;
	for(int i = 0; i < entries_used; i++)
	{
		trio = malloc(sizeof(AddrTrio));
		trio->parent_obj_header_address = this_trio->parent_obj_header_address;
		trio->tree_address = getBytesAsNumber(key_pointer + key_size, s_block.size_of_offsets, META_DATA_BYTE_ORDER) +
						 s_block.base_address;
		trio->heap_address = this_trio->heap_address;
		
		//in the case where we encounter a struct but have more items ahead
		priorityEnqueue(addr_queue, trio);
		
		key_pointer += key_size + s_block.size_of_offsets;
		
	}
	
}


void readSnod(byte* snod_pointer, byte* heap_pointer, AddrTrio* parent_trio, AddrTrio* this_trio, bool_t get_top_level)
{
	uint16_t num_symbols = (uint16_t) getBytesAsNumber(snod_pointer + 6, 2, META_DATA_BYTE_ORDER);
	SNODEntry** objects = malloc(num_symbols * sizeof(SNODEntry*));
	uint32_t cache_type;
	AddrTrio* trio;
	char* var_name = peekQueue(varname_queue, QUEUE_FRONT);
	
	uint64_t heap_data_segment_size = getBytesAsNumber(heap_pointer + 8, s_block.size_of_lengths,
											 META_DATA_BYTE_ORDER);
	uint64_t heap_data_segment_address =
			getBytesAsNumber(heap_pointer + 8 + 2 * s_block.size_of_lengths, s_block.size_of_offsets,
						  META_DATA_BYTE_ORDER) + s_block.base_address;
	byte* heap_data_segment_pointer = navigateTo(heap_data_segment_address, heap_data_segment_size, HEAP);
	
	//get to entries
	int sym_table_entry_size = 2 * s_block.size_of_offsets + 4 + 4 + 16;
	
	for(int i = 0, is_done = FALSE; i < num_symbols && is_done != TRUE; i++)
	{
		objects[i] = malloc(sizeof(SNODEntry));
		objects[i]->name_offset = getBytesAsNumber(snod_pointer + 8 + i * sym_table_entry_size,
										   s_block.size_of_offsets, META_DATA_BYTE_ORDER);
		objects[i]->parent_obj_header_address = this_trio->parent_obj_header_address;
		objects[i]->this_obj_header_address =
				getBytesAsNumber(snod_pointer + 8 + i * sym_table_entry_size + s_block.size_of_offsets,
							  s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
		strcpy(objects[i]->name, (char*) (heap_data_segment_pointer + objects[i]->name_offset));
		cache_type = (uint32_t) getBytesAsNumber(
				snod_pointer + 8 + 2 * s_block.size_of_offsets + sym_table_entry_size * i, 4, META_DATA_BYTE_ORDER);
		objects[i]->parent_tree_address = parent_trio->tree_address;
		objects[i]->this_tree_address = this_trio->tree_address;
		objects[i]->sub_tree_address = UNDEF_ADDR;
		
		//check if we have found the object we're looking for or if we are just adding subobjects
		if((var_name != NULL && strcmp(var_name, objects[i]->name) == 0) || variable_found == TRUE)
		{
			if(variable_found == FALSE)
			{
				resetQueue(header_queue);
				resetQueue(addr_queue);
				dequeue(varname_queue);
				
				//means this was the last token, so we've found the variable we want
				if(varname_queue->length == 0)
				{
					variable_found = TRUE;
					is_done = TRUE;
				}
				
			}
			
			if(strncmp(objects[i]->name, "#", 1) != 0 && strncmp(objects[i]->name, "function_handle", 15) != 0)
			{
				enqueue(header_queue, objects[i]);
			}
			
			//if the variable has been found we should keep going down the tree for that variable
			//all items in the queue should only be subobjects so this is safe
			if(cache_type == 1 && strncmp(objects[i]->name, "#", 1) != 0 &&
			   strncmp(objects[i]->name, "function_handle", 15) != 0 && get_top_level == FALSE)
			{
				
				//if another tree exists for this object, put it on the queue
				trio = malloc(sizeof(AddrTrio));
				trio->parent_obj_header_address = objects[i]->this_obj_header_address;
				trio->tree_address =
						getBytesAsNumber(
								snod_pointer + 8 + 2 * s_block.size_of_offsets + 8 + sym_table_entry_size * i,
								s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
				trio->heap_address =
						getBytesAsNumber(
								snod_pointer + 8 + 3 * s_block.size_of_offsets + 8 + sym_table_entry_size * i,
								s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
				objects[i]->sub_tree_address = trio->tree_address;
				priorityEnqueue(addr_queue, trio);
				parseHeaderTree(FALSE);
				snod_pointer = navigateTo(this_trio->tree_address, default_bytes, TREE);
				heap_pointer = navigateTo(this_trio->heap_address, default_bytes, HEAP);
				heap_data_segment_pointer = navigateTo(heap_data_segment_address, heap_data_segment_size, HEAP);
				
			}
			else if(cache_type == 2)
			{
				//this object is a symbolic link, the name is stored in the heap at the address indicated in the scratch pad
				objects[i]->name_offset = getBytesAsNumber(
						snod_pointer + 8 + 2 * s_block.size_of_offsets + 8 + sym_table_entry_size * i, 4,
						META_DATA_BYTE_ORDER);
				strcpy(objects[i]->name,
					  (char*) (heap_pointer + 8 + 2 * s_block.size_of_lengths + s_block.size_of_offsets +
							 objects[i]->name_offset));
			}
			
		}
	}
	
	free(objects);
}


void freeDataObject(void* object)
{
	Data* data_object = (Data*) object;
	
	if(data_object->data_arrays.is_mx_used != TRUE)
	{
		if(data_object->data_arrays.ui8_data != NULL)
		{
			free(data_object->data_arrays.ui8_data);
			data_object->data_arrays.ui8_data = NULL;
		}
		
		if(data_object->data_arrays.i8_data != NULL)
		{
			free(data_object->data_arrays.i8_data);
			data_object->data_arrays.i8_data = NULL;
		}
		
		if(data_object->data_arrays.ui16_data != NULL)
		{
			free(data_object->data_arrays.ui16_data);
			data_object->data_arrays.ui16_data = NULL;
		}
		
		if(data_object->data_arrays.i16_data != NULL)
		{
			free(data_object->data_arrays.i16_data);
			data_object->data_arrays.i16_data = NULL;
		}
		
		if(data_object->data_arrays.ui32_data != NULL)
		{
			free(data_object->data_arrays.ui32_data);
			data_object->data_arrays.ui32_data = NULL;
		}
		
		if(data_object->data_arrays.i32_data != NULL)
		{
			free(data_object->data_arrays.i32_data);
			data_object->data_arrays.i32_data = NULL;
		}
		
		if(data_object->data_arrays.ui64_data != NULL)
		{
			free(data_object->data_arrays.ui64_data);
			data_object->data_arrays.ui64_data = NULL;
		}
		
		if(data_object->data_arrays.i64_data != NULL)
		{
			free(data_object->data_arrays.i64_data);
			data_object->data_arrays.i64_data = NULL;
		}
		
		if(data_object->data_arrays.single_data != NULL)
		{
			free(data_object->data_arrays.single_data);
			data_object->data_arrays.single_data = NULL;
		}
		
		if(data_object->data_arrays.double_data != NULL)
		{
			free(data_object->data_arrays.double_data);
			data_object->data_arrays.double_data = NULL;
		}
	}
	
	if(data_object->data_arrays.sub_object_header_offsets != NULL)
	{
		free(data_object->data_arrays.sub_object_header_offsets);
		data_object->data_arrays.sub_object_header_offsets = NULL;
	}
	
	for(int j = 0; j < data_object->chunked_info.num_filters; j++)
	{
		free(data_object->chunked_info.filters[j].client_data);
		data_object->chunked_info.filters[j].client_data = NULL;
	}
	
	if(data_object->sub_objects != NULL)
	{
		free(data_object->sub_objects);
		data_object->sub_objects = NULL;
	}
	
	
	free(data_object);
	
}


void freeDataObjectTree(Data* super_object)
{
	
	if(super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.ui8_data != NULL)
	{
		free(super_object->data_arrays.ui8_data);
		super_object->data_arrays.ui8_data = NULL;
	}
	
	if(super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.i8_data != NULL)
	{
		free(super_object->data_arrays.i8_data);
		super_object->data_arrays.i8_data = NULL;
	}
	
	if(super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.ui16_data != NULL)
	{
		free(super_object->data_arrays.ui16_data);
		super_object->data_arrays.ui16_data = NULL;
	}
	
	if(super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.i16_data != NULL)
	{
		free(super_object->data_arrays.i16_data);
		super_object->data_arrays.i16_data = NULL;
	}
	
	if(super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.ui32_data != NULL)
	{
		free(super_object->data_arrays.ui32_data);
		super_object->data_arrays.ui32_data = NULL;
	}
	
	if(super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.i32_data != NULL)
	{
		free(super_object->data_arrays.i32_data);
		super_object->data_arrays.i32_data = NULL;
	}
	
	if(super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.ui64_data != NULL)
	{
		free(super_object->data_arrays.ui64_data);
		super_object->data_arrays.ui64_data = NULL;
	}
	
	if(super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.i64_data != NULL)
	{
		free(super_object->data_arrays.i64_data);
		super_object->data_arrays.i64_data = NULL;
	}
	
	if(super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.single_data != NULL)
	{
		free(super_object->data_arrays.single_data);
		super_object->data_arrays.single_data = NULL;
	}
	
	if(super_object->data_arrays.is_mx_used != TRUE && super_object->data_arrays.double_data != NULL)
	{
		free(super_object->data_arrays.double_data);
		super_object->data_arrays.double_data = NULL;
	}
	
	if(super_object->data_arrays.sub_object_header_offsets != NULL)
	{
		free(super_object->data_arrays.sub_object_header_offsets);
		super_object->data_arrays.sub_object_header_offsets = NULL;
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


void endHooks(void)
{
	freeAllMaps();
	
	freeQueue(addr_queue);
	freeQueue(varname_queue);
	freeQueue(header_queue);
	
	if(fd >= 0)
	{
		close(fd);
	}
	
}