#ifndef PLACE_CHUNKED_DATA_H
#define PLACE_CHUNKED_DATA_H


#include "getDataObjects.h"
#include "mtezq.h"

#if UINTPTR_MAX == 0xffffffff
#include "../extlib/libdeflate/x86/libdeflate.h"
#elif UINTPTR_MAX == 0xffffffffffffffff
#include "../extlib/libdeflate/x64/libdeflate.h"


#else
//you need at least 19th century hardware to run this
#endif

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

typedef struct
{
	Data* object;
	MTQueue* mt_data_queue;
	pthread_cond_t* pthread_sync;
	pthread_mutex_t* pthread_mtx;
	bool_t* main_thread_ready;
	errno_t err;
} InflateThreadObj;

typedef struct
{
	InflateThreadObj* inflate_thread_obj;
	pthread_attr_t attr_join;
	pthread_t* threads;
} ThreadStartupObj;

typedef struct
{
	Key data_key;
	TreeNode* data_node;
} DataPair;

errno_t fillNode(TreeNode* node, uint64_t num_chunked_dims);
errno_t decompressChunk(Data* object);
void* doInflate_(void* t);
void freeTree(void* n);
errno_t getChunkedData(Data* obj);
uint64_t findArrayPosition(const uint32_t* chunk_start, const uint32_t* array_dims, uint8_t num_chunked_dims);
void memdump(const char type[]);
void makeChunkedUpdates(uint32_t* chunk_update, const uint32_t* chunked_dims, const uint32_t* dims, uint8_t num_dims);
void* garbageCollection_(void* nothing);
void startThreads_(void* thread_startup_obj);

//pthread_t gc;
//pthread_attr_t attr;
bool_t is_working;

#endif //PLACE_CHUNKED_DATA_H
