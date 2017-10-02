#ifndef PLACE_CHUNKED_DATA_H
#define PLACE_CHUNKED_DATA_H


#include "getDataObjects.h"

typedef enum
{
	GROUP = (uint8_t) 0, CHUNK = (uint8_t) 1, NODETYPE_UNDEFINED
} NodeType;

typedef enum
{
	LEAFTYPE_UNDEFINED, RAWDATA, SYMBOL
} LeafType;

typedef struct
{
	uint32_t size;
	uint32_t filter_mask;
	uint32_t chunk_start[HDF5_MAX_DIMS + 1];
	uint64_t local_heap_offset;
} Key;

typedef struct tree_node_ TreeNode;
struct tree_node_
{
	address_t address;
	NodeType node_type;
	LeafType leaf_type;
	int16_t node_level;
	uint16_t entries_used;
	Key* keys;
	TreeNode** children;
};

typedef struct inflate_thread_obj_ inflate_thread_obj;
struct inflate_thread_obj_
{
	TreeNode* node;
	errno_t err;
};

errno_t fillNode(TreeNode* node, uint64_t num_chunked_dims);
errno_t decompressChunk(TreeNode* node);
void* doInflate_(void* t);
void freeTree(void* n);
errno_t getChunkedData(Data* obj);
uint64_t findArrayPosition(const uint32_t* chunk_start, const uint32_t* array_dims, uint8_t num_chunked_dims);
void memdump(const char type[]);
void makeChunkedUpdates(uint64_t chunk_update[32], const uint32_t chunked_dims[32], const uint32_t dims[32], uint8_t num_dims);

Queue* chunkTreeRoots;

#endif //PLACE_CHUNKED_DATA_H
