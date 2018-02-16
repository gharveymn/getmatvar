#include "headers/getDataObjects.h"


mapObject* st_navigateTo(address_t address, size_t bytes_needed)
{
	
	address_t map_start = address - (address % alloc_gran); //the start of the page
	size_t map_bytes_needed = (address % alloc_gran) + bytes_needed;
	map_bytes_needed = MIN(map_bytes_needed, file_size - map_start);
	
	initTraversal(map_objects);
	mapObject* obj = NULL;
	//this queue can get very long in recursive cases so set a hard limit
	uint8_t i;
	for(i = 0; i < max_num_map_objs && (obj = (mapObject*)traverseQueue(map_objects)) != NULL; i++)
	{
		if(obj->map_start <= address && address + bytes_needed - 1 <= obj->map_start + obj->map_size - 1 && obj->is_mapped == TRUE)
		{
			obj->num_using++;
			obj->address_ptr = obj->map_start_ptr + (address - obj->map_start);
			return obj;
		}
	}
	
	mapObject* map_obj = mxMalloc(sizeof(mapObject));
#ifdef NO_MEX
	if(map_obj == NULL)
	{
		sprintf(error_id, "getmatvar:mallocErrSTNTMO");
		sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
		return NULL;
	}
#endif
	map_obj->map_start = map_start;
	//map_end = map_end < map_start + alloc_gran? MIN(map_start + alloc_gran, file_size): map_end; //if the mapping is smaller than a page just map the entire page for reuse later
	map_obj->map_size = map_bytes_needed;
	map_obj->num_using = 1;
	map_obj->is_mapped = FALSE;
	map_obj->map_start_ptr = mmap(NULL, map_bytes_needed, PROT_READ, MAP_PRIVATE, fd, map_start);
	if(map_obj->map_start_ptr == NULL || map_obj->map_start_ptr== MAP_FAILED)
	{
		readMXError("getmatvar:mmapUnsuccessfulError", "mmap() unsuccessful in st_navigateTo(). Check errno %d\n\n", errno);
	}
	map_obj->is_mapped = TRUE;
	map_obj->address_ptr = map_obj->map_start_ptr + (address % alloc_gran);
	enqueue(map_objects, map_obj);
	
	if(map_objects->length > max_num_map_objs)
	{
		mapObject* obs_obj = (mapObject*)dequeue(map_objects);
		if(obs_obj->num_using == 0 && obs_obj->is_mapped)
		{
			if(munmap(obs_obj->map_start_ptr, obs_obj->map_size) != 0)
			{
				readMXError("getmatvar:badMunmapError", "munmap() unsuccessful in st_navigateTo(). Check errno %d\n\n", errno);
			}
#ifdef NO_MEX
			curr_mmap_usage -= obs_obj->map_size;
#endif
			obs_obj->map_start = 0;
			obs_obj->map_size = 0;
			obs_obj->map_start_ptr = NULL;
			obs_obj->address_ptr = NULL;
			obs_obj->is_mapped = FALSE;
		}
		
		//clean up the queue if there are too many nodes
		if(map_objects->abs_length > (size_t)4*max_num_map_objs)
		{
			initAbsTraversal(map_objects);
			size_t static_len = map_objects->abs_length, j;
			for(j = 0; j < static_len && (obs_obj = (mapObject*)peekTraverse(map_objects)) != NULL; j++)
			{
				if(obs_obj->num_using == 0)
				{
					removeAtTraverseNode(map_objects);
				}
				else
				{
					traverseQueue(map_objects);
				}
			}
		}
		
	}
	
	
#ifdef NO_MEX
	curr_mmap_usage += map_bytes_needed;
	max_mmap_usage = MAX(curr_mmap_usage, max_mmap_usage);
#endif
	return map_obj;
}


void st_releasePages(mapObject* map_obj)
{
	if(map_obj->num_using == 0)
	{
		readMXError("getmatvar:internalError", "A page was released more than once");
	}
	else
	{
		map_obj->num_using--;
	}
}

//TODO improve this system so that we split the pages between the threads evenly rather than incurring memory overhead ie. lockless parallelization
byte* mt_navigateTo(address_t address, size_t bytes_needed)
{
	
	//If done in parallel while placing chunked data the map size is predetermined while parsing the tree
	
	if(is_super_mapped == TRUE)
	{
		return super_pointer + address;
	}
	
	size_t start_page = address/alloc_gran;
	//size_t end_page = (address + bytes_needed - 1)/alloc_gran; //INCLUSIVE
	
	address_t start_address = page_objects[start_page].pg_start_a;
	
	//0 bytes_needed indicates operation done in parallel, kinda hacky
	size_t map_bytes_needed = bytes_needed != 0 ? (address % alloc_gran) + bytes_needed : page_objects[start_page].max_map_size;
	
	
	/*-----------------------------------------WINDOWS-----------------------------------------*/
#ifdef WIN32_LEAN_AND_MEAN
	
	//in Windows is_mapped becomes a flag for if the mapping originally came from this object
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
		if(map_bytes_needed <= page_objects[start_page].map_size)
		{
		

			EnterCriticalSection(&page_objects[start_page].lock);
			//confirm
			if(map_bytes_needed <= page_objects[start_page].map_size)
			{
				page_objects[start_page].num_using++;
				LeaveCriticalSection(&page_objects[start_page].lock);
				return page_objects[start_page].pg_start_p + (address - page_objects[start_page].pg_start_a);
			}
			else
			{
				LeaveCriticalSection(&page_objects[start_page].lock);
			}
			
		}
		else
		{
			
			//acquire lock if we need to remap
			if(page_objects[start_page].num_using == 0)
			{
				EnterCriticalSection(&page_objects[start_page].lock);
				
				//the state may have changed while acquiring the lock, so check again
				if(page_objects[start_page].num_using == 0)
				{
					page_objects[start_page].num_using++;
					break;
				}
				else
				{
					LeaveCriticalSection(&page_objects[start_page].lock);
				}
			}
		
		}
		
	}
	
	//if this page has already been mapped unmap since we can't fit
	freePageObject(start_page);
	page_objects[start_page].pg_start_p = mmap(NULL, map_bytes_needed, PROT_READ, MAP_PRIVATE, fd, start_address);
	
	if(page_objects[start_page].pg_start_p == NULL || page_objects[start_page].pg_start_p == MAP_FAILED)
	{
		readMXError("getmatvar:mmapUnsuccessfulError", "mmap() unsuccessful in mt_navigateTo(). Check errno %d\n\n", errno);
	}
	
	page_objects[start_page].is_mapped = TRUE;
	page_objects[start_page].map_size = map_bytes_needed;


#ifdef NO_MEX
	EnterCriticalSection(&mmap_usage_update_lock);
	curr_mmap_usage += map_bytes_needed;
	max_mmap_usage = MAX(curr_mmap_usage, max_mmap_usage);
	LeaveCriticalSection(&mmap_usage_update_lock);
#endif
	
	LeaveCriticalSection(&page_objects[start_page].lock);
	
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
		if(map_bytes_needed <= page_objects[start_page].map_size)
		{
			
			pthread_mutex_lock(&page_objects[start_page].lock);
			//confirm
			if(map_bytes_needed <= page_objects[start_page].map_size)
			{
				page_objects[start_page].num_using++;
				pthread_mutex_unlock(&page_objects[start_page].lock);
				return page_objects[start_page].pg_start_p + (address - page_objects[start_page].pg_start_a);
			}
			else
			{
				pthread_mutex_unlock(&page_objects[start_page].lock);
			}
			
		}
		else
		{
			
			//acquire lock if we need to remap
			if(page_objects[start_page].num_using == 0)
			{
			
				pthread_mutex_lock(&page_objects[start_page].lock);
				
				//the state may have changed while acquiring the lock, so check again
				if(page_objects[start_page].num_using == 0)
				{
					page_objects[start_page].num_using++;
					break;
				}
				else
				{
					pthread_mutex_unlock(&page_objects[start_page].lock);
				}
			}
			
		}
		
	}
	
	//if this page has already been mapped unmap since we can't fit
	freePageObject(start_page);
	
	page_objects[start_page].pg_start_p = mmap(NULL, map_bytes_needed, PROT_READ, MAP_PRIVATE, fd, start_address);
	
	if(page_objects[start_page].pg_start_p == NULL || page_objects[start_page].pg_start_p == MAP_FAILED)
	{
		readMXError("getmatvar:mmapUnsuccessfulError", "mmap() unsuccessful in navigateTo(). Check errno %d\n\n", errno);
	}
	
	page_objects[start_page].is_mapped = TRUE;
	page_objects[start_page].map_size = map_bytes_needed;

#ifdef NO_MEX
	pthread_mutex_lock(&mmap_usage_update_lock);
	curr_mmap_usage += map_bytes_needed;
	max_mmap_usage = MAX(curr_mmap_usage, max_mmap_usage);
	pthread_mutex_unlock(&mmap_usage_update_lock);
#endif
	
	pthread_mutex_unlock(&page_objects[start_page].lock);
	
	return page_objects[start_page].pg_start_p + (address - page_objects[start_page].pg_start_a);

#endif

}


void mt_releasePages(address_t address, size_t bytes_needed)
{
	
	if(is_super_mapped == TRUE)
	{
		return;
	}
	
	//call this after done with using the pointer
	size_t start_page = address/alloc_gran;
	//size_t end_page = (address + bytes_needed - 1)/alloc_gran; //INCLUSIVE
	
	/*-----------------------------------------WINDOWS-----------------------------------------*/
#ifdef WIN32_LEAN_AND_MEAN
	
	
	EnterCriticalSection(&page_objects[start_page].lock);
	page_objects[start_page].num_using--;
	
	//0 bytes_needed indicates operation done in parallel
	if(bytes_needed == 0)
	{
		page_objects[start_page].total_num_mappings--;
		//we will free each page if it isnt immediately being used to ensure we don't
		//map twice as much memory as we really need
		//destroys performance
		//idea -- bin each of the needed maps and sort/optimize to unmap only once
		//store number of maps needed and max extent
		if(page_objects[start_page].total_num_mappings == 0)
		{
			freePageObject(start_page);
			//fprintf(stderr, "%d ran", (int) start_page);
		}
	}
	LeaveCriticalSection(&page_objects[start_page].lock);

#else /*-----------------------------------------UNIX-----------------------------------------*/
	
	//release locks
	pthread_mutex_lock(&page_objects[start_page].lock);
	page_objects[start_page].num_using--;
	if(bytes_needed == 0)
	{
		page_objects[start_page].total_num_mappings--;
		
		//we will free each page if it isnt immediately being used to ensure we don't
		//map twice as much memory as we really need
		//destroys performance
		//idea -- bin each of the needed maps and sort/optimize to unmap only once
		//store number of maps needed and max extent
		if(page_objects[start_page].total_num_mappings == 0)
		{
			freePageObject(start_page);
			//fprintf(stderr, "%d\n", (int) start_page);
		}
	}
	pthread_mutex_unlock(&page_objects[start_page].lock);
#endif



}