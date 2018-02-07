#include "headers/superblock.h"

Superblock getSuperblock(void)
{
	mapObject* chunk_start_map_obj = findSuperblock();
	byte* superblock_pointer = NULL;
	if(chunk_start_map_obj == NULL)
	{
		readMXError(error_id, error_message);
	}
	else
	{
		superblock_pointer = chunk_start_map_obj->address_ptr;
	}
	Superblock s_block = fillSuperblock(superblock_pointer);
	return s_block;
}

//TODO change this to support entire searching entire file
mapObject* findSuperblock(void)
{
	
	for(address_t chunk_address = 0; (size_t)chunk_address < file_size; chunk_address += SUPERBLOCK_INTERVAL)
	{
		mapObject* chunk_start_map_obj = st_navigateTo(chunk_address, SUPERBLOCK_INTERVAL);
		byte* chunk_start = chunk_start_map_obj->address_ptr;
		if(memcmp(FORMAT_SIG, (char*)chunk_start, 8*sizeof(char)) == 0)
		{
			return chunk_start_map_obj;
		}
		st_releasePages(chunk_start_map_obj);
	}
	
	sprintf(error_id, "getmatvar:superblockNotFoundError");
	sprintf(error_message, "Unable to find the superblock of this file. Data may have been corrupted.\n\n");
	return NULL;
	
}


Superblock fillSuperblock(byte* superblock_pointer)
{
	Superblock s_block;
	//get stuff from superblock, for now assume consistent versions of stuff
	s_block.size_of_offsets = (uint8_t)getBytesAsNumber(superblock_pointer + 13, 1, META_DATA_BYTE_ORDER);
	s_block.size_of_lengths = (uint8_t)getBytesAsNumber(superblock_pointer + 14, 1, META_DATA_BYTE_ORDER);
	s_block.leaf_node_k = (uint16_t)getBytesAsNumber(superblock_pointer + 16, 2, META_DATA_BYTE_ORDER);
	s_block.internal_node_k = (uint16_t)getBytesAsNumber(superblock_pointer + 18, 2, META_DATA_BYTE_ORDER);
	s_block.base_address = (address_t)getBytesAsNumber(superblock_pointer + 24, s_block.size_of_offsets, META_DATA_BYTE_ORDER);
	
	//read scratchpad space
	byte* sps_start = superblock_pointer + 80;
	s_block.root_tree_address = (address_t)getBytesAsNumber(sps_start, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
	s_block.root_heap_address = (address_t)getBytesAsNumber(sps_start + s_block.size_of_offsets, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
	
	s_block.sym_table_entry_size = (uint16_t)(2*s_block.size_of_offsets + 4 + 4 + 16);
	
	return s_block;
}

