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
#define MAX_Q_LENGTH 20
#define TREE 0
#define HEAP 1
#define UNDEF_ADDR 0xffffffffffffffff
#define SYM_TABLE_ENTRY_SIZE 40
#define MAX_OBJS 100
#define CLASS_LENGTH 20
#define NAME_LENGTH 30
#define MAX_NUM_FILTERS 32; //see spec IV.A.2.1
#define MAX_SUB_OBJECTS 30;
#define USE_SUPER_OBJECT_CELL 1;
#define USE_SUPER_OBJECT_ALL 2;

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
	uint8_t size_of_offsets;
	uint8_t size_of_lengths;
	uint16_t leaf_node_k;
	uint16_t internal_node_k;
	uint64_t base_address;
	uint64_t root_tree_address;
} Superblock;

typedef struct
{
	char *map_start;
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
	UNDEF,
	CHAR,
	DOUBLE,
	UNSIGNEDINT16,
	REF,
	STRUCT
} Datatype;

typedef struct data_ Data;
struct data_
{
	Datatype type;
	char matlab_class[CLASS_LENGTH];
	uint32_t *dims;
	char *char_data;
	double *double_data;
	uint64_t *udouble_data;
	uint16_t *ushort_data;
	char name[NAME_LENGTH];
	uint64_t parent_obj_address;
	uint64_t this_obj_address;
	Data *sub_objects;
};

typedef enum
{
	NA,
	DEFLATE,
	SHUFFLE,
	FLETCHER32,
	SZIP,
	NBIT,
	SCALEOFFSET,
} FilterID;

typedef struct
{
	FilterID filter_id;
	uint16_t num_client_vals;
	uint32_t* client_data;
	uint8_t optional_flag;
} Filter;


//fileHelper.c
Superblock getSuperblock(int fd, size_t file_size);
char* findSuperblock(int fd, size_t file_size);
Superblock fillSuperblock(char *superblock_pointer);
char* navigateTo(uint64_t address, uint64_t bytes_needed, int map_index);
char* navigateTo_map(MemMap map, uint64_t address, uint64_t bytes_needed, int map_index);
void readTreeNode(char *tree_address, Addr_Trio this_trio);
void readSnod(char *snod_pointer, char *heap_pointer, char *var_name, Addr_Trio parent_trio, Addr_Trio this_address);
uint32_t *readDataSpaceMessage(char *msg_pointer, uint16_t msg_size);
Datatype readDataTypeMessage(char *msg_pointer, uint16_t msg_size);
void freeDataObjects(Data *objects, int num);


//numberHelper.c
double convertHexToFloatingPoint(uint64_t hex);
int roundUp(int numToRound);
uint64_t getBytesAsNumber(char *chunk_start, int num_bytes);
void indToSub(int index, uint32_t *dims, uint32_t *indices);


//queue.c
void enqueueTrio(Addr_Trio trio);
void flushQueue();
Addr_Trio dequeueTrio();
void priorityEnqueueTrio(Addr_Trio trio);
void flushHeaderQueue();
Object dequeueObject();
void priorityEnqueueObject(Object obj);
void enqueueObject(Object obj);


//mapping.c
Data* getDataObject(char *filename, char variable_name[], int *num_objs);
void findHeaderAddress(char *filename, char variable_name[]);
void collectMetaData(Data *object, uint64_t header_address, char *header_pointer);
Data* organizeObjects(Data *objects, int num_objs);
void placeInSuperObject(Data* super_object, Data* objects, int num_objs, int* index);
//void deepCopy(Data* dest, Data* source);

//getPageSize.c
int getPageSize(void);
int getAllocGran(void);


MemMap maps[2];
Addr_Q queue;
Header_Q header_queue;

int fd;
Superblock s_block;
int is_string;
uint64_t default_bytes;
int variable_found;