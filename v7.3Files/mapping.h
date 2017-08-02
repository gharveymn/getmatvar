#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>

#define TRUE 1
#define FALSE 0
#define FORMAT_SIG "\211HDF\r\n\032\n"
#define MAX_Q_LENGTH 20
#define TREE 0
#define HEAP 1
#define UNDEF_ADDR 0xffffffffffffffff
#define SYM_TABLE_ENTRY_SIZE 40
#define MAX_OBJS 100

typedef struct
{
	uint64_t tree_address;
	uint64_t heap_address;
} Addr_Pair;

typedef struct
{
	Addr_Pair pairs[MAX_Q_LENGTH];
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
} Superblock;

typedef struct 
{
	char* map_start;
	uint64_t bytes_mapped;
	uint64_t offset;
	int used;
} MemMap;

typedef struct
{
	uint64_t name_offset;
	uint64_t obj_header_address;
} Object;

typedef struct
{
	uint64_t header_addresses[MAX_Q_LENGTH];
	int front;
	int back;
	int length;
} Header_Q;

typedef enum
{
	CHAR,
	DOUBLE,
	UINT16_T,
	REF, 
	UNDEF
} Datatype;

typedef struct
{
	Datatype type;
	char matlab_class[20];
	uint32_t* dims;
	char* char_data;
	double* double_data;
	uint64_t* udouble_data;
	uint16_t* ushort_data;
} Data;

Superblock getSuperblock(int fd, size_t file_size);
char* findSuperblock(int fd, size_t file_size);
Superblock fillSuperblock(char* superblock_pointer);
char* navigateTo(uint64_t address, uint64_t bytes_needed, int map_index);
void readTreeNode(char* tree_address);
uint64_t readSnod(char* snod_pointer, char* heap_pointer, char* var_name);
uint32_t* readDataSpaceMessage(char* msg_pointer, uint16_t msg_size);
Datatype readDataTypeMessage(char* msg_pointer, uint16_t msg_size);
void freeDataObjects(Data* objects, int num);

double convertHexToFloatingPoint(uint64_t hex);
int roundUp(int numToRound);
uint64_t getBytesAsNumber(char* chunk_start, int num_bytes);

void enqueuePair(Addr_Pair pair);
void flushQueue();
Addr_Pair dequeuePair();
void priorityEnqueuePair(Addr_Pair pair);
void flushHeaderQueue();
uint64_t dequeueAddress();
void priorityEnqueueAddress(uint64_t address);
void enqueueAddress(uint64_t address);

Data* getDataObject(char* filename, char variable_name[]);
void findHeaderAddress(char* filename, char variable_name[]);

MemMap maps[2];
Addr_Q queue;
Header_Q header_queue;

int fd;
Superblock s_block;
int is_string;
uint64_t default_bytes;