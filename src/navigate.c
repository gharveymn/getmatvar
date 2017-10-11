#include "headers/getDataObjects.h"


byte* renavigateTo(uint64_t address, uint64_t bytes_needed)
{
	//use this on subsequent calls to the same address
	releasePages(address, bytes_needed);
	return navigateTo(address, bytes_needed);
}


byte* navigateTo(uint64_t address, uint64_t bytes_needed)
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
			//confirm
			if(page_objects[start_page].map_base <= address && address + bytes_needed <= page_objects[start_page].map_end)
			{
				page_objects[start_page].num_using++;
				page_objects[start_page].last_use_time_stamp = usage_iterator;
				usage_iterator++;
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
	if(page_objects[start_page].is_mapped == TRUE)
	{
		//second parameter doesnt do anything on windows
		if(munmap(page_objects[start_page].pg_start_p, page_objects[start_page].map_end - page_objects[start_page].map_base) != 0)
		{
			readMXError("getmatvar:badMunmapError", "munmap() unsuccessful in navigateTo(). Check errno %d\n\n", errno);
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
		readMXError("getmatvar:mmapUnsuccessfulError", "mmap() unsuccessful in navigateTo(). Check errno %d\n\n", errno);
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
	//size_t end_page = (address + bytes_needed - 1)/alloc_gran; //INCLUSIVE
	
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