#ifndef GET_DATA_OBJECTS_H
#define GET_DATA_OBJECTS_H

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

//#define DO_MEMDUMP
//#define NO_MEX		pass this through gcc -DNO_MEX=TRUE

#include <stdint.h>


#if UINTPTR_MAX == 0xffffffff
#define GMV_32_BIT
#define _FILE_OFFSET_BITS 32
#elif UINTPTR_MAX == 0xffffffffffffffff
#define GMV_64_BIT
#define _FILE_OFFSET_BITS 64
#else
#error architecture is weird
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <math.h>
#include <assert.h>
#include <errno.h>


#ifndef NO_MEX

#include <mex.h>


#endif

#include <stdarg.h>
#include "ezq.h"


#if (defined(_WIN32) || defined(WIN32) || defined(_WIN64)) && !defined __CYGWIN__
//#pragma message ("getmatvar is compiling on WINDOWS")
#define WIN32_LEAN_AND_MEAN
#ifdef WINVER
#undef WINVER
#define WINVER 0x0A00
#endif

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#include <io.h>
#include "../extlib/mman-win32/mman.h"
#include <windows.h>


#ifdef GMV_64_BIT
#ifdef lseek
#undef lseek
#endif
#define lseek _lseeki64

#ifdef off_t
#undef off_t
#endif
#define off_t int64_t
#endif

#else
//#pragma message ("getmatvar is compiling on UNIX")
#include <pthread.h>
#include <endian.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
typedef int errno_t;

#ifdef GMV_64_BIT
typedef uint64_t OffsetType;
#else
typedef uint32 OffsetType;
#endif

#endif

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

typedef enum
{
	FALSE = 0, TRUE = 1
} bool_t;
#define FORMAT_SIG "\211HDF\r\n\032\n"
#define MATFILE_7_3_SIG "\x4D\x41\x54\x4C\x41\x42\x20\x37\x2E\x33\x20\x4D\x41\x54\x2D\x66\x69\x6C\x65"
#define MATFILE_SIG_LEN 19
#define RETURN_STRUCT_NAME "Q291cnRuZXkgQm9ubmVyIDop"
#define RETURN_STRUCT_NAME_LEN 24
#define SELECTION_SIG "R2VuZSBIYXJ2ZXkgOik"
#define SELECTION_SIG_LEN 19
#define UNDEF_ADDR 0xffffffffffffffff
#define NAME_LENGTH 200
#define MAX_NUM_FILTERS 32 /*see spec IV.A.2.1*/
#define HDF5_MAX_DIMS 32 /*see the "Chunking in HDF5" in documentation*/
#define ERROR_BUFFER_SIZE 500
#define WARNING_BUFFER_SIZE 1000
#define ERROR_MESSAGE_SIZE 5000
#define WARNING_MESSAGE_SIZE 1000
#define DEFAULT_MAX_NUM_MAP_OBJS 10
#define MIN_MT_ELEMS_THRESH 100000
#define ONE_MB 1024000

#define MIN(X, Y) (((X) < (Y))? (X) : (Y))
#define MAX(X, Y) (((X) > (Y))? (X) : (Y))

#define MATLAB_HELP_MESSAGE "Usage:\n " \
                         "\tgetmatvar(filename)\n" \
                         "\tvar = getmatvar(filename,'var')\n" \
                         "\t[var1,...,varN] = getmatvar(filename,'var1',...,'varN')\n\n" \
                         "\tgetmatvar(__,'-t',n)\n" \
                         "\tgetmatvar(__,'-st')\n" \
                         "\tgetmatvar(__,'-sw')\n\n" \
                         "\tfilename\ta character vector of the location of a 7.3+ MAT-file\n" \
                         "\tvar\t\ta character vector of the variable to extract from the file\n" \
                         "\t'-t'\t\tspecify the number of threads to use in the next argument n\n" \
                         "\t'-st'\t\trestrict getmatvar to a single thread\n" \
                         "\t'-sw'\t\tsuppress warnings from getmatvar\n\n" \
                         "Examples:\n\tgetmatvar('my_workspace.mat')\n" \
                         "\tmy_struct = getmatvar('my_workspace.mat', 'my_struct')\n\n"

#define MATLAB_WARN_MESSAGE ""

#ifdef NO_MEX
#define mxMalloc malloc
#define mxFree free
typedef enum
{
	mxREAL, mxCOMPLEX
} mxComplexity;
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
	HDF5_FIXED_POINT = 0, HDF5_FLOATING_POINT, HDF5_TIME, HDF5_STRING, HDF5_BIT_FIELD, HDF5_OPAQUE, HDF5_COMPOUND, HDF5_REFERENCE, HDF5_ENUMERATED, HDF5_VARIABLE_LENGTH, HDF5_ARRAY, HDF5_UNKNOWN
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
	Filter* filters;
	uint8_t num_chunked_dims;
	uint64_t num_chunked_elems;
	uint64_t* chunked_dims;
	uint64_t* chunk_update;
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
	NO_OBJ_HINT = 0, FUNCTION_HINT, OBJECT_HINT, OPAQUE_HINT
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
	
	char* matlab_class;
	ChunkedInfo chunked_info;
	
	uint64_t* dims;
	uint8_t num_dims;
	uint64_t num_elems;
	size_t elem_size;
	
	uint64_t s_c_array_index;
	
	uint8_t layout_class;
	address_t data_address;
	DataArrays data_arrays;
	
	//address_t parent_obj_address;
	address_t this_obj_address;
	
	Data* super_object;
	Queue* sub_objects;
	uint32_t num_sub_objs;
};

typedef struct
{
#ifdef WIN32_LEAN_AND_MEAN
	CRITICAL_SECTION lock;
#else
	pthread_mutex_t lock;
#endif
	bool_t is_mapped;
	address_t pg_start_a;
	address_t pg_end_a;
	address_t map_start;//if already mapped by windows, the base offset of this map
	address_t map_end; //if already mapped by windows, what offset this map extends to
	address_t max_map_end;
	byte* pg_start_p;
	uint8_t num_using;
	uint32_t total_num_mappings;
} pageObject;

typedef struct
{
	address_t map_start;//if already mapped by windows, the base offset of this map
	address_t map_end; //if already mapped by windows, what offset this map extends to
	byte* map_start_ptr;
	byte* address_ptr;
	uint32_t num_using;
	bool_t is_mapped;
} mapObject;

typedef struct
{
	int num_vars;
	char** full_variable_names;
	char* filename;
} ParamStruct;

typedef enum
{
	VT_UNKNOWN, VT_LOCAL_NAME, VT_LOCAL_INDEX, VT_LOCAL_COORDINATES
} VariableNameType;

typedef struct
{
	VariableNameType variable_name_type;
	uint64_t variable_local_index;
	uint64_t variable_local_coordinates[HDF5_MAX_DIMS];
	char* variable_local_name;
} VariableNameToken;

//mapping.c
int getDataObjects(const char* filename, char** variable_names, int num_names);

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
Queue* map_objects;

int fd;
Superblock s_block;
uint64_t default_bytes;

size_t max_num_map_objs;

Queue* inflate_thread_obj_queue;
int num_avail_threads;          //number of processors - 1
int num_threads_user_def;          //user specifies number of threads
bool_t will_multithread;
bool_t will_suppress_warnings;

bool_t is_super_mapped;
byte* super_pointer;

bool_t threads_are_started; //only start the thread pool once
#ifdef WIN32_LEAN_AND_MEAN
CRITICAL_SECTION thread_acquisition_lock;
#else
pthread_mutex_t thread_acquisition_lock;
#endif
pageObject* page_objects;

Data* virtual_super_object;

int max_depth;
#ifdef NO_MEX
size_t curr_mmap_usage;
size_t max_mmap_usage;
#ifdef WIN32_LEAN_AND_MEAN
CRITICAL_SECTION mmap_usage_update_lock;
#else
pthread_mutex_t mmap_usage_update_lock;
#endif
#endif

#ifdef DO_MEMDUMP
FILE* dump;
pthread_cond_t dump_ready;
pthread_mutex_t dump_lock;
#endif

#ifdef WIN32_LEAN_AND_MEAN
CRITICAL_SECTION if_lock;
#else
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