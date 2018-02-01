#ifndef PLACE_CHUNKED_DATA_H
#define PLACE_CHUNKED_DATA_H


#include "getDataObjects.h"
#include "mtezq.h"

#if defined(GMV_32_BIT)
#include "../extlib/libdeflate/x86/libdeflate.h"
#elif defined(GMV_64_BIT)
#include "../extlib/libdeflate/x64/libdeflate.h"
#else
//you need at least 19th century hardware to run this
#endif

typedef enum
{
	GROUP = 0, CHUNK, NODETYPE_UNDEFINED, NODETYPE_ROOT
} NodeType;

typedef enum
{
	LEAFTYPE_UNDEFINED, RAWDATA, SYMBOL
} LeafType;

typedef struct
{
	uint32_t size;
	uint32_t filter_mask;
	uint64_t* chunk_start;
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
	Key** keys;
	TreeNode** children;
};

typedef struct
{
	Data* object;
	MTQueue* mt_data_queue;
#ifdef WIN32_LEAN_AND_MEAN
	CONDITION_VARIABLE* thread_sync;
	CRITICAL_SECTION* thread_mtx;
#else
	pthread_cond_t* thread_sync;
	pthread_mutex_t* thread_mtx;
#endif
	bool_t* main_thread_ready;
	errno_t err;
} InflateThreadObj;

typedef struct
{
	Key* data_key;
	TreeNode* data_node;
} DataPair;

typedef struct
{
	TreeNode* node;
	uint8_t num_chunked_dims;
	errno_t err;
} FillNodeObj;

errno_t fillNode(TreeNode* node, uint8_t num_chunked_dims);
errno_t decompressChunk(Data* object);
#ifdef WIN32_LEAN_AND_MEAN
DWORD doInflate_(void* t);
DWORD mt_fillNode(void* fno);
#else
void* doInflate_(void* t);
void* mt_fillNode(void* fno);
#endif
void freeTree(void* n);
errno_t getChunkedData(Data* obj);
uint64_t findArrayPosition(const uint64_t* chunk_start, const uint64_t* array_dims, uint8_t num_chunked_dims);
void memdump(const char type[]);
void makeChunkedUpdates(uint64_t* chunk_update, const uint64_t* chunked_dims, const uint64_t* dims, uint8_t num_dims);
void* garbageCollection_(void* nothing);
void startThreads_(void* thread_startup_obj);

//pthread_t gc;
//pthread_attr_t attr;
bool_t is_working;

#endif //PLACE_CHUNKED_DATA_H
