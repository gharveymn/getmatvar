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
	byte* chunk_start = navigateTo(0, alloc_gran);
	uint16_t chunk_address = 0;
	
	while(strncmp(FORMAT_SIG, (char*)chunk_start, 8) != 0 && chunk_address < alloc_gran)
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
	s_block.size_of_offsets = (uint8_t)getBytesAsNumber(superblock_pointer + 13, 1, META_DATA_BYTE_ORDER);
	s_block.size_of_lengths = (uint8_t)getBytesAsNumber(superblock_pointer + 14, 1, META_DATA_BYTE_ORDER);
	s_block.leaf_node_k = (uint16_t)getBytesAsNumber(superblock_pointer + 16, 2, META_DATA_BYTE_ORDER);
	s_block.internal_node_k = (uint16_t)getBytesAsNumber(superblock_pointer + 18, 2, META_DATA_BYTE_ORDER);
	s_block.base_address = getBytesAsNumber(superblock_pointer + 24, s_block.size_of_offsets, META_DATA_BYTE_ORDER);
	
	//read scratchpad space
	byte* sps_start = superblock_pointer + 80;
	s_block.root_tree_address = getBytesAsNumber(sps_start, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
	s_block.root_heap_address = getBytesAsNumber(sps_start + s_block.size_of_offsets, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
	
	s_block.sym_table_entry_size = (uint16_t)(2*s_block.size_of_offsets + 4 + 4 + 16);
	
	return s_block;
}


void freeMap(MemMap map)
{
	map.used = FALSE;
	if(munmap(map.map_start, map.bytes_mapped) != 0)
	{
		readMXError("getmatvar:badMunmapError", "munmap() unsuccessful in freeMap(). Check errno %s\n\n", strerror(errno));
	}
}


byte* navigateTo(uint64_t address, uint64_t bytes_needed)
{
	
	size_t start_page = address/alloc_gran;
	size_t end_page = (address + bytes_needed - 1)/alloc_gran; //INCLUSIVE
	
	if(page_objects[start_page].map_base <= address && address + bytes_needed <= page_objects[start_page].map_end)
	{
		page_objects[start_page].last_use_time_stamp = usage_iterator;
		return page_objects[start_page].pg_start_p + (address - page_objects[start_page].pg_start_a);
	}
	
	//unmap current page
	if(page_objects[start_page].is_mapped == TRUE)
	{
		//second parameter doesnt do anything on windows
		if(munmap(page_objects[start_page].pg_start_p, page_objects[start_page].map_end - page_objects[start_page].map_base) != 0)
		{
			readMXError("getmatvar:badMunmapError", "munmap() unsuccessful in navigatePolitely(). Check errno %d\n\n", errno);
		}
		
		page_objects[start_page].is_mapped = FALSE;
		page_objects[start_page].pg_start_p = NULL;
		page_objects[start_page].map_base = UNDEF_ADDR;
		page_objects[start_page].map_end = UNDEF_ADDR;

#ifdef DO_MEMDUMP
		memdump("U");
#endif
	
	}
	
	page_objects[start_page].pg_start_p = mmap(NULL, page_objects[end_page].pg_end_a - page_objects[start_page].pg_start_a, PROT_READ, MAP_PRIVATE, fd, page_objects[start_page].pg_start_a);
	
	if(page_objects[start_page].pg_start_p == NULL || page_objects[start_page].pg_start_p == MAP_FAILED)
	{
		readMXError("getmatvar:mmapUnsuccessfulError", "mmap() unsuccessful in navigatePolitely(). Check errno %d\n\n", errno);
	}
	
	page_objects[start_page].is_mapped = TRUE;
	page_objects[start_page].map_base = page_objects[start_page].pg_start_a;
	page_objects[start_page].map_end = page_objects[end_page].pg_end_a;

#ifdef DO_MEMDUMP
	memdump("M");
#endif
	
	page_objects[start_page].last_use_time_stamp = usage_iterator;
	
	return page_objects[start_page].pg_start_p + (address - page_objects[start_page].pg_start_a);
	
}


byte* navigatePolitely(uint64_t address, uint64_t bytes_needed)
{
	
	size_t start_page = address/alloc_gran;
	size_t end_page = (address + bytes_needed - 1)/alloc_gran; //INCLUSIVE
	
	
	/*-----------------------------------------WINDOWS-----------------------------------------*/
#if ((defined(_WIN32) || defined(WIN32) || defined(_WIN64)) && !defined __CYGWIN__)
	
	//in Windows the .is_mapped becomes a flag for if the mapping originally came from this object
	//if there is a map available the map_start and map_end addresses indicate where the start and end are
	//if the object is not associated to a map at all the map_start and map_end addresses will be UNDEF_ADDR
	
	while(TRUE)
	{
		
		//this covers cases:
		//				already mapped, use this map
		//				already mapped, in use, wait for lock and threads using to finish to remap
		//				already mapped, in use, remapped while waiting, use that map
		//				not mapped, acquire lock to map
		//				not mapped, mapped while waiting for map lock, use that map
		//				not mapped
		
		
		//check if we have continuous mapping available (if yes then return pointer)
		if(page_objects[start_page].map_base <= address && address + bytes_needed <= page_objects[start_page].map_end)
		{
		
#ifdef DO_MEMDUMP
			memdump("R");
#endif
			
			pthread_mutex_lock(&page_objects[start_page].lock);
			page_objects[start_page].num_using++;
			page_objects[start_page].last_use_time_stamp = usage_iterator;
			usage_iterator++;
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
		//second parameter doesnt do anything on windows
		if(munmap(page_objects[start_page].pg_start_p, page_objects[start_page].map_end - page_objects[start_page].map_base) != 0)
		{
			readMXError("getmatvar:badMunmapError", "munmap() unsuccessful in navigatePolitely(). Check errno %d\n\n", errno);
		}
		
		page_objects[start_page].is_mapped = FALSE;
		page_objects[start_page].pg_start_p = NULL;
		page_objects[start_page].map_base = UNDEF_ADDR;
		page_objects[start_page].map_end = UNDEF_ADDR;

#ifdef DO_MEMDUMP
		memdump("U");
#endif
	
	}
	
	page_objects[start_page].pg_start_p = mmap(NULL, page_objects[end_page].pg_end_a - page_objects[start_page].pg_start_a, PROT_READ, MAP_PRIVATE, fd, page_objects[start_page].pg_start_a);
	
	if(page_objects[start_page].pg_start_p == NULL || page_objects[start_page].pg_start_p == MAP_FAILED)
	{
		readMXError("getmatvar:mmapUnsuccessfulError", "mmap() unsuccessful in navigatePolitely(). Check errno %d\n\n", errno);
	}
	
	page_objects[start_page].is_mapped = TRUE;
	page_objects[start_page].map_base = page_objects[start_page].pg_start_a;
	page_objects[start_page].map_end = page_objects[end_page].pg_end_a;

#ifdef DO_MEMDUMP
	memdump("M");
#endif
	
	page_objects[start_page].last_use_time_stamp = usage_iterator;
	usage_iterator++;
	pthread_mutex_unlock(&page_objects[start_page].lock);
	
	return page_objects[start_page].pg_start_p + (address - page_objects[start_page].pg_start_a);

#else /*-----------------------------------------UNIX-----------------------------------------*/
	
	
	//This is the same as on Windows, which I now think my just be a better idea than unmapping subsections.
	//The overhead associated with checks needed to make sure no other maps are using each subsection is
	//probably too high, and it is very difficult to program/error prone.
	
	while(TRUE)
	{
		
		//this covers cases:
		//				already mapped, use this map
		//				already mapped, in use, wait for lock and threads using to finish to remap
		//				already mapped, in use, remapped while waiting, use that map
		//				not mapped, acquire lock to map
		//				not mapped, mapped while waiting for map lock, use that map
		//				not mapped
		
		
		//check if we have continuous mapping available (if yes then return pointer)
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
		if(munmap(page_objects[start_page].pg_start_p, page_objects[start_page].map_end - page_objects[start_page].map_base) != 0)
		{
			readMXError("getmatvar:badMunmapError", "munmap() unsuccessful in navigatePolitely(). Check errno %d\n\n", errno);
		}
		
		page_objects[start_page].is_mapped = FALSE;
		page_objects[start_page].pg_start_p = NULL;
		page_objects[start_page].map_base = UNDEF_ADDR;
		page_objects[start_page].map_end = UNDEF_ADDR;

#ifdef DO_MEMDUMP
		memdump("U");
#endif
	
	}
	
	page_objects[start_page].pg_start_p = mmap(NULL, page_objects[end_page].pg_end_a - page_objects[start_page].pg_start_a, PROT_READ, MAP_PRIVATE, fd, page_objects[start_page].pg_start_a);
	
	if(page_objects[start_page].pg_start_p == NULL || page_objects[start_page].pg_start_p == MAP_FAILED)
	{
		readMXError("getmatvar:mmapUnsuccessfulError", "mmap() unsuccessful in navigatePolitely(). Check errno %d\n\n", errno);
	}
	
	page_objects[start_page].is_mapped = TRUE;
	page_objects[start_page].map_base = page_objects[start_page].pg_start_a;
	page_objects[start_page].map_end = page_objects[end_page].pg_end_a;
	
#ifdef DO_MEMDUMP
	memdump("M");
#endif
	
	pthread_mutex_unlock(&page_objects[start_page].lock);
	
	return page_objects[start_page].pg_start_p + (address - page_objects[start_page].pg_start_a);

#endif

}


void releasePages(uint64_t address, uint64_t bytes_needed)
{
	
	//call this after done with using the pointer
	size_t start_page = address/alloc_gran;
	//size_t end_page = (address + bytes_needed)/alloc_gran; //INCLUSIVE
	
	/*-----------------------------------------WINDOWS-----------------------------------------*/
#if (defined(_WIN32) || defined(WIN32) || defined(_WIN64)) && !defined __CYGWIN__
	
	pthread_mutex_lock(&page_objects[start_page].lock);
	page_objects[start_page].num_using--;
	pthread_mutex_unlock(&page_objects[start_page].lock);

#else /*-----------------------------------------UNIX-----------------------------------------*/
	
	//release locks
	pthread_mutex_lock(&page_objects[start_page].lock);
	page_objects[start_page].num_using--;
	pthread_mutex_unlock(&page_objects[start_page].lock);
#endif

}

void freeDataObject(void* object)
{
	Data* data_object = (Data*)object;
	
	if(data_object->data_arrays.is_mx_used != TRUE)
	{
		if(data_object->data_arrays.data != NULL)
		{
			free(data_object->data_arrays.data);
			data_object->data_arrays.data = NULL;
		}
	}
	
	if(data_object->names.short_name != NULL)
	{
		free(data_object->names.short_name);
		data_object->names.short_name = NULL;
	}
	
	if(data_object->names.long_name != NULL)
	{
		free(data_object->names.long_name);
		data_object->names.long_name = NULL;
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


void freeDataObjectTree(Data* data_object)
{
	
	if(data_object->data_arrays.is_mx_used != TRUE)
	{
		if(data_object->data_arrays.data != NULL)
		{
			free(data_object->data_arrays.data);
			data_object->data_arrays.data = NULL;
		}
	}
	
	if(data_object->names.short_name != NULL)
	{
		free(data_object->names.short_name);
		data_object->names.short_name = NULL;
	}
	
	if(data_object->names.long_name != NULL)
	{
		free(data_object->names.long_name);
		data_object->names.long_name = NULL;
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
		for(int j = 0; j < data_object->num_sub_objs; j++)
		{
			freeDataObjectTree(data_object->sub_objects[j]);
		}
		free(data_object->sub_objects);
		data_object->sub_objects = NULL;
	}
	
	
	free(data_object);
	data_object = NULL;
	
}


void endHooks(void)
{
	
	freeQueue(varname_queue);
	freeQueue(object_queue);
	freeQueue(eval_objects);
	
	if(parameters.full_variable_names != NULL)
	{
		for(int i = 0; i < parameters.num_vars; i++)
		{
			free(parameters.full_variable_names[i]);
		}
		free(parameters.full_variable_names);
	}
	
	destroyPageObjects();
	
	if(fd >= 0)
	{
		close(fd);
	}
	
}