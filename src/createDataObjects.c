#include "headers/getDataObjects.h"

void makeObjectTreeSkeleton(void)
{
	readTreeNode(virtual_super_object, s_block.root_tree_address, s_block.root_heap_address);
	
}

void readTreeNode(Data* object, uint64_t node_address, uint64_t heap_address)
{
	
	uint16_t entries_used = 0;
	byte* tree_pointer = st_navigateTo(node_address, 8);
	entries_used = (uint16_t)getBytesAsNumber(tree_pointer + 6, 2, META_DATA_BYTE_ORDER);
	st_releasePages(tree_pointer, node_address, 8);
	
	uint64_t total_size = entries_used*(s_block.size_of_lengths + s_block.size_of_offsets) + (uint64_t)8 + 2*s_block.size_of_offsets;
	
	//group node B-Tree traversal (version 0)
	address_t key_address = node_address + 8 + 2*s_block.size_of_offsets;
	
	//quickly get the total number of possible subobjects first so we can allocate the subobject array
	uint64_t* sub_node_address_list = malloc(entries_used * sizeof(uint64_t));
	for(int i = 0; i < entries_used; i++)
	{
		byte* key_pointer = st_navigateTo(key_address, total_size - (key_address - node_address));
		sub_node_address_list[i] = getBytesAsNumber(key_pointer + s_block.size_of_lengths, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
		st_releasePages(key_pointer, key_address, total_size - (key_address - node_address));
		key_address += s_block.size_of_lengths + s_block.size_of_offsets;
	}
	
	for(int i = 0; i < entries_used; i++)
	{
		readSnod(object, sub_node_address_list[i], heap_address);
	}
	
	free(sub_node_address_list);
	
}

void readSnod(Data* object, uint64_t node_address, uint64_t heap_address)
{
	byte* snod_pointer = st_navigateTo(node_address, 8);
	if(memcmp(snod_pointer, "TREE", 4) == 0)
	{
		st_releasePages(snod_pointer, node_address, 8);
		readTreeNode(object, node_address, heap_address);
		return;
	}
	uint16_t num_symbols = (uint16_t) getBytesAsNumber(snod_pointer + 6, 2, META_DATA_BYTE_ORDER);
	uint64_t total_snod_size = 8 + (num_symbols - 1) * s_block.sym_table_entry_size + 3 * s_block.size_of_offsets + 8 + s_block.size_of_offsets;
	st_releasePages(snod_pointer, node_address, 8);
	snod_pointer = st_navigateTo(node_address, total_snod_size);
	
	uint32_t cache_type;
	
	byte* heap_pointer = st_navigateTo(heap_address, (uint64_t) 8 + 2 * s_block.size_of_lengths + s_block.size_of_offsets);
	uint64_t heap_data_segment_size = getBytesAsNumber(heap_pointer + 8, s_block.size_of_lengths, META_DATA_BYTE_ORDER);
	uint64_t heap_data_segment_address = getBytesAsNumber(heap_pointer + 8 + 2*s_block.size_of_lengths, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
	st_releasePages(heap_pointer, heap_address, (uint64_t) 8 + 2 * s_block.size_of_lengths + s_block.size_of_offsets);
	byte* heap_data_segment_pointer = st_navigateTo(heap_data_segment_address, heap_data_segment_size);
	
	for(int i = num_symbols - 1; i >= 0; i--)
	{
		uint64_t name_offset = getBytesAsNumber(snod_pointer + 8 + i*s_block.sym_table_entry_size, s_block.size_of_offsets, META_DATA_BYTE_ORDER);
		uint64_t sub_obj_address = getBytesAsNumber(snod_pointer + 8 + i*s_block.sym_table_entry_size + s_block.size_of_offsets, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
		char* name = (char*)(heap_data_segment_pointer + name_offset);
		cache_type = (uint32_t)getBytesAsNumber(snod_pointer + 8 + i*s_block.sym_table_entry_size + 2*s_block.size_of_offsets, 4, META_DATA_BYTE_ORDER);
		
		Data* sub_object = connectSubObject(object, sub_obj_address, name);
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
			st_releasePages(snod_pointer, node_address, total_snod_size);
			st_releasePages(heap_data_segment_pointer, heap_data_segment_address, heap_data_segment_size);
			
			readTreeNode(sub_object, sub_tree_address, sub_heap_address);
			
			snod_pointer = st_navigateTo(node_address, total_snod_size);
			heap_data_segment_pointer = st_navigateTo(heap_data_segment_address, heap_data_segment_size);
			
		}
		else if(cache_type == 2)
		{
			//this object is a symbolic link, the name is stored in the heap at the address indicated in the scratch pad
//			name_offset = getBytesAsNumber(snod_pointer + 8 + 2*s_block.size_of_offsets + 8 + s_block.sym_table_entry_size*i, 4, META_DATA_BYTE_ORDER);
//			name = (char*)(heap_pointer + 8 + 2*s_block.size_of_lengths + s_block.size_of_offsets + name_offset);
			
			error_flag = TRUE;
			sprintf(error_id, "getmatvar:unexpectedCacheType");
			sprintf(error_message, "Tried to read an unknown SNOD cache type.\n\n");
			
		}
		
	}
	
	st_releasePages(snod_pointer, node_address, total_snod_size);
	st_releasePages(heap_data_segment_pointer, heap_data_segment_address, heap_data_segment_size);
	
}


Data* connectSubObject(Data* super_object, uint64_t sub_obj_address, char* sub_obj_name)
{
	
	Data* sub_object = malloc(sizeof(Data));
	initializeObject(sub_object);
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
		
		sub_object->names.short_name = malloc((sub_object->names.short_name_length + 1)*sizeof(char));
		strcpy(sub_object->names.short_name, sub_obj_name);
		
		//append the short name to the data object long name
		if(super_object->names.long_name_length != 0)
		{
			//+1 because of the '.' delimiter
			sub_object->names.long_name_length = super_object->names.long_name_length + (uint16_t)1 + sub_object->names.short_name_length;
			sub_object->names.long_name = malloc((sub_object->names.long_name_length + 1)*sizeof(char));
			strcpy(sub_object->names.long_name, super_object->names.long_name);
			sub_object->names.long_name[super_object->names.long_name_length] = '.';
			strcpy(&sub_object->names.long_name[super_object->names.long_name_length + 1], sub_object->names.short_name);
		}
		else
		{
			sub_object->names.long_name_length = sub_object->names.short_name_length;
			sub_object->names.long_name = malloc((sub_object->names.long_name_length + 1)*sizeof(char));
			strcpy(sub_object->names.long_name, sub_object->names.short_name);
		}
	//}
	
	return sub_object;
	
}