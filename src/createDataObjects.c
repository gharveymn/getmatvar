#include "headers/createDataObjects.h"

error_t makeObjectTreeSkeleton(void)
{
	return readTreeNode(virtual_super_object, s_block.root_tree_address, s_block.root_heap_address);
}

error_t readTreeNode(Data* object, uint64_t node_address, uint64_t heap_address)
{
	
	uint16_t entries_used = 0;
	//guess how large the tree is going to be so we don't waste time mapping later
	//mapObject* tree_map_obj = st_navigateTo(node_address, 8);
	mapObject* tree_map_obj = st_navigateTo(node_address, 8);
	byte* tree_pointer = tree_map_obj->address_ptr;
	entries_used = (uint16_t)getBytesAsNumber(tree_pointer + 6, 2, META_DATA_BYTE_ORDER);
	st_releasePages(tree_map_obj);
	
	uint64_t total_size = entries_used*(s_block.size_of_lengths + s_block.size_of_offsets) + (uint64_t)8 + 2*s_block.size_of_offsets;
	
	//group node B-Tree traversal (version 0)
	address_t key_address = node_address + 8 + 2*s_block.size_of_offsets;
	
	//quickly get the total number of possible subobjects first so we can allocate the subobject array
	uint64_t* sub_node_address_list = NULL;
	if(entries_used > 0)
	{
		sub_node_address_list = mxMalloc(entries_used * sizeof(uint64_t));
#ifdef NO_MEX
		if(sub_node_address_list == NULL)
		{
			error_flag = TRUE;
			sprintf(error_id, "getmatvar:mallocErrSUNAL");
			sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
			return 1;
		}
#endif
	}
	else
	{
		error_flag = TRUE;
		sprintf(error_id, "getmatvar:invalidTree");
		sprintf(error_message, "Tried to read from tree with no children. Data may have been corrupted.\n\n");
		return 1;
	}
	
	for(int i = 0; i < entries_used; i++)
	{
		mapObject* key_map_obj = st_navigateTo(key_address, total_size - (key_address - node_address));
		byte* key_pointer = key_map_obj->address_ptr;
		sub_node_address_list[i] = (uint64_t)getBytesAsNumber(key_pointer + s_block.size_of_lengths, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
		st_releasePages(key_map_obj);
		key_address += s_block.size_of_lengths + s_block.size_of_offsets;
	}
	
	for(int i = 0; i < entries_used; i++)
	{
		if(readSnod(object, sub_node_address_list[i], heap_address) != 0)
		{
			mxFree(sub_node_address_list);
			return 1;
		}
	}
	
	mxFree(sub_node_address_list);
	return 0;
	
}

error_t readSnod(Data* object, uint64_t node_address, uint64_t heap_address)
{
	mapObject* snod_map_obj = st_navigateTo(node_address, 8);
	byte* snod_pointer = snod_map_obj->address_ptr;
	if(memcmp(snod_pointer, "TREE", 4) == 0)
	{
		st_releasePages(snod_map_obj);
		readTreeNode(object, node_address, heap_address);
		return 0;
	}
	uint16_t num_symbols = (uint16_t) getBytesAsNumber(snod_pointer + 6, 2, META_DATA_BYTE_ORDER);
	uint64_t total_snod_size = (uint64_t)8 + (num_symbols - 1) * s_block.sym_table_entry_size + 3 * s_block.size_of_offsets + 8 + s_block.size_of_offsets;
	st_releasePages(snod_map_obj);
	snod_map_obj = st_navigateTo(node_address, total_snod_size);
	snod_pointer = snod_map_obj->address_ptr;
	
	uint32_t cache_type;
	
	
	mapObject* heap_map_obj = st_navigateTo(heap_address, (uint64_t) 8 + 2 * s_block.size_of_lengths + s_block.size_of_offsets);
	byte* heap_pointer = heap_map_obj ->address_ptr;
	uint64_t heap_data_segment_size = getBytesAsNumber(heap_pointer + 8, s_block.size_of_lengths, META_DATA_BYTE_ORDER);
	uint64_t heap_data_segment_address = getBytesAsNumber(heap_pointer + 8 + 2*s_block.size_of_lengths, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
	st_releasePages(heap_map_obj);
	mapObject* heap_data_segment_map_obj = st_navigateTo(heap_data_segment_address, heap_data_segment_size);
	byte* heap_data_segment_pointer = heap_data_segment_map_obj->address_ptr;
	
	for(int i = num_symbols - 1; i >= 0; i--)
	{
		uint64_t name_offset = getBytesAsNumber(snod_pointer + 8 + i*s_block.sym_table_entry_size, s_block.size_of_offsets, META_DATA_BYTE_ORDER);
		uint64_t sub_obj_address = getBytesAsNumber(snod_pointer + 8 + i*s_block.sym_table_entry_size + s_block.size_of_offsets, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
		char* name = (char*)(heap_data_segment_pointer + name_offset);
		cache_type = (uint32_t)getBytesAsNumber(snod_pointer + 8 + i*s_block.sym_table_entry_size + 2*s_block.size_of_offsets, 4, META_DATA_BYTE_ORDER);
		
		Data* sub_object = connectSubObject(object, sub_obj_address, name);
		if(sub_object == NULL)
		{
			return 1;
		}
		enqueue(object_queue, sub_object);
		
		//if the variable has been found we should keep going down the tree for that variable
		//all items in the queue should only be subobjects so this is safe
		if(cache_type == 1)
		{
			uint64_t sub_tree_address =
					getBytesAsNumber(snod_pointer + 8 + i*s_block.sym_table_entry_size + 2*s_block.size_of_offsets + 8, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
			uint64_t sub_heap_address =
					getBytesAsNumber(snod_pointer + 8 + i*s_block.sym_table_entry_size + 3*s_block.size_of_offsets + 8, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
			
			//parse the subtree
			st_releasePages(snod_map_obj);
			st_releasePages(heap_data_segment_map_obj);
			
			if(readTreeNode(sub_object, sub_tree_address, sub_heap_address) != 0)
			{
				return 1;
			}
			
			snod_map_obj = st_navigateTo(node_address, total_snod_size);
			snod_pointer = snod_map_obj->address_ptr;
			heap_data_segment_map_obj = st_navigateTo(heap_data_segment_address, heap_data_segment_size);
			heap_data_segment_pointer = heap_data_segment_map_obj->address_ptr;
			
		}
		else if(cache_type == 2)
		{
			//this object is a symbolic link, the name is stored in the heap at the address indicated in the scratch pad
//			name_offset = getBytesAsNumber(snod_pointer + 8 + 2*s_block.size_of_offsets + 8 + s_block.sym_table_entry_size*i, 4, META_DATA_BYTE_ORDER);
//			name = (char*)(heap_pointer + 8 + 2*s_block.size_of_lengths + s_block.size_of_offsets + name_offset);
			
			error_flag = TRUE;
			sprintf(error_id, "getmatvar:unexpectedCacheType");
			sprintf(error_message, "Tried to read an unknown SNOD cache type.\n\n");
			return 1;
		}
		
	}
	
	st_releasePages(snod_map_obj);
	st_releasePages(heap_data_segment_map_obj);
	return 0;
	
}


Data* connectSubObject(Data* super_object, uint64_t sub_obj_address, char* sub_obj_name)
{
	
	Data* sub_object = mxMalloc(sizeof(Data));
#ifdef NO_MEX
	if(sub_object == NULL)
	{
		error_flag = TRUE;
		sprintf(error_id, "getmatvar:mallocErrCSO");
		sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
		return NULL;
	}
#endif
	if(initializeObject(sub_object) != 0)
	{
		return NULL;
	}
	
	sub_object->this_obj_address = sub_obj_address;
	enqueue(super_object->sub_objects, sub_object);
	super_object->num_sub_objs++;
	sub_object->super_object = super_object;
	
	if((super_object->data_flags.is_reference == TRUE) || (super_object->names.short_name_length != 0 && super_object->names.short_name[0] == '#'))
	{
		sub_object->data_flags.is_reference = TRUE;
	}
	
	//the name will be changed later if this is just a reference
//	if((super_object->names.long_name_length == 0 || super_object->names.short_name[0] != '#') && sub_object->data_flags.is_reference != TRUE)
//	{
		//very expensive for some reason---change
		sub_object->names.short_name_length = (uint16_t)strlen(sub_obj_name);
		
		sub_object->names.short_name = mxMalloc((sub_object->names.short_name_length + 1)*sizeof(char));
#ifdef NO_MEX
		if(sub_object->names.short_name == NULL)
		{
			error_flag = TRUE;
			sprintf(error_id, "getmatvar:mallocErrShNaCSO");
			sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
			return NULL;
		}
#endif
		strcpy(sub_object->names.short_name, sub_obj_name);
		
		//append the short name to the data object long name
		if(super_object->names.long_name_length != 0)
		{
			//+1 because of the '.' delimiter
			sub_object->names.long_name_length = super_object->names.long_name_length + (uint16_t)1 + sub_object->names.short_name_length;
			sub_object->names.long_name = mxMalloc((sub_object->names.long_name_length + 1)*sizeof(char));
#ifdef NO_MEX
			if(sub_object->names.long_name == NULL)
			{
				error_flag = TRUE;
				sprintf(error_id, "getmatvar:mallocErrLoNaCSO1");
				sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
				return NULL;
			}
#endif
			strcpy(sub_object->names.long_name, super_object->names.long_name);
			sub_object->names.long_name[super_object->names.long_name_length] = '.';
			strcpy(&sub_object->names.long_name[super_object->names.long_name_length + 1], sub_object->names.short_name);
		}
		else
		{
			sub_object->names.long_name_length = sub_object->names.short_name_length;
			sub_object->names.long_name = mxMalloc((sub_object->names.long_name_length + 1)*sizeof(char));
#ifdef NO_MEX
			if(sub_object->names.long_name == NULL)
			{
				error_flag = TRUE;
				sprintf(error_id, "getmatvar:mallocErrLoNaCSO2");
				sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
				return NULL;
			}
#endif
			strcpy(sub_object->names.long_name, sub_object->names.short_name);
		}
	//}
	
	return sub_object;
	
}