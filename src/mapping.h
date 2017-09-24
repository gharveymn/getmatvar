#ifndef MAPPING_H
#define MAPPING_H

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

//#define DO_MEMDUMP
//#define NO_MEX		pass this through gcc -DNO_MEX=TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>


#ifndef NO_MEX

#include <mex.h>


#endif

#include <stdarg.h>
#include "ezq.h"
#include "extlib/thpool/thpool.h"


#if (defined(_WIN32) || defined(WIN32) || defined(_WIN64)) && !defined __CYGWIN__
#pragma message ("getmatvar is compiling on WINDOWS")

#include "extlib/mman-win32/mman.h"
#include "extlib/param.h"
#include <pthread.h>
//#include "extlib/pthreads-win32/include/pthread.h"
#else
#pragma message ("getmatvar is compiling on UNIX")
#include <pthread.h>
#include <endian.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
typedef int errno_t;
typedef uint64_t OffsetType;
#endif

#define TRUE 1
#define FALSE 0
#define FORMAT_SIG "\211HDF\r\n\032\n"
#define MATFILE_7_3_SIG "\x4D\x41\x54\x4C\x41\x42\x20\x37\x2E\x33\x20\x4D\x41\x54\x2D\x66\x69\x6C\x65"
#define MATFILE_SIG_LEN 19
#define FUNCTION_HANDLE_SIG "R2VuZSBIYXJ2ZXkgOik="
#define FUNCTION_HANDLE_SIG_LEN 20
#define TREE 0
#define HEAP 1
#define UNDEF_ADDR 0xffffffffffffffff
#define CLASS_LENGTH 200
#define NAME_LENGTH 200
#define MAX_NUM_FILTERS 32 /*see spec IV.A.2.1*/
#define HDF5_MAX_DIMS 32 /*see the "Chunking in HDF5" in documentation*/
#define CHUNK_BUFFER_SIZE 1048576 /*1MB size of the buffer used in zlib inflate (who doesn't have 1MB to spare?)*/
#define NUM_TREE_MAPS 4
#define NUM_HEAP_MAPS 2
#define ERROR_BUFFER_SIZE 5000
#define WARNING_BUFFER_SIZE 1000

#define MIN(X, Y) (((X) < (Y))? (X) : (Y))
#define MAX(X, Y) (((X) > (Y))? (X) : (Y))

#define MATLAB_HELP_MESSAGE "Usage:\n \tgetmatvar(filename,variable)\n" \
                         "\tgetmatvar(filename,variable1,...,variableN)\n\n" \
                         "\tfilename\t\ta character vector of the name of the file with a .mat extension\n" \
                         "\tvariable\t\ta character vector of the variable to extract from the file\n\n" \
                         "Example:\n\tgetmatvar('my_workspace.mat', 'my_struct')\n"

#define MATLAB_WARN_MESSAGE ""

#ifdef NO_MEX
#define mxMalloc malloc
#define mxFree free
#define mxComplexity int
#define mxREAL 0
#define mxCOMPLEX 1
#endif

//compiler hints

/* likely(expr) - hint that an expression is usually true */
#ifndef likely
#  define likely(expr)          (expr)
#endif

/* unlikely(expr) - hint that an expression is usually false */
#ifndef unlikely
#  define unlikely(expr)     (expr)
#endif

//typedefs

typedef unsigned char byte;  /* ensure an unambiguous, readable 8 bits */
typedef uint8_t bool_t;

typedef struct
{
	uint64_t parent_obj_header_address;
	uint64_t tree_address;
	uint64_t heap_address;
} AddrTrio;

typedef struct
{
	//this is an ENTRY for a SNOD
	
	uint64_t name_offset;
	char name[NAME_LENGTH];
	
	uint64_t parent_obj_header_address;
	uint64_t this_obj_header_address;
	
	uint64_t parent_tree_address;
	uint64_t this_tree_address;
	uint64_t sub_tree_address; //invoked if cache type 1
	
} SNODEntry;

typedef struct
{
	uint8_t size_of_offsets;
	uint8_t size_of_lengths;
	uint16_t leaf_node_k;
	uint16_t internal_node_k;
	uint64_t base_address;
	uint64_t root_tree_address;
} Superblock;

typedef struct
{
	byte* map_start;
	uint64_t bytes_mapped;
	OffsetType offset;
	int used;
} MemMap;

typedef enum
{
	NULLTYPE_DATA = 1 << 0,
	UINT8_DATA = 1 << 1,
	INT8_DATA = 1 << 2,
	UINT16_DATA = 1 << 3,
	INT16_DATA = 1 << 4,
	UINT32_DATA = 1 << 5,
	INT32_DATA = 1 << 6,
	UINT64_DATA = 1 << 7,
	INT64_DATA = 1 << 8,
	SINGLE_DATA = 1 << 9,
	DOUBLE_DATA = 1 << 10,
	REF_DATA = 1 << 11,
	STRUCT_DATA = 1 << 12,
	FUNCTION_HANDLE_DATA = 1 << 13,
	TABLE_DATA = 1 << 14,
	DELIMITER = 1 << 15,
	END_SENTINEL = 1 << 16,
	ERROR_DATA = 1 << 17,
	UNDEF_DATA = 1 << 18
} DataType;

typedef enum
{
	NOT_AVAILABLE, DEFLATE, SHUFFLE, FLETCHER32, SZIP, NBIT, SCALEOFFSET
} FilterType;

#ifdef LITTLE_ENDIAN
#undef LITTLE_ENDIAN
#endif

#ifdef BIG_ENDIAN
#undef BIG_ENDIAN
#endif

typedef enum
{
	LITTLE_ENDIAN, BIG_ENDIAN
} ByteOrder;
#define META_DATA_BYTE_ORDER LITTLE_ENDIAN

typedef struct
{
	FilterType filter_id;
	uint16_t num_client_vals;
	uint32_t* client_data;
	uint8_t optional_flag;
} Filter;

typedef struct
{
	uint8_t num_filters;
	Filter filters[MAX_NUM_FILTERS];
	uint8_t num_chunked_dims;
	uint32_t num_chunked_elems;
	uint32_t chunked_dims[HDF5_MAX_DIMS + 1];
	uint64_t chunk_update[HDF5_MAX_DIMS];
} ChunkedInfo;

typedef struct
{
	int is_mx_used;
	uint8_t* ui8_data; //note that ui8 stores logicals and ui8s
	int8_t* i8_data;
	uint16_t* ui16_data; //note that ui16 stores strings, and ui16s
	int16_t* i16_data;
	uint32_t* ui32_data;
	int32_t* i32_data;
	uint64_t* ui64_data;
	int64_t* i64_data;
	float* single_data;
	double* double_data;
	uint64_t* sub_object_header_offsets;
} DataArrays;

typedef struct data_ Data;
struct data_
{
	DataType type;
	mxComplexity complexity_flag;
	uint32_t datatype_bit_field;
	ByteOrder byte_order;
	char name[NAME_LENGTH];
	char matlab_class[CLASS_LENGTH];
	ChunkedInfo chunked_info;
	
	uint32_t dims[HDF5_MAX_DIMS + 1];
	uint8_t num_dims;
	uint32_t num_elems;
	size_t elem_size;
	
	uint8_t layout_class;
	uint64_t data_address;
	byte* data_pointer;
	DataArrays data_arrays;
	
	uint64_t parent_obj_address;
	uint64_t this_obj_address;
	
	Data** sub_objects;
	uint32_t num_sub_objs;
};

typedef enum
{
	GROUP = (uint8_t)0, CHUNK = (uint8_t)1, NODETYPE_UNDEFINED
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
	uint64_t address;
	NodeType node_type;
	LeafType leaf_type;
	int16_t node_level;
	uint16_t entries_used;
	Key* keys;
	TreeNode* children;
	TreeNode* left_sibling;
	TreeNode* right_sibling;
};

typedef struct
{
	pthread_mutex_t lock;
	bool_t is_mapped;
	uint64_t pg_start_a;
	uint64_t pg_end_a;
	uint64_t map_base;//if already mapped by windows, the base offset of this map
	uint64_t map_end; //if already mapped by windows, what offset this map extends to
	byte* pg_start_p;
	uint8_t num_using;
	uint32_t last_use_time_stamp;
} pageObject;

//fileHelper.c
Superblock getSuperblock(void);
byte* findSuperblock(void);
Superblock fillSuperblock(byte* superblock_pointer);
byte* navigateTo(uint64_t address, uint64_t bytes_needed, int map_type);
byte* navigatePolitely(uint64_t address, uint64_t bytes_needed);
void releasePages(uint64_t address, uint64_t bytes_needed);
byte* navigateWithMapIndex(uint64_t address, uint64_t bytes_needed, int map_type, int map_index);
void readTreeNode(byte* tree_pointer, AddrTrio* this_trio);
void readSnod(byte* snod_pointer, byte* heap_pointer, AddrTrio* parent_trio, AddrTrio* this_address, bool_t get_top_level);
void freeDataObject(void* object);
void freeDataObjectTree(Data* super_object);
void endHooks(void);
void freeAllMaps(void);
void freeMap(MemMap map);
void destroyPageObjects(void);

//numberHelper.c
int roundUp(int numToRound);
uint64_t getBytesAsNumber(byte* chunk_start, size_t num_bytes, ByteOrder endianness);
void reverseBytes(byte* data_pointer, size_t num_elems);

//readMessage.c
void readDataSpaceMessage(Data* object, byte* msg_pointer, uint64_t msg_address, uint16_t msg_size);
void readDataTypeMessage(Data* object, byte* msg_pointer, uint64_t msg_address, uint16_t msg_size);
void readDataLayoutMessage(Data* object, byte* msg_pointer, uint64_t msg_address, uint16_t msg_size);
void readDataStoragePipelineMessage(Data* object, byte* msg_pointer, uint64_t msg_address, uint16_t msg_size);
void readAttributeMessage(Data* object, byte* msg_pointer, uint64_t msg_address, uint16_t msg_size);

//mapping.c
Queue* getDataObjects(const char* filename, char** variable_names, int num_names);
void findHeaderAddress(char* variable_name, bool_t get_top_level);
void collectMetaData(Data* object, uint64_t header_address, uint16_t num_msgs, uint32_t header_length);
Data* organizeObjects(Queue* objects);
void placeInSuperObject(Data* super_object, Queue* objects, int num_total_objs);
errno_t allocateSpace(Data* object);
void placeData(Data* object, byte* data_pointer, uint64_t starting_index, uint64_t condition, size_t elem_size, ByteOrder data_byte_order);
void initializeMaps(void);
void placeDataWithIndexMap(Data* object, byte* data_pointer, uint64_t num_elems, size_t elem_size, ByteOrder data_byte_order, const uint64_t* index_map);
void initializeObject(Data* object);
uint16_t interpretMessages(Data* object, uint64_t header_address, uint32_t header_length, uint16_t message_num, uint16_t num_msgs, uint16_t repeat_tracker);
void parseHeaderTree(bool_t get_top_level);
void initializePageObjects(void);
void freeVarname(void* vn);


//getPageSize.c
size_t getPageSize(void);
size_t getAllocGran(void);
int getNumProcessors(void);
ByteOrder getByteOrder(void);


//chunkedData.c
errno_t fillNode(TreeNode* node, uint64_t num_chunked_dims);
errno_t decompressChunk(TreeNode* node);
void* doInflate_(void* t);
void freeTree(TreeNode* node);
errno_t getChunkedData(Data* obj);
uint64_t findArrayPosition(const uint32_t* chunk_start, const uint32_t* array_dims, uint8_t num_chunked_dims);
void memdump(const char type[]);
void makeChunkedUpdates(uint64_t chunk_update[32], const uint32_t chunked_dims[32], const uint32_t dims[32], uint8_t num_dims);

void readMXError(const char error_id[], const char error_message[], ...);
void readMXWarn(const char warn_id[], const char warn_message[], ...);

#ifdef DO_MEMDUP
void memdump(const char type[]);
#endif

ByteOrder __byte_order__;
size_t alloc_gran;
size_t file_size;
size_t num_pages;

MemMap tree_maps[NUM_TREE_MAPS];
MemMap heap_maps[NUM_HEAP_MAPS];
int map_nums[2];
int map_queue_fronts[2]; //cycle thru a queue so that we dont overwrite too soon

Queue* addr_queue;
Queue* varname_queue;
Queue* header_queue;

int fd;
Superblock s_block;
uint64_t default_bytes;
int variable_found;
AddrTrio root_trio;


threadpool threads;
int num_avail_threads;
int num_threads_to_use;
bool_t will_multithread;
bool_t will_suppress_warnings;

bool_t threads_are_started; //only start the thread pool once
pthread_mutex_t thread_acquisition_lock;
pageObject* page_objects;

uint32_t usage_iterator;
double sum_usage_offset;

#ifdef DO_MEMDUMP
FILE* dump;
pthread_cond_t dump_ready;
pthread_mutex_t dump_lock;
#endif

#ifdef NO_MEX
pthread_mutex_t if_lock;
#define pthread_spin_lock pthread_mutex_lock
#define pthread_spin_unlock pthread_mutex_unlock
#define pthread_spin_init pthread_mutex_init
#define pthread_spin_destroy pthread_mutex_destroy
#undef PTHREAD_PROCESS_SHARED
#define PTHREAD_PROCESS_SHARED NULL
#else
pthread_spinlock_t if_lock;
#endif

#ifndef NO_MEX

#include "getmatvar_.h"


#endif

#endif