#include "headers/superblock.h"

Superblock getSuperblock(void)
{
	mapObject* chunk_start_map_obj = st_navigateTo(0, default_bytes);
	byte* chunk_start_pointer = chunk_start_map_obj->address_ptr;
	byte* superblock_pointer = findSuperblock(chunk_start_pointer);
	if(superblock_pointer == NULL)
	{
		readMXError(error_id, error_message);
	}
	Superblock s_block = fillSuperblock(superblock_pointer);
	st_releasePages(chunk_start_map_obj);
	return s_block;
}

//TODO change this to support entire searching entire file
byte* findSuperblock(byte* chunk_start)
{
	//Assuming that superblock is in first 8 512 byte chunks
	uint16_t chunk_address = 0;
	
	while(strncmp(FORMAT_SIG, (char*)chunk_start, 8) != 0 && chunk_address < default_bytes)
	{
		chunk_start += 512;
		chunk_address += 512;
	}
	
	if(chunk_address >= default_bytes)
	{
		sprintf(error_id, "getmatvar:superblockNotFoundError");
		sprintf(error_message, "Couldn't find superblock in first 8 512-byte chunks.\n\n");
		return NULL;
	}
	
	return chunk_start;
}


Superblock fillSuperblock(byte* superblock_pointer)
{
	Superblock s_block;
	//get stuff from superblock, for now assume consistent versions of stuff
	s_block.size_of_offsets = (uint8_t)getBytesAsNumber(superblock_pointer + 13, 1, META_DATA_BYTE_ORDER);
	s_block.size_of_lengths = (uint8_t)getBytesAsNumber(superblock_pointer + 14, 1, META_DATA_BYTE_ORDER);
	s_block.leaf_node_k = (uint16_t)getBytesAsNumber(superblock_pointer + 16, 2, META_DATA_BYTE_ORDER);
	s_block.internal_node_k = (uint16_t)getBytesAsNumber(superblock_pointer + 18, 2, META_DATA_BYTE_ORDER);
	s_block.base_address = getBytesAsNumber(superblock_pointer + 24, s_block.size_of_offsets, META_DATA_BYTE_ORDER);
	
	//read scratchpad space
	byte* sps_start = superblock_pointer + 80;
	s_block.root_tree_address = getBytesAsNumber(sps_start, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
	s_block.root_heap_address = getBytesAsNumber(sps_start + s_block.size_of_offsets, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
	
	s_block.sym_table_entry_size = (uint16_t)(2*s_block.size_of_offsets + 4 + 4 + 16);
	
	return s_block;
}

