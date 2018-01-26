#include "headers/cleanup.h"
#include "headers/getDataObjects.h"


void freeVarname(void* vn)
{
	VariableNameToken* varname_token = (VariableNameToken*)vn;
	if(varname_token != NULL)
	{
		if(varname_token->variable_local_name != NULL && strcmp(varname_token->variable_local_name, "\0") != 0)
		{
			free(varname_token->variable_local_name);
		}
		free(varname_token);
	}
}


void freeDataObject(void* object)
{
	Data* data_object = (Data*)object;
	
	if(data_object->data_flags.is_mx_used != TRUE)
	{
		if(data_object->data_arrays.data != NULL)
		{
			mxFree(data_object->data_arrays.data);
			data_object->data_arrays.data = NULL;
		}
	}
	
	if(data_object->names.short_name_length != 0)
	{
		if (data_object->names.short_name != NULL)
		{
			free(data_object->names.short_name);
			data_object->names.short_name = NULL;
		}
		data_object->names.short_name_length = 0;
	}
	
	if(data_object->names.long_name_length != 0)
	{
		if(data_object->names.long_name != NULL)
		{
			free(data_object->names.long_name);
			data_object->names.long_name = NULL;
		}
		data_object->names.short_name_length = 0;
	}
	
	if(data_object->data_arrays.sub_object_header_offsets != NULL)
	{
		free(data_object->data_arrays.sub_object_header_offsets);
		data_object->data_arrays.sub_object_header_offsets = NULL;
	}
	
	if(data_object->chunked_info.filters != NULL)
	{
		for(int j = 0; j < data_object->chunked_info.num_filters; j++)
		{
			free(data_object->chunked_info.filters[j].client_data);
			data_object->chunked_info.filters[j].client_data = NULL;
		}
		free(data_object->chunked_info.filters);
		data_object->chunked_info.filters = NULL;
	}
	
	if(data_object->dims != NULL)
	{
		free(data_object->dims);
		data_object->dims = NULL;
	}
	
	if(data_object->chunked_info.chunked_dims != NULL)
	{
		free(data_object->chunked_info.chunked_dims);
		data_object->chunked_info.chunked_dims = NULL;
	}
	
	if(data_object->chunked_info.chunk_update != NULL)
	{
		free(data_object->chunked_info.chunk_update);
		data_object->chunked_info.chunk_update = NULL;
	}
	
	if(data_object->matlab_class != NULL)
	{
		free(data_object->matlab_class);
		data_object->matlab_class = NULL;
	}
	
	if(data_object->sub_objects != NULL)
	{
		freeQueue(data_object->sub_objects);
		data_object->sub_objects = NULL;
	}
	
	free(data_object);
	
}


void freeDataObjectTree(Data* data_object)
{
	
	if(data_object->data_flags.is_mx_used != TRUE)
	{
		if(data_object->data_arrays.data != NULL)
		{
			free(data_object->data_arrays.data);
			data_object->data_arrays.data = NULL;
		}
	}
	
	if(data_object->names.short_name_length != 0)
	{
		free(data_object->names.short_name);
		data_object->names.short_name = NULL;
		data_object->names.short_name_length = 0;
	}
	
	if(data_object->names.long_name_length != 0)
	{
		free(data_object->names.long_name);
		data_object->names.long_name = NULL;
		data_object->names.short_name_length = 0;
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
			Data* obj = dequeue(data_object->sub_objects);
			freeDataObjectTree(obj);
		}
		freeQueue(data_object->sub_objects);
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
				
				if(munmap(page_objects[i].pg_start_p, page_objects[i].map_end - page_objects[i].map_start) != 0)
				{
					readMXError("getmatvar:badMunmapError", "munmap() unsuccessful in freeMap(). Check errno %s\n\n", strerror(errno));
				}
				
				page_objects[i].is_mapped = FALSE;
				page_objects[i].pg_start_p = NULL;
				page_objects[i].map_start = UNDEF_ADDR;
				page_objects[i].map_end = UNDEF_ADDR;
				
			}
			
			pthread_mutex_destroy(&page_objects[i].lock);
			
		}
		
		pthread_spin_destroy(&if_lock);
		free(page_objects);
		page_objects = NULL;
		
	}
	
}

void freePageObject(size_t page_index)
{
	
	if(page_objects[page_index].is_mapped == TRUE)
	{
		//second parameter doesnt do anything on windows
		if(munmap(page_objects[page_index].pg_start_p, page_objects[page_index].map_end - page_objects[page_index].map_start) != 0)
		{
			readMXError("getmatvar:badMunmapError", "munmap() unsuccessful in mt_navigateTo(). Check errno %d\n\n", errno);
		}

#ifdef NO_MEX
		pthread_mutex_lock(&mmap_usage_update_lock);
		curr_mmap_usage -= page_objects[page_index].map_end - page_objects[page_index].map_start;
		pthread_mutex_unlock(&mmap_usage_update_lock);
#endif
		
		page_objects[page_index].is_mapped = FALSE;
		page_objects[page_index].pg_start_p = NULL;
		page_objects[page_index].map_start = UNDEF_ADDR;
		page_objects[page_index].map_end = UNDEF_ADDR;

#ifdef DO_MEMDUMP
		memdump("U");
#endif
	
	}
}

void freeMapObject(void* mo)
{
	mapObject* map_obj = (mapObject*)mo;
	if(map_obj != NULL)
	{
		if(map_obj->is_mapped == TRUE)
		{
			if(munmap(map_obj->map_start_ptr, map_obj->map_end - map_obj->map_start) != 0)
			{
				readMXError("getmatvar:badMunmapError", "munmap() unsuccessful in st_releasePages(). Check errno %d\n\n", errno);
			}
			map_obj->is_mapped = FALSE;
#ifdef NO_MEX
			curr_mmap_usage -= map_obj->map_end - map_obj->map_start;
#endif
		}
		free(map_obj);
	}
}

void endHooks(void)
{
	
	freeQueue(varname_queue);
	freeQueue(object_queue);
	freeQueue(top_level_objects);
	freeQueue(map_objects);
	
	if(parameters.full_variable_names != NULL)
	{
		for(int i = 0; i < parameters.num_vars; i++)
		{
			mxFree(parameters.full_variable_names[i]);
		}
		free(parameters.full_variable_names);
	}
	mxFree(parameters.filename);
	
	destroyPageObjects();
	
	if(fd >= 0)
	{
		close(fd);
	}
	
}