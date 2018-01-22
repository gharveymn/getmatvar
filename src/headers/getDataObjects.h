#ifndef GET_DATA_OBJECTS_H
#define GET_DATA_OBJECTS_H

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


#if (defined(_WIN32) || defined(WIN32) || defined(_WIN64)) && !defined __CYGWIN__
//#pragma message ("getmatvar is compiling on WINDOWS")


#include "../extlib/mman-win32/mman.h"
#include <pthread.h>
//#include "extlib/pthreads-win32/include/pthread.h"
#else
//#pragma message ("getmatvar is compiling on UNIX")
#include <pthread.h>
#include <endian.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
typedef int errno_t;
typedef uint64_t OffsetType;
#endif

typedef enum {FALSE = (uint8_t)0, TRUE = (uint8_t)1 } bool_t;
#define FORMAT_SIG "\211HDF\r\n\032\n"
#define MATFILE_7_3_SIG "\x4D\x41\x54\x4C\x41\x42\x20\x37\x2E\x33\x20\x4D\x41\x54\x2D\x66\x69\x6C\x65"
#define MATFILE_SIG_LEN 19
#define RETURN_STRUCT_NAME "Q291cnRuZXkgQm9ubmVyIDop"
#define RETURN_STRUCT_NAME_LEN 24
#define SELECTION_SIG "R2VuZSBIYXJ2ZXkgOik"
#define SELECTION_SIG_LEN 19
#define UNDEF_ADDR 0xffffffffffffffff
#define CLASS_LENGTH 200
#define NAME_LENGTH 200
#define MAX_NUM_FILTERS 32 /*see spec IV.A.2.1*/
#define HDF5_MAX_DIMS 32 /*see the "Chunking in HDF5" in documentation*/
#define CHUNK_BUFFER_SIZE 1048576 /*1MB size of the buffer used in zlib inflate (who doesn't have 1MB to spare?)*/
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
typedef uint64_t address_t;

typedef struct
{
	uint8_t size_of_offsets;
	uint8_t size_of_lengths;
	uint16_t leaf_node_k;
	uint16_t internal_node_k;
	uint16_t sym_table_entry_size;
	address_t base_address;
	address_t root_tree_address;
	address_t root_heap_address;
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
	HDF5_FIXED_POINT = 0,
	HDF5_FLOATING_POINT,
	HDF5_TIME,
	HDF5_STRING,
	HDF5_BIT_FIELD,
	HDF5_OPAQUE,
	HDF5_COMPOUND,
	HDF5_REFERENCE,
	HDF5_ENUMERATED,
	HDF5_VARIABLE_LENGTH,
	HDF5_ARRAY,
	HDF5_UNKNOWN
} HDF5Datatype;

#ifdef NO_MEX
//from matrix.h

typedef enum
{
	mxUNKNOWN_CLASS = 0,
	mxCELL_CLASS,
	mxSTRUCT_CLASS,
	mxLOGICAL_CLASS,
	mxCHAR_CLASS,
	mxVOID_CLASS,
	mxDOUBLE_CLASS,
	mxSINGLE_CLASS,
	mxINT8_CLASS,
	mxUINT8_CLASS,
	mxINT16_CLASS,
	mxUINT16_CLASS,
	mxINT32_CLASS,
	mxUINT32_CLASS,
	mxINT64_CLASS,
	mxUINT64_CLASS,
	mxFUNCTION_CLASS,
	mxOPAQUE_CLASS,
	mxOBJECT_CLASS, /* keep the last real item in the list */
#if defined(_LP64) || defined(_WIN64)
	mxINDEX_CLASS = mxUINT64_CLASS,
#else
	mxINDEX_CLASS = mxUINT32_CLASS,
#endif
	/* TEMPORARY AND NASTY HACK UNTIL mxSPARSE_CLASS IS COMPLETELY ELIMINATED */
			mxSPARSE_CLASS = mxVOID_CLASS /* OBSOLETE! DO NOT USE */
} mxClassID;

#endif


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
	byte* data;
	uint64_t* sub_object_header_offsets;
} DataArrays;

typedef struct
{
	uint16_t long_name_length;
	char* long_name;
	uint16_t short_name_length;
	char* short_name;
} NameStruct;

typedef enum
{
	NO_OBJ_HINT = 0,
	FUNCTION_HINT,
	OBJECT_HINT,
	OPAQUE_HINT
} objectDecodingHint;

typedef struct
{
	bool_t MATLAB_sparse;
	bool_t MATLAB_empty;
	objectDecodingHint MATLAB_object_decode;
} MatlabAttributes;

typedef struct
{
	bool_t is_struct_array;
	bool_t is_filled;
	bool_t is_mx_used;
	//bool_t is_finalized;
	bool_t is_reference;
} DataFlags;

typedef struct data_ Data;
struct data_
{
	HDF5Datatype hdf5_internal_type;
	mxClassID matlab_internal_type;
	mxClassID matlab_sparse_type;
	MatlabAttributes matlab_internal_attributes;
	DataFlags data_flags;
	mxComplexity complexity_flag;
	uint32_t datatype_bit_field;
	ByteOrder byte_order;
	
	NameStruct names;
	
	char matlab_class[CLASS_LENGTH];
	ChunkedInfo chunked_info;
	
	uint32_t dims[HDF5_MAX_DIMS + 1];
	uint8_t num_dims;
	uint32_t num_elems;
	size_t elem_size;
	
	uint32_t s_c_array_index;
	
	uint8_t layout_class;
	address_t data_address;
	byte* data_pointer;
	DataArrays data_arrays;
	
	address_t parent_obj_address;
	address_t this_obj_address;
	
	Data* super_object;
	Queue* sub_objects;
	uint32_t num_sub_objs;
};

typedef struct
{
	pthread_mutex_t lock;
	bool_t is_mapped;
	address_t pg_start_a;
	address_t pg_end_a;
	address_t map_base;//if already mapped by windows, the base offset of this map
	address_t map_end; //if already mapped by windows, what offset this map extends to
	address_t max_map_end;
	byte* pg_start_p;
	uint8_t num_using;
	uint32_t total_num_mappings;
	uint32_t last_use_time_stamp;
} pageObject;

typedef struct
{
	int num_vars;
	char** full_variable_names;
	char* filename;
} ParamStruct;

typedef enum
{
	VT_UNKNOWN,
	VT_LOCAL_NAME,
	VT_LOCAL_INDEX,
	VT_LOCAL_COORDINATES
} VariableNameType;

typedef struct
{
	VariableNameType variable_name_type;
	uint64_t variable_local_index;
	uint32_t variable_local_coordinates[HDF5_MAX_DIMS];
	char* variable_local_name;
} VariableNameToken;

//mapping.c
void getDataObjects(const char* filename, char** variable_names, int num_names);

#ifdef DO_MEMDUP
void memdump(const char type[]);
#endif

ByteOrder __byte_order__;
size_t alloc_gran;
size_t file_size;
size_t num_pages;
bool_t error_flag;
bool_t is_done;
char error_id[ERROR_BUFFER_SIZE];
char error_message[ERROR_BUFFER_SIZE];
char warn_id[WARNING_BUFFER_SIZE];
char warn_message[WARNING_BUFFER_SIZE];

ParamStruct parameters;
Queue* top_level_objects;
Queue* varname_queue;
Queue* object_queue;
Queue* eval_objects;

int fd;
Superblock s_block;
uint64_t default_bytes;

Queue* inflate_thread_obj_queue;
int num_avail_threads;		//number of processors - 1
int num_threads_user_def;		//user specifies number of threads
bool_t will_multithread;
bool_t will_suppress_warnings;

bool_t threads_are_started; //only start the thread pool once
pthread_mutex_t thread_acquisition_lock;
pageObject* page_objects;

volatile uint32_t usage_iterator;
volatile double sum_usage_offset;

Data* virtual_super_object;

int max_depth;

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

#include "cleanup.h"
#include "createDataObjects.h"
#include "fillDataObjects.h"
#include "getSystemInfo.h"
#include "init.h"
#include "navigate.h"
#include "numberHelper.h"
#include "placeChunkedData.h"
#include "placeData.h"
#include "readMessage.h"
#include "superblock.h"
#include "utils.h"


#endif //GET_DATA_OBJECTS