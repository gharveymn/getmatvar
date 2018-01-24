#include "headers/getDataObjects.h"


byte* st_renavigateTo(byte* page_start_pointer, uint64_t address, uint64_t bytes_needed)
{
	//use this on subsequent calls to the same address (only on Windows)
	st_releasePages(page_start_pointer, address, bytes_needed);
	return st_navigateTo(address, bytes_needed);
}


byte* st_navigateTo(uint64_t address, uint64_t bytes_needed)
{
	address_t page_start_address = address - (address % alloc_gran);
	byte* page_start_pointer = mmap(NULL, (address + bytes_needed) - page_start_address, PROT_READ, MAP_PRIVATE, fd, page_start_address);
	if(page_start_pointer == NULL || page_start_pointer== MAP_FAILED)
	{
		readMXError("getmatvar:mmapUnsuccessfulError", "mmap() unsuccessful in st_navigateTo(). Check errno %d\n\n", errno);
	}
#ifdef NO_MEX
	curr_mmap_usage += (address + bytes_needed) - page_start_address;
	max_mmap_usage = MAX(curr_mmap_usage, max_mmap_usage);
#endif
	return page_start_pointer + (address % alloc_gran);
}


void st_releasePages(byte* address_pointer, uint64_t address, uint64_t bytes_needed)
{
	address_t page_start_address = address - (address % alloc_gran);
	byte* page_start_pointer = address_pointer - (address % alloc_gran);
	if(munmap(page_start_pointer, (address + bytes_needed) - page_start_address) != 0)
	{
		readMXError("getmatvar:badMunmapError", "munmap() unsuccessful in st_releasePages(). Check errno %d\n\n", errno);
	}
#ifdef NO_MEX
	curr_mmap_usage -= (address + bytes_needed) - page_start_address;
#endif
}


byte* mt_renavigateTo(uint64_t address, uint64_t bytes_needed)
{
	//use this on subsequent calls to the same address
	mt_releasePages(address, bytes_needed);
	return mt_navigateTo(address, bytes_needed);
}


byte* mt_navigateTo(uint64_t address, uint64_t bytes_needed)
{
	
	size_t start_page = address/alloc_gran;
	//size_t end_page = (address + bytes_needed - 1)/alloc_gran; //INCLUSIVE
	
	address_t start_address = page_objects[start_page].pg_start_a;
	
	//0 bytes_needed indicates operation done in parallel
	address_t end_address = bytes_needed != 0 ? address + bytes_needed - 1 : page_objects[start_page].max_map_end;
	
	
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
		if(page_objects[start_page].map_base <= start_address && end_address <= page_objects[start_page].map_end)
		{
		
#ifdef DO_MEMDUMP
			memdump("R");
#endif
			
			pthread_mutex_lock(&page_objects[start_page].lock);
			//confirm
			if(page_objects[start_page].map_base <= start_address && end_address <= page_objects[start_page].map_end)
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
	freePageObject(start_page);
	
	page_objects[start_page].pg_start_p = mmap(NULL, end_address - start_address, PROT_READ, MAP_PRIVATE, fd, start_address);
	
	if(page_objects[start_page].pg_start_p == NULL || page_objects[start_page].pg_start_p == MAP_FAILED)
	{
		readMXError("getmatvar:mmapUnsuccessfulError", "mmap() unsuccessful in mt_navigateTo(). Check errno %d\n\n", errno);
	}
	
	page_objects[start_page].is_mapped = TRUE;
	page_objects[start_page].map_base = start_address;
	page_objects[start_page].map_end = end_address;

#ifdef DO_MEMDUMP
	memdump("M");
#endif
	
	page_objects[start_page].last_use_time_stamp = usage_iterator;
	usage_iterator++;

#ifdef NO_MEX
	pthread_mutex_lock(&mmap_usage_update_lock);
	curr_mmap_usage += end_address - start_address;
	max_mmap_usage = MAX(curr_mmap_usage, max_mmap_usage);
	pthread_mutex_unlock(&mmap_usage_update_lock);
#endif
	
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
		if(page_objects[start_page].map_base <= start_address && end_address <= page_objects[start_page].map_end)
		{

#ifdef DO_MEMDUMP
			memdump("R");
#endif
			
			pthread_mutex_lock(&page_objects[start_page].lock);
			//confirm
			if(page_objects[start_page].map_base <= start_address && end_address <= page_objects[start_page].map_end)
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
	freePageObject(start_page);
	
	page_objects[start_page].pg_start_p = mmap(NULL, end_address - start_address, PROT_READ, MAP_PRIVATE, fd, start_address);
	
	if(page_objects[start_page].pg_start_p == NULL || page_objects[start_page].pg_start_p == MAP_FAILED)
	{
		readMXError("getmatvar:mmapUnsuccessfulError", "mmap() unsuccessful in navigateTo(). Check errno %d\n\n", errno);
	}
	
	page_objects[start_page].is_mapped = TRUE;
	page_objects[start_page].map_base = start_address;
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
	
	//call this after done with using the pointer
	size_t start_page = address/alloc_gran;
	//size_t end_page = (address + bytes_needed - 1)/alloc_gran; //INCLUSIVE
	
	/*-----------------------------------------WINDOWS-----------------------------------------*/
#if (defined(_WIN32) || defined(WIN32) || defined(_WIN64)) && !defined __CYGWIN__
	
	pthread_mutex_lock(&page_objects[start_page].lock);
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
	pthread_mutex_unlock(&page_objects[start_page].lock);

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