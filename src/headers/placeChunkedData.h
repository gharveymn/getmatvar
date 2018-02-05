#ifndef PLACE_CHUNKED_DATA_H
#define PLACE_CHUNKED_DATA_H


#include "getDataObjects.h"
#include "mtezq.h"
#include "../extlib/libdeflate/libdeflate.h"


typedef enum
{
	GROUP = 0, CHUNK, NODETYPE_UNDEFINED, NODETYPE_ROOT, RAWDATA, SYMBOL
} NodeType;

//typedef enum
//{
//	LEAFTYPE_UNDEFINED, RAWDATA, SYMBOL
//} LeafType;

typedef struct
{
	uint32_t chunk_size;
	//uint32_t filter_mask;
	index_t* chunk_start;
	//address_t local_heap_offset;
} Key;

typedef struct tree_node_ TreeNode;
struct tree_node_
{
	address_t address;
	NodeType node_type;
	//LeafType leaf_type;
	//int16_t node_level;
	uint16_t entries_used;
	Key** keys;
	TreeNode** children;
};

typedef struct
{
	Data* object;
	MTQueue* mt_data_queue;
	error_t err;
#ifdef WIN32_LEAN_AND_MEAN
	HANDLE* thread_sync;
#else
	pthread_cond_t* thread_sync;
	pthread_mutex_t* thread_mtx;
#endif
	bool_t* main_thread_ready;
} InflateThreadObj;

typedef struct
{
	Key* data_key;
	TreeNode* data_node;
} DataPair;

error_t fillNode(TreeNode* node, uint8_t num_chunked_dims);
#ifdef WIN32_LEAN_AND_MEAN
DWORD doInflate_(void* t);
DWORD mt_fillNode(void* fno);
#else
void* doInflate_(void* t);
void* mt_fillNode(void* fno);
#endif
void freeTree(void* n);
error_t getChunkedData(Data* obj);
index_t findArrayPosition(const index_t* chunk_start, const index_t* array_dims, uint8_t num_chunked_dims);
void memdump(const char type[]);
void makeChunkedUpdates(index_t* chunk_update, const index_t* chunked_dims, const index_t* dims, uint8_t num_dims);

#endif //PLACE_CHUNKED_DATA_H
