#include "headers/cleanup.h"


void freeVarname(void* vn)
{
	char* varname = (char*)vn;
	if(varname != NULL && strcmp(varname, "\0") != 0)
	{
		free(varname);
	}
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
			mxFree(data_object->data_arrays.data);
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

void destroyPageObjects(void)
{
	if(page_objects != NULL)
	{
		for(int i = 0; i < num_pages; ++i)
		{
			if(page_objects[i].is_mapped == TRUE)
			{
				
				if(munmap(page_objects[i].pg_start_p, page_objects[i].map_end - page_objects[i].map_base) != 0)
				{
					readMXError("getmatvar:badMunmapError", "munmap() unsuccessful in freeMap(). Check errno %s\n\n", strerror(errno));
				}
				
				page_objects[i].is_mapped = FALSE;
				page_objects[i].pg_start_p = NULL;
				page_objects[i].map_base = UNDEF_ADDR;
				page_objects[i].map_end = UNDEF_ADDR;
				
			}
			
			pthread_mutex_destroy(&page_objects[i].lock);
			
		}
		
		pthread_spin_destroy(&if_lock);
		free(page_objects);
		page_objects = NULL;
		
	}
	
}

void endHooks(void)
{
	
	freeQueue(varname_queue);
	freeQueue(object_queue);
	freeQueue(eval_objects);
	freeQueue(top_level_objects);
	
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