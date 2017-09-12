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
#include "extlib/param.h"
#define __BYTE_ORDER    BYTE_ORDER
#else
#include <endian.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
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
#define CLASS_LENGTH 200
#define NAME_LENGTH 200
#define MAX_NUM_FILTERS 32 /*see spec IV.A.2.1*/
#define CHUNK_BUFFER_SIZE 1048576 /*1MB size of the buffer used in zlib inflate (who doesn't have 1MB to spare?)*/
#define MAX_VAR_NAMES 64
#define MAX_MALLOC_VARS 1000
#define NUM_MAPS 2
#define MAX_SUB_OBJECTS 30
#define USE_SUPER_OBJECT_CELL 1
#define USE_SUPER_OBJECT_ALL 2

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))


#define MATLAB_HELP_MESSAGE "Usage:\n \tgetmatvar(filename,variable)\n" \
					"\tgetmatvar(filename,variable1,...,variableN)\n\n" \
					"\tfilename\t\ta character vector of the name of the file with a .mat extension\n" \
					"\tvariable\t\ta character vector of the variable to extract from the file\n\n" \
					"Example:\n\ts = getmatvar('my_workspace.mat', 'my_struct')\n"

#define MATLAB_WARN_MESSAGE ""

typedef unsigned char byte;  /* ensure an unambiguous, readable 8 bits */

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
	byte* map_start;
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
	NULLTYPE = 1 << 0,
	UINT8 = 1 << 1,
	INT8 = 1 << 2,
	UINT16 = 1 << 3,
	INT16 = 1 << 4,
	UINT32 = 1 << 5,
	INT32 = 1 << 6,
	UINT64 = 1 << 7,
	INT64 = 1 << 8,
	SINGLE = 1 << 9,
	DOUBLE = 1 << 10,
	REF = 1 << 11,
	STRUCT = 1 << 12,
	FUNCTION_HANDLE = 1 << 13,
	TABLE = 1 << 14,
	DELIMITER = 1 << 15,
	END_SENTINEL = 1 << 16,
	ERROR = 1 << 17,
	UNDEF = 1 << 18
} DataType;

typedef enum
{
	NOT_AVAILABLE, DEFLATE, SHUFFLE, FLETCHER32, SZIP, NBIT, SCALEOFFSET
}FilterType;

#ifdef LITTLE_ENDIAN
#undef LITTLE_ENDIAN
#endif

#ifdef BIG_ENDIAN
#undef BIG_ENDIAN
#endif

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
	uint32_t chunk_size;
	uint32_t* chunked_dims;
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
	char* data_pointer;
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
Superblock getSuperblock(void);
byte* findSuperblock(void);
Superblock fillSuperblock(byte* superblock_pointer);
byte* navigateTo(uint64_t address, uint64_t bytes_needed, int map_index);
void readTreeNode(byte* tree_pointer, Addr_Trio this_trio);
void readSnod(byte* snod_pointer, byte* heap_pointer, Addr_Trio parent_trio, Addr_Trio this_address);
void freeDataObjects(Data** objects);
void freeDataObjectTree(Data* super_object);
void endHooks(void);
void freeMap(int map_index);


//numberHelper.c
double convertHexToDouble(uint64_t hex);
float convertHexToSingle(uint32_t hex);
int roundUp(int numToRound);
uint64_t getBytesAsNumber(byte* chunk_start, size_t num_bytes, ByteOrder endianness);
void indToSub(int index, const uint32_t* dims, uint32_t* indices);
void reverseBytes(byte* data_pointer, size_t num_elems);


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
void readDataSpaceMessage(Data* object, byte* msg_pointer, uint64_t msg_address, uint16_t msg_size);
void readDataTypeMessage(Data* object, byte* msg_pointer, uint64_t msg_address, uint16_t msg_size);
void readDataLayoutMessage(Data* object, byte* msg_pointer, uint64_t msg_address, uint16_t msg_size);
void readDataStoragePipelineMessage(Data* object, byte* msg_pointer, uint64_t msg_address, uint16_t msg_size);
void readAttributeMessage(Data* object, byte* msg_pointer, uint64_t msg_address, uint16_t msg_size);

//mapping.c
Data* findDataObject(const char* filename, const char variable_name[]);
Data** getDataObjects(const char* filename, const char* variable_names[], int num_names);
void findHeaderAddress(const char variable_name[]);
void collectMetaData(Data* object, uint64_t header_address, uint16_t num_msgs, uint32_t header_length);
Data* organizeObjects(Data** objects, int* starting_pos);
void placeInSuperObject(Data* super_object, Data** objects, int num_total_objs, int* index);
void allocateSpace(Data* object);
void placeData(Data* object, byte* data_pointer, uint64_t starting_index, uint64_t condition, size_t elem_size, ByteOrder data_byte_order);
void initializeObject(Data* object);
uint16_t interpretMessages(Data* object, uint64_t header_address, uint32_t header_length, uint16_t message_num, uint16_t num_msgs, uint16_t repeat_tracker);
void parseHeaderTree(void);
//void deepCopy(Data* dest, Data* source);

//getPageSize.c
size_t getPageSize(void);
size_t getAllocGran(void);

//chunkedData.c
void fillChunkTree(TreeNode* root, uint64_t num_chunked_dims);
void fillNode(TreeNode* node, uint64_t num_chunked_dims);
void decompressChunk(Data* object, TreeNode* node);
void doInflate(Data* object, TreeNode* node);
void freeTree(TreeNode* node);
void getChunkedData(Data* object);
uint64_t findArrayPosition(const uint64_t* chunk_start, const uint32_t* array_dims, uint8_t num_chunked_dims);

MemMap maps[NUM_MAPS];
Addr_Q queue;
Header_Q header_queue;
Variable_Name_Q variable_name_queue;

int fd;
size_t file_size;
Superblock s_block;
uint64_t default_bytes;
int variable_found;
Addr_Trio root_trio;
