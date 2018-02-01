#include "headers/init.h"


/*this intializer should be called in the entry function before anything else */
void initialize(void)
{
	//init maps, needed so we don't confuse ending hooks in the case of error
	top_level_objects = NULL;
	varname_queue = NULL;
	object_queue = NULL;
	map_objects = NULL;
	page_objects = NULL;
	is_done = FALSE;
	fd = -1;
	num_threads_user_def = -1;
	will_multithread = TRUE;
	will_suppress_warnings = FALSE;
	max_depth = 0;
	error_flag = FALSE;
	virtual_super_object = NULL;
	parameters.full_variable_names = NULL;
	parameters.filename = NULL;
	parameters.num_vars = 0;
	max_num_map_objs = DEFAULT_MAX_NUM_MAP_OBJS;
	is_super_mapped = FALSE;
	super_pointer = NULL;
	memset(error_id, 0, sizeof(error_id));
	memset(error_message, 0, sizeof(error_message));
	memset(warn_id, 0, sizeof(warn_id));
	memset(warn_message, 0, sizeof(warn_message));
#ifdef NO_MEX
	curr_mmap_usage = 0;
	max_mmap_usage = 0;
#ifdef WIN32_LEAN_AND_MEAN
	InitializeCriticalSection(&mmap_usage_update_lock);
#else
	pthread_mutex_init(&mmap_usage_update_lock, NULL);
#endif
#endif
}


void initializeObject(Data* object)
{
	object->data_flags.is_struct_array = FALSE;
	//object->data_flags.is_sparse = FALSE;
	object->data_flags.is_filled = FALSE;
	//object->data_flags.is_finalized = FALSE;
	object->data_flags.is_reference = FALSE;
	object->data_flags.is_mx_used = FALSE;
	object->data_arrays.data = NULL;
	object->data_arrays.sub_object_header_offsets = NULL;
	
	object->matlab_class = NULL;
	
	object->chunked_info.num_filters = 0;
	object->chunked_info.filters = NULL;
	object->chunked_info.num_chunked_dims = 0;
	object->chunked_info.num_chunked_elems = 0;
	object->chunked_info.chunked_dims = NULL;
	object->chunked_info.chunk_update = NULL;
	
	object->super_object = NULL;
	object->sub_objects = initQueue(NULL);
	
	object->matlab_internal_attributes.MATLAB_sparse = FALSE;
	object->matlab_internal_attributes.MATLAB_empty = FALSE;
	object->matlab_internal_attributes.MATLAB_object_decode = NO_OBJ_HINT;
	
	object->names.long_name_length = 0;
	object->names.long_name = NULL;
	object->names.short_name_length = 0;
	object->names.short_name = NULL;
	
	object->layout_class = 3;			//3 by default for non-data
	object->datatype_bit_field = 0;
	object->byte_order = LITTLE_ENDIAN;
	object->hdf5_internal_type = HDF5_UNKNOWN;
	object->matlab_internal_type = mxUNKNOWN_CLASS;
	object->matlab_sparse_type = mxUNKNOWN_CLASS;
	object->complexity_flag = (mxComplexity)mxREAL;
	
	object->num_dims = 0;
	object->dims = NULL;
	object->num_elems = 0;
	object->elem_size = 0;
	object->num_sub_objs = 0;
	object->s_c_array_index = 0;
	object->data_address = UNDEF_ADDR;
	object->this_obj_address = UNDEF_ADDR;
	
}


void initializePageObjects(void)
{
	if(page_objects == NULL)
	{
		page_objects = malloc(num_pages*sizeof(pageObject));
		for(int i = 0; i < num_pages; i++)
		{
#ifdef WIN32_LEAN_AND_MEAN
			InitializeCriticalSection(&page_objects[i].lock);
#else
			pthread_mutex_init(&page_objects[i].lock, NULL);
#endif
			page_objects[i].is_mapped = FALSE;
			page_objects[i].pg_start_a = alloc_gran*i;
			page_objects[i].pg_end_a = MIN(alloc_gran*(i + 1), file_size);
			page_objects[i].map_size = 0;
			page_objects[i].pg_start_p = NULL;
			page_objects[i].num_using = 0;
			page_objects[i].max_map_size = 0;
			page_objects[i].total_num_mappings = 0;
		}
	}
}
