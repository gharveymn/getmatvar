#include "headers/getDataObjects.h"
//#pragma message ("getmatvar is compiling on WINDOWS")


/*this is the entry function*/
void getDataObjects(const char* filename, char** variable_names, const int num_names)
{
	
	virtual_super_object = malloc(sizeof(Data));
	initializeObject(virtual_super_object);
	enqueue(object_queue, virtual_super_object);
	
	threads_are_started = FALSE;
	__byte_order__ = getByteOrder();
	alloc_gran = getAllocGran();
	
#ifdef DO_MEMDUMP
	pthread_cond_init(&dump_ready, NULL);
	pthread_mutex_init(&dump_lock, NULL);
	dump = fopen("memdump.log", "w+");
#endif
	
	num_avail_threads = getNumProcessors() - 1;
	
	//init queues
	top_level_objects = initQueue(NULL);
	varname_queue = initQueue(freeVarname);
	map_objects = initQueue(freeMapObject);
	
	//open the file descriptor
	fd = open(filename, O_RDONLY);
	if(fd < 0)
	{
		error_flag = TRUE;
		sprintf(error_id, "getmatvar:fileNotFoundError");
		sprintf(error_message, "No file found with name \'%s\'.\n\n", filename);
		return;
	}
	
	//get file size
	file_size = (size_t)lseek(fd, 0, SEEK_END);
	if(file_size == (size_t)-1)
	{
		error_flag = TRUE;
		sprintf(error_id, "getmatvar:lseekFailureError");
		sprintf(error_message, "lseek failed, check errno %s\n\n", strerror(errno));
		return;
	}
	default_bytes = alloc_gran < file_size? alloc_gran : file_size;
	num_pages = file_size/alloc_gran + 1;
	
	if(file_size < 1024000)
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
		char filetype[MATFILE_SIG_LEN];
		memcpy(filetype, head_pointer, MATFILE_SIG_LEN);
		error_flag = TRUE;
		sprintf(error_id, "getmatvar:wrongFormatError");
		if(memcmp(filetype, "MATLAB", 6) == 0)
		{
			sprintf(error_message, "The input file must be a Version 7.3+ MAT-file. This is a %s.\n\n", filetype);
		}
		else
		{
			sprintf(error_message, "The input file must be a Version 7.3+ MAT-file. This is an unknown file format.\n\n");
		}
		return;
	}
	st_releasePages(head_map_obj);
	
	//find superblock
	s_block = getSuperblock();
	
	makeObjectTreeSkeleton();
	
	for(int name_index = 0; name_index < num_names; name_index++)
	{
		fillVariable(variable_names[name_index]);
	}
	
	close(fd);
	fd = -1;
	
	freeQueue(varname_queue);
	varname_queue = NULL;
	destroyPageObjects();

#ifdef DO_MEMDUMP
	pthread_cond_destroy(&dump_ready);
	pthread_mutex_destroy(&dump_lock);
#endif
	
	virtual_super_object->num_sub_objs = (uint32_t)top_level_objects->length;
	
	if(virtual_super_object->num_sub_objs > 0)
	{
		flushQueue(virtual_super_object->sub_objects);
		restartQueue(top_level_objects);
		while(top_level_objects->length > 0)
		{
			Data* obj = dequeue(top_level_objects);
			enqueue(virtual_super_object->sub_objects, obj);
			obj->super_object = NULL;
			obj->s_c_array_index = 0;
		}
		restartQueue(top_level_objects);
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

}