#include "headers/init.h"
#include "headers/getDataObjects.h"


/*this intializer should be called in the entry function before anything else */
void initialize(void)
{
	//init maps, needed so we don't confuse ending hooks in the case of error
	top_level_objects = NULL;
	varname_queue = NULL;
	object_queue = NULL;
	eval_objects = NULL;
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
}


void initializeObject(Data* object)
{
	object->struct_array_flag = FALSE;
	object->is_filled = FALSE;
	object->is_finalized = FALSE;
	
	object->data_arrays.is_mx_used = FALSE;
	object->data_arrays.data = NULL;
	object->data_arrays.sub_object_header_offsets = NULL;
	
	object->chunked_info.num_filters = 0;
	object->chunked_info.num_chunked_dims = 0;
	object->chunked_info.num_chunked_elems = 0;
	for(int i = 0; i < MAX_NUM_FILTERS; i++)
	{
		object->chunked_info.filters[i].client_data = NULL;
	}
	
	object->super_object = NULL;
	object->sub_objects = initQueue(nullFreeFunction);
	
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
	object->complexity_flag = mxREAL;
	
	object->num_dims = 0;
	object->num_elems = 0;
	object->elem_size = 0;
	object->num_sub_objs = 0;
	object->s_c_array_index = 0;
	object->data_address = UNDEF_ADDR;
	object->this_obj_address = UNDEF_ADDR;
	
}


void initializePageObjects(void)
{
	page_objects = malloc(num_pages*sizeof(pageObject));
	for(int i = 0; i < num_pages; i++)
	{
		pthread_mutex_init(&page_objects[i].lock, NULL);
		//page_objects[i].ready = PTHREAD_COND_INITIALIZER;//initialize these later if we need to?
		//page_objects[i].lock = PTHREAD_MUTEX_INITIALIZER;
		page_objects[i].is_mapped = FALSE;
		page_objects[i].pg_start_a = alloc_gran*i;
		page_objects[i].pg_end_a = MIN(alloc_gran*(i + 1), file_size);
		page_objects[i].map_base = UNDEF_ADDR;
		page_objects[i].map_end = UNDEF_ADDR;
		page_objects[i].pg_start_p = NULL;
		page_objects[i].num_using = 0;
		page_objects[i].last_use_time_stamp = 0;
		page_objects[i].max_map_end = 0;
		page_objects[i].total_num_mappings = 0;
	}
	usage_iterator = 0;
	pthread_spin_init(&if_lock, PTHREAD_PROCESS_SHARED);
}
