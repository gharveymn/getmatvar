#include "headers/getDataObjects.h"


mapObject* st_navigateTo(uint64_t address, uint64_t bytes_needed)
{
	
	address_t map_start = address - (address % alloc_gran); //the start of the page
	address_t map_end = address + bytes_needed;
	
	initTraversal(map_objects);
	mapObject* obj = NULL;
	while((obj = (mapObject*)traverseQueue(map_objects)) != NULL)
	{
		if(obj->map_start <= address && map_end <= obj->map_end && obj->is_mapped == TRUE)
		{
			obj->num_using++;
			obj->address_ptr = obj->map_start_ptr + (address - obj->map_start);
			return obj;
		}
	}
	
	mapObject* map_obj = malloc(sizeof(mapObject));
	map_obj->map_start = map_start;
	map_obj->map_end = map_end;
	map_obj->num_using = 1;
	map_obj->is_mapped = FALSE;
	map_obj->map_start_ptr = mmap(NULL, map_end - map_start, PROT_READ, MAP_PRIVATE, fd, map_start);
	if(map_obj->map_start_ptr == NULL || map_obj->map_start_ptr== MAP_FAILED)
	{
		readMXError("getmatvar:mmapUnsuccessfulError", "mmap() unsuccessful in st_navigateTo(). Check errno %d\n\n", errno);
	}
	map_obj->is_mapped = TRUE;
	map_obj->address_ptr = map_obj->map_start_ptr + (address % alloc_gran);
	enqueue(map_objects, map_obj);
	
	if(map_objects->length > max_num_map_objs)
	{
		obj = (mapObject*)peekQueue(map_objects, QUEUE_FRONT);
		if(obj->num_using == 0)
		{
			mapObject* obs_obj = (mapObject*)dequeue(map_objects);
			if(munmap(obs_obj->map_start_ptr, obs_obj->map_end - obs_obj->map_start) != 0)
			{
				readMXError("getmatvar:badMunmapError", "munmap() unsuccessful in st_navigateTo(). Check errno %d\n\n", errno);
			}
			obs_obj->is_mapped = FALSE;
#ifdef NO_MEX
			curr_mmap_usage -= obs_obj->map_end - obs_obj->map_start;
#endif
		}
	}
	
#ifdef NO_MEX
	curr_mmap_usage += (address + bytes_needed) - map_start;
	max_mmap_usage = MAX(curr_mmap_usage, max_mmap_usage);
#endif
	return map_obj;
}


void st_releasePages(mapObject* map_obj)
{
	map_obj->num_using--;
}


byte* mt_navigateTo(uint64_t address, uint64_t bytes_needed)
{
	
	if(is_super_mapped == TRUE)
	{
		return super_pointer + address;
	}
	
	size_t start_page = address/alloc_gran;
	//size_t end_page = (address + bytes_needed - 1)/alloc_gran; //INCLUSIVE
	
	address_t start_address = page_objects[start_page].pg_start_a;
	
	//0 bytes_needed indicates operation done in parallel
	address_t end_address = bytes_needed != 0 ? address + bytes_needed - 1 : page_objects[start_page].max_map_end;
	
	
	/*-----------------------------------------WINDOWS-----------------------------------------*/
#ifdef WIN32_LEAN_AND_MEAN
	
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
		if(page_objects[start_page].map_start <= start_address && end_address <= page_objects[start_page].map_end)
		{
		
#ifdef DO_MEMDUMP
			memdump("R");
#endif

			EnterCriticalSection(&page_objects[start_page].lock);
			//confirm
			if(page_objects[start_page].map_start <= start_address && end_address <= page_objects[start_page].map_end)
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
			//lock the if so there isn't deadlock
			EnterCriticalSection(&if_lock);
			if(page_objects[start_page].num_using == 0)
			{
				EnterCriticalSection(&page_objects[start_page].lock);
				
				//the state may have changed while acquiring the lock, so check again
				if(page_objects[start_page].num_using == 0)
				{
					page_objects[start_page].num_using++;
					LeaveCriticalSection(&if_lock);
					break;
				}
				else
				{
					LeaveCriticalSection(&page_objects[start_page].lock);
				}
			}
			LeaveCriticalSection(&if_lock);
		
		}
		
	}
	
	//if this page has already been mapped unmap since we can't fit
	freePageObject(start_page);
	
	page_objects[start_page].pg_start_p = mmap(NULL, end_address - start_address, PROT_READ, MAP_PRIVATE, fd, start_address);
	
	if(page_objects[start_page].pg_start_p == NULL || page_objects[start_page].pg_start_p == MAP_FAILED)
	{
		readMXError("getmatvar:mmapUnsuccessfulError", "mmap() unsuccessful in mt_navigateTo(). Check errno %d\n\n", errno);
	}
	
	page_objects[start_page].is_mapped = TRUE;
	page_objects[start_page].map_start = start_address;
	page_objects[start_page].map_end = end_address;

#ifdef DO_MEMDUMP
	memdump("M");
#endif


#ifdef NO_MEX
	EnterCriticalSection(&mmap_usage_update_lock);
	curr_mmap_usage += end_address - start_address;
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
		if(page_objects[start_page].map_start <= start_address && end_address <= page_objects[start_page].map_end)
		{

#ifdef DO_MEMDUMP
			memdump("R");
#endif
			
			pthread_mutex_lock(&page_objects[start_page].lock);
			//confirm
			if(page_objects[start_page].map_start <= start_address && end_address <= page_objects[start_page].map_end)
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
	freePageObject(start_page);
	
	page_objects[start_page].pg_start_p = mmap(NULL, end_address - start_address, PROT_READ, MAP_PRIVATE, fd, start_address);
	
	if(page_objects[start_page].pg_start_p == NULL || page_objects[start_page].pg_start_p == MAP_FAILED)
	{
		readMXError("getmatvar:mmapUnsuccessfulError", "mmap() unsuccessful in navigateTo(). Check errno %d\n\n", errno);
	}
	
	page_objects[start_page].is_mapped = TRUE;
	page_objects[start_page].map_start = start_address;
	page_objects[start_page].map_end = end_address;
	
#ifdef DO_MEMDUMP
	memdump("M");
#endif

#ifdef NO_MEX
	pthread_mutex_lock(&mmap_usage_update_lock);
	curr_mmap_usage += end_address - start_address;
	max_mmap_usage = MAX(curr_mmap_usage, max_mmap_usage);
	pthread_mutex_unlock(&mmap_usage_update_lock);
#endif
	
	pthread_mutex_unlock(&page_objects[start_page].lock);
	
	return page_objects[start_page].pg_start_p + (address - page_objects[start_page].pg_start_a);

#endif

}


void mt_releasePages(uint64_t address, uint64_t bytes_needed)
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