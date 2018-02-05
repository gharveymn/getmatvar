#include "headers/utils.h"


/*for use on a single level*/
Data* findSubObjectByShortName(Data* object, char* name)
{
	restartQueue(object->sub_objects);
	while(object->sub_objects->length > 0)
	{
		Data* obj = dequeue(object->sub_objects);
		if(strcmp(obj->names.short_name, name) == 0)
		{
			restartQueue(object->sub_objects);
			return obj;
		}
	}
	restartQueue(object->sub_objects);
	return NULL;
}


/*for use on a single level*/
Data* findSubObjectBySCIndex(Data* object, index_t index)
{
	restartQueue(object->sub_objects);
	while(object->sub_objects->length > 0)
	{
		Data* obj = dequeue(object->sub_objects);
		if(obj->s_c_array_index == index)
		{
			restartQueue(object->sub_objects);
			return obj;
		}
	}
	restartQueue(object->sub_objects);
	return NULL;
}


/*searches entire tree*/
Data* findObjectByHeaderAddress(address_t obj_address)
{
	
	restartQueue(object_queue);
	while(object_queue->length > 0)
	{
		Data* obj = dequeue(object_queue);
		if(obj->this_obj_address == obj_address)
		{
			restartQueue(object_queue);
			return obj;
		}
	}
	restartQueue(object_queue);
	
	return NULL;
	
}


error_t parseCoordinates(VariableNameToken* vnt)
{
	char* delim = ",", * coord_num_str;
	size_t vnlen = strlen(vnt->variable_local_name);
	char* variable_name_cpy = mxMalloc((vnlen + 1)*sizeof(char));
#ifdef NO_MEX
	if(variable_name_cpy == NULL)
	{
		sprintf(error_id, "getmatvar:mallocErrVNCcoord");
		sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
		return 1;
	}
#endif
	memcpy(variable_name_cpy, vnt->variable_local_name, vnlen*sizeof(char));
	variable_name_cpy[vnlen] = '\0';
	coord_num_str = strtok(variable_name_cpy, delim);
	for(int i = 0; coord_num_str != NULL; i++)
	{
		size_t coord_num_str_len = strlen(coord_num_str);
		vnt->variable_local_coordinates[i] = 0;
		for(uint8_t j = 0; j < coord_num_str_len; j++)
		{
			vnt->variable_local_coordinates[i] = vnt->variable_local_coordinates[i]*10 + (coord_num_str[j] - '0');
		}
		coord_num_str = strtok(NULL, delim);
	}
	mxFree(variable_name_cpy);
	return 0;
}


index_t coordToInd(const index_t* coords, const index_t* dims, uint8_t num_dims)
{
	index_t ret = 0;
	index_t mult = 1;
	for(int i = 0; i < num_dims; i++)
	{
		ret += (coords[i] - 1)*mult;
		mult *= dims[i];
	}
	return ret;
}


Data* cloneData(Data* old_object)
{
	
	Data* new_object = mxMalloc(sizeof(Data));
#ifdef NO_MEX
	if(new_object == NULL)
	{
		sprintf(error_id, "getmatvar:mallocErrNewObjCD");
		sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
		return NULL;
	}
#endif
	//init so we can free cleanly if there are any errors
	if(initializeObject(new_object) != 0)
	{
		return NULL;
	}
	
	new_object->layout_class = old_object->layout_class;               //3 by default for non-data
	new_object->datatype_bit_field = old_object->datatype_bit_field;
	new_object->byte_order = old_object->byte_order;
	new_object->hdf5_internal_type = old_object->hdf5_internal_type;
	new_object->matlab_internal_type = old_object->matlab_internal_type;
	new_object->matlab_sparse_type = old_object->matlab_sparse_type;
	new_object->complexity_flag = old_object->complexity_flag;
	
	new_object->num_dims = old_object->num_dims;
	new_object->num_elems = old_object->num_elems;
	new_object->elem_size = old_object->elem_size;
	new_object->num_sub_objs = old_object->num_sub_objs;
	new_object->s_c_array_index = old_object->s_c_array_index;
	new_object->data_address = old_object->data_address;
	new_object->this_obj_address = old_object->this_obj_address;
	
	
	new_object->data_flags.is_struct_array = old_object->data_flags.is_struct_array;
	new_object->data_flags.is_filled = old_object->data_flags.is_filled;
	new_object->data_flags.is_reference = old_object->data_flags.is_reference;
	new_object->data_flags.is_mx_used = old_object->data_flags.is_mx_used;
	if(old_object->data_arrays.data != NULL)
	{
		new_object->data_arrays.data = mxMalloc((size_t)old_object->num_elems*old_object->elem_size);
#ifdef NO_MEX
		if(new_object->data_arrays.data == NULL)
		{
			sprintf(error_id, "getmatvar:mallocErrDataCD");
			sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
			freeDataObject(new_object);
			freeDataObject(new_object);
			return NULL;
		}
#endif
		memcpy(new_object->data_arrays.data, old_object->data_arrays.data, (size_t)old_object->num_elems*old_object->elem_size);
	}
	else
	{
		new_object->data_arrays.data = NULL;
	}
	
	if(old_object->data_arrays.sub_object_header_offsets != NULL)
	{
		new_object->data_arrays.sub_object_header_offsets = mxMalloc((size_t)old_object->num_elems*old_object->elem_size);
#ifdef NO_MEX
		if(new_object->data_arrays.sub_object_header_offsets  == NULL)
		{
			sprintf(error_id, "getmatvar:mallocErrSOHOCD");
			sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
			freeDataObject(new_object);
			freeDataObject(new_object);
			return NULL;
		}
#endif
		memcpy(new_object->data_arrays.sub_object_header_offsets, old_object->data_arrays.sub_object_header_offsets, (size_t)old_object->num_elems*old_object->elem_size);
	}
	else
	{
		new_object->data_arrays.sub_object_header_offsets = NULL;
	}
	
	new_object->chunked_info.num_filters = old_object->chunked_info.num_filters;
	new_object->chunked_info.num_chunked_dims = old_object->chunked_info.num_chunked_dims;
	new_object->chunked_info.num_chunked_elems = old_object->chunked_info.num_chunked_elems;
	if(old_object->chunked_info.filters != NULL)
	{
		new_object->chunked_info.filters = mxMalloc(new_object->chunked_info.num_filters*sizeof(Filter));
#ifdef NO_MEX
		if(new_object->chunked_info.filters  == NULL)
		{
			sprintf(error_id, "getmatvar:mallocErrCIFCD");
			sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
			freeDataObject(new_object);
			freeDataObject(new_object);
			return NULL;
		}
#endif
		for(int i = 0; i < old_object->chunked_info.num_filters; i++)
		{
			new_object->chunked_info.filters[i].filter_id = old_object->chunked_info.filters[i].filter_id;
			new_object->chunked_info.filters[i].num_client_vals = old_object->chunked_info.filters[i].num_client_vals;
			new_object->chunked_info.filters[i].optional_flag = old_object->chunked_info.filters[i].optional_flag;
			new_object->chunked_info.filters[i].client_data = mxMalloc(new_object->chunked_info.filters[i].num_client_vals*sizeof(uint32_t));
#ifdef NO_MEX
			if(new_object->chunked_info.filters[i].client_data  == NULL)
			{
				sprintf(error_id, "getmatvar:mallocErrCIFCDCD");
				sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
				freeDataObject(new_object);
				freeDataObject(new_object);
				return NULL;
			}
#endif
			for(int j = 0; j < new_object->chunked_info.filters[i].num_client_vals; j++)
			{
				new_object->chunked_info.filters[i].client_data[j] = old_object->chunked_info.filters[i].client_data[j];
			}
		}
	}
	else
	{
		old_object->chunked_info.filters = NULL;
	}
	
	if(old_object->chunked_info.chunked_dims != NULL)
	{
		new_object->chunked_info.chunked_dims = mxMalloc(new_object->chunked_info.num_chunked_elems*sizeof(index_t));
#ifdef NO_MEX
		if(new_object->chunked_info.chunked_dims  == NULL)
		{
			sprintf(error_id, "getmatvar:mallocErrCICDCD");
			sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
			freeDataObject(new_object);
			freeDataObject(new_object);
			return NULL;
		}
#endif
		memcpy(new_object->chunked_info.chunked_dims, old_object->chunked_info.chunked_dims, new_object->chunked_info.num_chunked_elems*sizeof(index_t));
	}
	else
	{
		new_object->chunked_info.chunked_dims = NULL;
	}
	
	if(old_object->chunked_info.chunk_update != NULL)
	{
		new_object->chunked_info.chunk_update = mxMalloc(new_object->chunked_info.num_chunked_elems*sizeof(index_t));
#ifdef NO_MEX
		if(new_object->chunked_info.chunk_update  == NULL)
		{
			sprintf(error_id, "getmatvar:mallocErrCICUCD");
			sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
			freeDataObject(new_object);
			freeDataObject(new_object);
			return NULL;
		}
#endif
		memcpy(new_object->chunked_info.chunk_update, old_object->chunked_info.chunk_update, new_object->chunked_info.num_chunked_elems*sizeof(index_t));
	}
	else
	{
		new_object->chunked_info.chunk_update = NULL;
	}
	
	if(old_object->dims != NULL)
	{
		new_object->dims = mxMalloc(new_object->num_dims*sizeof(index_t));
#ifdef NO_MEX
		if(new_object->dims  == NULL)
		{
			sprintf(error_id, "getmatvar:mallocErrDCD");
			sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
			freeDataObject(new_object);
			freeDataObject(new_object);
			return NULL;
		}
#endif
		memcpy(new_object->dims, old_object->dims, new_object->num_dims*sizeof(index_t));
	}
	else
	{
		new_object->dims = NULL;
	}
	
	
	new_object->super_object = old_object->super_object;
	while(old_object->sub_objects->length > 0)
	{
		enqueue(new_object->sub_objects, dequeue(old_object->sub_objects));
	}
	restartQueue(old_object->sub_objects);
	
	new_object->matlab_internal_attributes.MATLAB_sparse = old_object->matlab_internal_attributes.MATLAB_sparse;
	new_object->matlab_internal_attributes.MATLAB_empty = old_object->matlab_internal_attributes.MATLAB_empty;
	new_object->matlab_internal_attributes.MATLAB_object_decode = old_object->matlab_internal_attributes.MATLAB_object_decode;
	
	new_object->names.long_name_length = old_object->names.long_name_length;
	new_object->names.long_name = mxMalloc((new_object->names.long_name_length + 1)*sizeof(char));
#ifdef NO_MEX
	if(new_object->names.long_name  == NULL)
	{
		sprintf(error_id, "getmatvar:mallocErrNLNCD");
		sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
		freeDataObject(new_object);
		freeDataObject(new_object);
		return NULL;
	}
#endif
	strcpy(new_object->names.long_name, old_object->names.long_name);
	new_object->names.short_name_length = old_object->names.short_name_length;
	new_object->names.short_name = mxMalloc((new_object->names.short_name_length + 1)*sizeof(char));
#ifdef NO_MEX
	if(new_object->names.short_name  == NULL)
	{
		sprintf(error_id, "getmatvar:mallocErrNSNCD");
		sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
		freeDataObject(new_object);
		freeDataObject(new_object);
		return NULL;
	}
#endif
	strcpy(new_object->names.short_name, old_object->names.short_name);
	
	enqueue(object_queue, new_object);
	
	return new_object;
}


error_t makeVarnameQueue(char* variable_name)
{
	
	char* delim = ".{}()", * token;
	flushQueue(varname_queue);
	
	char* variable_name_cpy = mxMalloc((strlen(variable_name) + 1)*sizeof(char));
	strcpy(variable_name_cpy, variable_name);
	removeSpaces(variable_name_cpy);
	token = strtok(variable_name_cpy, delim);
	while(token != NULL)
	{
		size_t vnlen = strlen(token);
		VariableNameToken* varname_token = mxMalloc(sizeof(VariableNameToken));
#ifdef NO_MEX
		if(unlikely(varname_token == NULL))
		{
			//very very unlikely
			sprintf(error_id, "getmatvar:mallocErrVNT");
			sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
			return 1;
		}
#endif
		varname_token->variable_local_name = mxMalloc((vnlen + 1)*sizeof(char));
#ifdef NO_MEX
		if(unlikely(varname_token->variable_local_name == NULL))
		{
			//very very unlikely
			sprintf(error_id, "getmatvar:mallocErrVNTvln");
			sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
			return 1;
		}
#endif
		strcpy(varname_token->variable_local_name, token);
		varname_token->variable_local_index = 0;
		varname_token->variable_name_type = VT_LOCAL_INDEX;
		for(uint8_t i = 0; i < vnlen; i++)
		{
			if(varname_token->variable_local_name[i] < '0' || varname_token->variable_local_name[i] > '9')
			{
				if(varname_token->variable_local_name[i] == ',')
				{
					//indicates that these are coordinates
					varname_token->variable_name_type = VT_LOCAL_COORDINATES;
					break;
				}
				else
				{
					varname_token->variable_local_index = 0;
					varname_token->variable_name_type = VT_LOCAL_NAME;
					break;
				}
			}
			else
			{
				varname_token->variable_local_index = varname_token->variable_local_index*10 + (varname_token->variable_local_name[i] - '0');
			}
		}
		if(varname_token->variable_name_type == VT_LOCAL_INDEX)
		{
			varname_token->variable_local_index--;
		}
		enqueue(varname_queue, varname_token);
		token = strtok(NULL, delim);
	}
	
	while(varname_queue->length > 0)
	{
		VariableNameToken* vnt = dequeue(varname_queue);
		if(vnt->variable_name_type == VT_LOCAL_COORDINATES)
		{
			if(parseCoordinates(vnt) != 0)
			{
				return 1;
			}
		}
	}
	restartQueue(varname_queue);
	
	mxFree(variable_name_cpy);
	return 0;
}


void removeSpaces(char* source)
{
	char* i = source;
	char* j = source;
	while(*j != 0)
	{
		*i = *j++;
		if(*i != ' ')
		{
			i++;
		}
	}
	*i = 0;
}


void readMXError(const char error_id[], const char error_message[], ...)
{
	
	char message_buffer[ERROR_MESSAGE_SIZE] = {0};
	
	va_list va;
	va_start(va, error_message);
	sprintf(message_buffer, error_message, va_arg(va, const char*));
	strcat(message_buffer, MATLAB_HELP_MESSAGE);
	endHooks();
	va_end(va);

#ifdef NO_MEX
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
	fprintf(stderr, message_buffer);
#pragma GCC diagnostic pop
	exit(1);
#else
	mexErrMsgIdAndTxt(error_id, message_buffer);
#endif

}


void readMXWarn(const char warn_id[], const char warn_message[], ...)
{
	
	if(will_suppress_warnings != TRUE)
	{
		char message_buffer[WARNING_MESSAGE_SIZE] = {0};
		
		va_list va;
		va_start(va, warn_message);
		sprintf(message_buffer, warn_message, va_arg(va, const char*));
		strcat(message_buffer, MATLAB_WARN_MESSAGE);
		va_end(va);

#ifdef NO_MEX
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
		fprintf(stderr, message_buffer);
#pragma GCC diagnostic pop
#else
		mexWarnMsgIdAndTxt(warn_id, message_buffer);
#endif
	
	}
	
}