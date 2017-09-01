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

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64)
#include "extlib/mman-win32/mman.h"
#else
#include <sys/mman.h>
typedef uint64_t OffsetType;
#endif

#define TRUE 1
#define FALSE 0
#define FORMAT_SIG "\211HDF\r\n\032\n"
#define MAX_Q_LENGTH 1000
#define TREE 0
#define HEAP 1
#define UNDEF_ADDR 0xffffffffffffffff
#define MAX_OBJS 1000
#define CLASS_LENGTH 20
#define NAME_LENGTH 30
#define MAX_NUM_FILTERS 32 /*see spec IV.A.2.1*/
#define CHUNK_BUFFER_SIZE 1048576 /*1MB size of the buffer used in zlib inflate (who doesn't have 1MB to spare?)*/
#define MAX_SUB_OBJECTS 30
#define USE_SUPER_OBJECT_CELL 1
#define USE_SUPER_OBJECT_ALL 2

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

typedef struct
{
	uint64_t parent_obj_header_address;
	uint64_t tree_address;
	uint64_t heap_address;
} Addr_Trio;

typedef struct
{
	Addr_Trio trios[MAX_Q_LENGTH];
	int front;
	int back;
	int length;
} Addr_Q;

typedef struct
{
	char* variable_names[MAX_Q_LENGTH];
	int front;
	int back;
	int length;
} Variable_Name_Q;

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
	char* map_start;
	uint64_t bytes_mapped;
	OffsetType offset;
	int used;
} MemMap;

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
	
} Object;

typedef struct
{
	Object objects[MAX_Q_LENGTH];
	int front;
	int back;
	int length;
} Header_Q;

typedef enum
{
	UNDEF, CHAR, UINT8, INT8, UINT16, INT16, UINT32, INT32, UINT64, INT64, SINGLE, DOUBLE, REF, STRUCT, FUNCTION_HANDLE
} DataType;

typedef enum
{
	NOT_AVAILABLE, DEFLATE, SHUFFLE, FLETCHER32, SZIP, NBIT, SCALEOFFSET
}FilterType;

typedef enum
{
	LITTLE_ENDIAN,
	BIG_ENDIAN
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
	uint32_t* chunked_dims;
} ChunkedInfo;

typedef struct
{
	char* char_data;
	int8_t i8_data;
	uint16_t* ui16_data;
	int16_t* i16_data;
	uint32_t* ui32_data;
	int32_t* i32_data;
	uint64_t* ui64_data;
	int64_t* i64_data;
	double* single_data;
	double* double_data;
	uint64_t* udouble_data;
} DataArrays;

typedef struct data_ Data;
struct data_
{
	DataType type;
	uint32_t datatype_bit_field;
	ByteOrder byte_order;
	char name[NAME_LENGTH];
	char matlab_class[CLASS_LENGTH];
	ChunkedInfo chunked_info;
	
	uint32_t* dims;
	uint8_t num_dims;
	uint32_t num_elems;
	size_t elem_size;
	
	uint8_t layout_class;
	uint64_t data_address;
	DataArrays data_arrays;
	
	uint64_t parent_obj_address;
	uint64_t this_obj_address;
	
	Data** sub_objects;
	uint32_t num_sub_objs;
};

typedef enum
{
	NODETYPE_UNDEFINED, GROUP = (uint8_t) 0, CHUNK = (uint8_t) 1
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


//fileHelper.c
Superblock getSuperblock(int fd, size_t file_size);
char* findSuperblock(int fd, size_t file_size);
Superblock fillSuperblock(char* superblock_pointer);
char* navigateTo(uint64_t address, uint64_t bytes_needed, int map_index);
char* navigateTo_map(MemMap map, uint64_t address, uint64_t bytes_needed, int map_index);
void readTreeNode(char* tree_address);
void readSnod(char* snod_pointer, char* heap_pointer, Addr_Trio parent_trio, Addr_Trio this_address);
void freeDataObjects(Data** objects);
void freeDataObjectTree(Data* super_object);


//numberHelper.c
double convertHexToDouble(uint64_t hex);
int roundUp(int numToRound);
uint64_t getBytesAsNumber(char* chunk_start, size_t num_bytes, ByteOrder endianness);
void indToSub(int index, const uint32_t* dims, uint32_t* indices);


//queue.c
void enqueueTrio(Addr_Trio trio);
void flushQueue();
Addr_Trio dequeueTrio();
void priorityEnqueueTrio(Addr_Trio trio);
void flushHeaderQueue();
Object dequeueObject();
void priorityEnqueueObject(Object obj);
void enqueueObject(Object obj);
void flushVariableNameQueue();
char* dequeueVariableName();
char* peekVariableName();
void enqueueVariableName(char* variable_name);

//readMessage.c
void readDataSpaceMessage(Data* object, char* msg_pointer, uint64_t msg_address, uint16_t msg_size);
void readDataTypeMessage(Data* object, char* msg_pointer, uint64_t msg_address, uint16_t msg_size);
char* readDataLayoutMessage(Data* object, char* msg_pointer, uint64_t msg_address, uint16_t msg_size);
void readDataStoragePipelineMessage(Data* object, char* msg_pointer, uint64_t msg_address, uint16_t msg_size);
void readAttributeMessage(Data* object, char* msg_pointer, uint64_t msg_address, uint16_t msg_size);


//mapping.c
Data* findDataObject(const char* filename, const char variable_name[]);
Data** getDataObjects(const char* filename, const char variable_name[]);
void findHeaderAddress(const char variable_name[]);
void collectMetaData(Data* object, uint64_t header_address, char* header_pointer);
Data* organizeObjects(Data** objects);
void placeInSuperObject(Data* super_object, Data** objects, int num_total_objs, int* index);
void allocateSpace(Data* object);
void placeData(Data* object, char* data_pointer, uint64_t starting_index, uint64_t condition, size_t elem_size, ByteOrder byte_order);
//void deepCopy(Data* dest, Data* source);

//getPageSize.c
int getPageSize(void);
int getAllocGran(void);

//chunkedData.c
void fillChunkTree(TreeNode* root, uint64_t num_chunked_dims);
void fillNode(TreeNode* node, uint64_t num_chunked_dims);
void decompressChunk(Data* object, TreeNode* node);
void doInflate(Data* object, TreeNode* node);
void freeTree(TreeNode* node);
void getChunkedData(Data* object);
uint64_t findArrayPosition(const uint64_t* chunk_start, const uint32_t* array_dims, uint8_t num_chunked_dims);

MemMap maps[2];
Addr_Q queue;
Header_Q header_queue;
Variable_Name_Q variable_name_queue;

int fd;
Superblock s_block;
int is_string;
uint64_t default_bytes;
int variable_found;