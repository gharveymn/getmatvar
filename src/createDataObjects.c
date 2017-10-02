#include "headers/getDataObjects.h"

void makeObjectTreeSkeleton(void)
{
	readTreeNode(super_object, s_block.root_tree_address, s_block.root_heap_address);
}

void readTreeNode(Data* object, uint64_t node_address, uint64_t heap_address)
{
	
	uint16_t entries_used = 0;
	byte* tree_pointer = navigateTo(node_address, 8);
	entries_used = (uint16_t)getBytesAsNumber(tree_pointer + 6, 2, META_DATA_BYTE_ORDER);
	releasePages(node_address, 8);
	
	uint64_t total_size = entries_used*(s_block.size_of_lengths + s_block.size_of_offsets) + (uint64_t)8 + 2*s_block.size_of_offsets;
	
	//group node B-Tree traversal (version 0)
	address_t key_address = node_address + 8 + 2*s_block.size_of_offsets;
	
	//quickly get the total number of possible subobjects first so we can allocate the subobject array
	uint16_t max_num_sub_objs = 0; //may be fewer than this number by throwing away the refs and such things
	uint64_t* sub_node_address_list = malloc(entries_used * sizeof(uint64_t));
	for(int i = 0; i < entries_used; i++)
	{
		byte* key_pointer = navigateTo(key_address, total_size - (key_address - node_address));
		sub_node_address_list[i] = getBytesAsNumber(key_pointer + s_block.size_of_lengths, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
		releasePages(key_address,total_size - (key_address - node_address));
		max_num_sub_objs += getNumSymbols(sub_node_address_list[i]);
		key_address += s_block.size_of_lengths + s_block.size_of_offsets;
	}
	
	if(object->sub_objects == NULL)
	{
		object->sub_objects = malloc(max_num_sub_objs*sizeof(Data*));
	}
	
	for(int i = 0; i < entries_used; i++)
	{
		readSnod(object, sub_node_address_list[i], heap_address);
	}
	
	free(sub_node_address_list);
	
}

void readSnod(Data* object, uint64_t node_address, uint64_t heap_address)
{
	
	byte* snod_pointer = navigateTo(node_address, 8);
	uint16_t num_symbols = (uint16_t)getBytesAsNumber(snod_pointer + 6, 2, META_DATA_BYTE_ORDER);
	snod_pointer = renavigateTo(node_address, num_symbols * s_block.sym_table_entry_size);
	
	uint32_t cache_type;
	
	byte* heap_pointer = navigateTo(heap_address, (uint64_t)8 + 2*s_block.size_of_lengths);
	uint64_t heap_data_segment_size = getBytesAsNumber(heap_pointer + 8, s_block.size_of_lengths, META_DATA_BYTE_ORDER);
	uint64_t heap_data_segment_address = getBytesAsNumber(heap_pointer + 8 + 2*s_block.size_of_lengths, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
	releasePages(heap_address, (uint64_t)8 + 2*s_block.size_of_lengths);
	byte* heap_data_segment_pointer = navigateTo(heap_data_segment_address, heap_data_segment_size);
	
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
			
			//parse a subtree
			releasePages(node_address, num_symbols * s_block.sym_table_entry_size);
			releasePages(heap_data_segment_address, heap_data_segment_size);
			
			readTreeNode(sub_object, sub_tree_address, sub_heap_address);
			
			snod_pointer = navigateTo(node_address, num_symbols * s_block.sym_table_entry_size);
			heap_data_segment_pointer = navigateTo(heap_data_segment_address, heap_data_segment_size);
			
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
	
	releasePages(node_address, num_symbols * s_block.sym_table_entry_size);
	releasePages(heap_data_segment_address, heap_data_segment_size);
	
}


Data* connectSubObject(Data* object, uint64_t sub_obj_address, char* name)
{
	
	Data* sub_object = malloc(sizeof(Data));
	initializeObject(sub_object);
	sub_object->this_obj_address = sub_obj_address;
	sub_object->names.short_name_length = (uint16_t)strlen(name);
	sub_object->names.short_name = malloc((sub_object->names.short_name_length + 1) * sizeof(char));
	strcpy(sub_object->names.short_name, name);
	object->sub_objects[object->num_sub_objs] = sub_object;
	object->num_sub_objs++;
	
	
	//append the short name to the data object long name
	if(object->names.long_name_length != 0)
	{
		//+1 because of the '.' delimiter
		sub_object->names.long_name_length = object->names.long_name_length + (uint16_t)1 + sub_object->names.short_name_length;
		sub_object->names.long_name = malloc((sub_object->names.long_name_length + 1) * sizeof(char));
		strcpy(sub_object->names.long_name, object->names.long_name);
		sub_object->names.long_name[object->names.long_name_length] = '.';
		strcpy(&sub_object->names.long_name[object->names.long_name_length + 1], sub_object->names.short_name);
	}
	else
	{
		sub_object->names.long_name_length = sub_object->names.short_name_length;
		sub_object->names.long_name = malloc((sub_object->names.long_name_length + 1) * sizeof(char));
		strcpy(sub_object->names.long_name, sub_object->names.short_name);
	}
	
	return sub_object;
	
}


uint16_t getNumSymbols(uint64_t address)
{
	byte* p = navigateTo(address, 8);
	uint16_t ret = (uint16_t)getBytesAsNumber(p + 6, 2, META_DATA_BYTE_ORDER);
	releasePages(address, 8);
	return ret;
}