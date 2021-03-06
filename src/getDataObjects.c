#include "headers/getDataObjects.h"
//#pragma message ("getmatvar is compiling on WINDOWS")

/*this is the entry function*/
error_t getDataObjects(const char* filename, char** variable_names, const int num_names)
{
	
	__byte_order__ = getByteOrder();
	alloc_gran = getAllocGran();
	
	num_avail_threads = getNumProcessors();
	if((map_objects = initQueue(freeMapObject)) == NULL)
	{
		sprintf(error_id, "getmatvar:initQueueErr");
		sprintf(error_message, "Could not initialize the map_objects queue");
		return 1;
	}
	
	//open the file descriptor
	fd = _open(filename, O_RDONLY);
	if(fd < 0)
	{
		sprintf(error_id, "getmatvar:fileNotFoundError");
		sprintf(error_message, "No file found with name \'%s\'.\n\n", filename);
		return 1;
	}
	
	//get file size
	off_t file_size_check = (off_t)_lseek(fd, 0, SEEK_END);
	if(file_size_check < 0)
	{
		sprintf(error_id, "getmatvar:_lseekFailureError");
		sprintf(error_message, "_lseek failed, check errno %s.\n\n", strerror(errno));
		return 1;
	}
	else if(file_size_check == 0)
	{
		sprintf(error_id, "getmatvar:nullFileError");
		sprintf(error_message, "The file specified is empty.\n\n");
		return 1;
	}
	else
	{
		file_size = (size_t)file_size_check;
	}
	num_pages = file_size/alloc_gran + 1;
	
	//1MB
	if(file_size < ONE_MB)
	{
		is_super_mapped = TRUE;
		will_multithread = FALSE;
		mapObject* super_map_obj = st_navigateTo(0, file_size);
		super_pointer = super_map_obj->address_ptr;
	}
	
	mapObject* head_map_obj = st_navigateTo(0, MATFILE_SIG_LEN);
	byte* head_pointer = head_map_obj->address_ptr;
	if(memcmp(head_pointer, MATFILE_7_3_SIG, MATFILE_SIG_LEN) != 0)
	{
		char filetype[MATFILE_SIG_LEN + 1] = {0};
		memcpy(filetype, head_pointer, MATFILE_SIG_LEN);
		sprintf(error_id, "getmatvar:wrongFormatError");
		if(memcmp(filetype, "MATLAB", 6) == 0)
		{
			sprintf(error_message, "The input file must be a Version 7.3+ MAT-file. This is a %s.\n\n", filetype);
		}
		else
		{
			sprintf(error_message, "The input file must be a Version 7.3+ MAT-file. This is an unknown file format.\n\n");
		}
		st_releasePages(head_map_obj);
		return 1;
	}
	st_releasePages(head_map_obj);
	
	//find superblock
	s_block = getSuperblock();
	
	if((object_queue = initQueue(freeDataObject)) == NULL)
	{
		sprintf(error_id, "getmatvar:initQueueErr");
		sprintf(error_message, "Could not initialize the object queue");
		return 1;
	}
	if((top_level_objects = initQueue(NULL)) == NULL)
	{
		sprintf(error_id, "getmatvar:initQueueErr");
		sprintf(error_message, "Could not initialize the top_level_objects queue");
		return 1;
	}
	if((varname_queue = initQueue(freeVarname)) == NULL)
	{
		sprintf(error_id, "getmatvar:initQueueErr");
		sprintf(error_message, "Could not initialize the varname queue");
		return 1;
	}
	
	virtual_super_object = mxMalloc(sizeof(Data));
#ifdef NO_MEX
	if(virtual_super_object == NULL)
	{
		sprintf(error_id, "getmatvar:mallocErrRPCD");
		sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
		return 1;
	}
#endif
	if(initializeObject(virtual_super_object) != 0)
	{
		return 1;
	}
	enqueue(object_queue, virtual_super_object);
	
	if(makeObjectTreeSkeleton() != 0)
	{
		return 1;
	}
	
	for(int name_index = 0; name_index < num_names; name_index++)
	{
		if(fillVariable(variable_names[name_index]) != 0)
		{
			return 1;
		}
	}
	
	_close(fd);
	fd = -1;
	
	freeQueue(varname_queue);
	varname_queue = NULL;
	destroyPageObjects();
	
	if(is_getting_everything == FALSE)
	{
		initTraversal(top_level_objects);
		for(size_t i = 0; i < top_level_objects->length; i++)
		{
			Data* object = traverseQueue(top_level_objects);
			size_t n = i;
			uint16_t num_digits = 0;
			do
			{
				n /= 10;
				num_digits++;
			} while(n != 0);
			
			if(object->names.short_name_length != 0)
			{
				mxFree(object->names.short_name);
				object->names.short_name = NULL;
				object->names.short_name_length = 0;
			}
			
			if(object->names.long_name_length != 0)
			{
				mxFree(object->names.long_name);
				object->names.long_name = NULL;
				object->names.long_name_length = 0;
			}
			
			object->names.short_name_length = (uint16_t)SELECTION_SIG_LEN + num_digits;
			object->names.short_name = mxMalloc((object->names.short_name_length + 1)*sizeof(char));
#ifdef NO_MEX
			if(object->names.short_name == NULL)
			{
				sprintf(error_id, "getmatvar:mallocErrShNa");
				sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
				return 1;
			}
#endif
			sprintf(object->names.short_name, "%s%d", SELECTION_SIG, (int)i);
			
			object->names.long_name_length = (uint16_t)SELECTION_SIG_LEN + num_digits;
			object->names.long_name = mxMalloc((object->names.long_name_length + 1)*sizeof(char));
#ifdef NO_MEX
			if(object->names.long_name == NULL)
			{
				sprintf(error_id, "getmatvar:mallocErrLoNa");
				sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
				return 1;
			}
#endif
			sprintf(object->names.long_name, "%s%d", SELECTION_SIG, (int)i);
		}
	}
	
	virtual_super_object->num_sub_objs = (uint32_t)top_level_objects->length;
	
	if(virtual_super_object->num_sub_objs > 0)
	{
		flushQueue(virtual_super_object->sub_objects);
		initTraversal(top_level_objects);
		while(top_level_objects->traverse_length > 0)
		{
			Data* obj = traverseQueue(top_level_objects);
			enqueue(virtual_super_object->sub_objects, obj);
			obj->super_object = NULL;
			obj->s_c_array_index = 0;
		}
	}
	
	freeQueue(top_level_objects);
	top_level_objects = NULL;

	freeQueue(map_objects);
	map_objects = NULL;
	
#ifdef NO_MEX
#ifdef WIN32_LEAN_AND_MEAN
	DeleteCriticalSection(&mmap_usage_update_lock);
#else
	pthread_mutex_destroy(&mmap_usage_update_lock);
#endif
#endif
	
	return 0;

}