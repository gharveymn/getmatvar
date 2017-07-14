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

Superblock getSuperblock(int fd, size_t file_size);
char* findSuperblock(int fd, size_t file_size);
uint64_t getBytesAsNumber(char* chunk_start, int num_bytes);
void enqueuePair(Addr_Pair pair);
void flushQueue();
Addr_Pair dequeuePair();
Superblock fillSuperblock(char* superblock_pointer);
char* navigateTo(uint64_t address, int map_index);
void readTreeNode(char* tree_address);
uint64_t readSnod(char* snod_pointer, char* heap_pointer, char* var_name);

MemMap maps[2];
Addr_Q queue;
int fd;
Superblock s_block;

int main (int argc, char* argv[])
{
	char* filename = "my_struct.mat";
	char variable_name[] = "my_struct.array"; //must be [] not * in order for strtok() to work
	char* delim = ".";
	char* tree_pointer;
	char* heap_pointer;
	char* token;

	uint64_t header_address = 0;

	token = strtok(variable_name, delim);

	//init maps
	maps[0].used = FALSE;
	maps[1].used = FALSE;

	//init queue
	flushQueue();

	//open the file descriptor
	fd = open(filename, O_RDWR);
	if (fd < 0)
	{
		printf("open() unsuccessful, Check errno: %d\n", errno);
		exit(EXIT_FAILURE);
	}

	//get file size
	size_t file_size = lseek(fd, 0, SEEK_END);

	//find superblock
	s_block = getSuperblock(fd, file_size);

	while (queue.length > 0)
	{
		tree_pointer = navigateTo(queue.pairs[queue.front].tree_address, TREE);
		heap_pointer = navigateTo(queue.pairs[queue.front].heap_address, HEAP);
		assert(strncmp("HEAP", heap_pointer, 4) == 0);

		if (strncmp("TREE", tree_pointer, 4) == 0)
		{
			readTreeNode(tree_pointer);
			dequeuePair();
		}
		else if (strncmp("SNOD", tree_pointer, 4) == 0)
		{
			header_address = readSnod(tree_pointer, heap_pointer, token);
			//b_tree that is next on the variable_name path should be only pair in queue
			//check if there is more path to explore and continue if so, break otherwise
			token = strtok(NULL, delim);
			if (token == NULL)
			{
				break;
			}

			if (header_address == UNDEF_ADDR)
			{
				dequeuePair();
			}
		}
	}
	printf("Object header for variable %s is at 0x%lx\n", variable_name, header_address);
	
	exit(EXIT_SUCCESS);
}
Superblock getSuperblock(int fd, size_t file_size)
{
	char* superblock_pointer = findSuperblock(fd, file_size);
	Superblock s_block = fillSuperblock(superblock_pointer);

	//unmap superblock
	if (munmap(maps[0].map_start, maps[0].bytes_mapped) != 0)
	{
		printf("munmap() unsuccessful in getSuperblock(), Check errno: %d\n", errno);
		exit(EXIT_FAILURE);
	}
	maps[0].used = FALSE;

	return s_block;
}
char* findSuperblock(int fd, size_t file_size)
{
	char* chunk_start = "";
	size_t page_size = sysconf(_SC_PAGE_SIZE);

	//Assuming that superblock is in first 8 512 byte chunks
	maps[0].map_start = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	maps[0].bytes_mapped = page_size;
	maps[0].offset = 0;
	maps[0].used = TRUE;

	if (maps[0].map_start == NULL || maps[0].map_start == MAP_FAILED)
	{
		printf("mmap() unsuccessful, Check errno: %d\n", errno);
		exit(EXIT_FAILURE);
	}

	chunk_start = maps[0].map_start;
	
	while (strncmp(FORMAT_SIG, chunk_start, 8) != 0 && (chunk_start - maps[0].map_start) < page_size)
	{
		chunk_start += 512;
	}

	if ((chunk_start - maps[0].map_start) >= page_size)
	{
		printf("Couldn't find superblock in first 8 512-byte chunks. I am quitting.\n");
		exit(EXIT_FAILURE);
	}

	return chunk_start;
}
uint64_t getBytesAsNumber(char* chunk_start, int num_bytes)
{
	uint64_t ret = 0;
	int n = 0;
	uint8_t byte = 0;
	uint64_t temp = 0;
	while (n < num_bytes)
	{
		byte = *(chunk_start + n);
		temp = byte;
		temp = temp << (8*n);
		ret += temp;
		n++;
	}
	return ret;
}
void enqueuePair(Addr_Pair pair)
{
	if (queue.length >= MAX_Q_LENGTH)
	{
		printf("Not enough room in queue\n");
		exit(EXIT_FAILURE);
	}
	queue.pairs[queue.back].tree_address = pair.tree_address;
	queue.pairs[queue.back].heap_address = pair.heap_address;
	queue.length++;

	if (queue.back < MAX_Q_LENGTH - 1)
	{
		queue.back++;
	}
	else
	{
		queue.back = 0;
	}
}
void priorityEnqueuePair(Addr_Pair pair)
{
	if (queue.length >= MAX_Q_LENGTH)
	{
		printf("Not enough room in queue\n");
		exit(EXIT_FAILURE);
	}
	if (queue.front - 1 < 0)
	{
		queue.pairs[MAX_Q_LENGTH - 1].tree_address = pair.tree_address;
		queue.pairs[MAX_Q_LENGTH - 1].heap_address = pair.heap_address;
		queue.front = MAX_Q_LENGTH - 1;
	}
	else
	{
		queue.pairs[queue.front - 1].tree_address = pair.tree_address;
		queue.pairs[queue.front - 1].heap_address = pair.heap_address;
		queue.front--;
	}
	queue.length++;
}
void flushQueue()
{
	queue.length = 0;
	queue.front = 0;
	queue.back = 0;
}
Addr_Pair dequeuePair()
{
	Addr_Pair pair;
	pair.tree_address = queue.pairs[queue.front].tree_address;
	pair.heap_address = queue.pairs[queue.front].heap_address;
	if (queue.front + 1 < MAX_Q_LENGTH)
	{
		queue.front++;
	}
	else
	{
		queue.front = 0;
	}
	queue.length--;
	return pair;
}
Superblock fillSuperblock(char* superblock_pointer)
{
	Superblock s_block;
	//get stuff from superblock, for now assume consistent versions of stuff
	s_block.size_of_offsets = getBytesAsNumber(superblock_pointer + 13, 1);
	s_block.size_of_lengths = getBytesAsNumber(superblock_pointer + 14, 1);
	s_block.leaf_node_k = getBytesAsNumber(superblock_pointer + 16, 2);
	s_block.internal_node_k = getBytesAsNumber(superblock_pointer + 18, 2);
	s_block.base_address = getBytesAsNumber(superblock_pointer + 24, s_block.size_of_offsets);

	//read scratchpad space
	char* sps_start = superblock_pointer + 80;
	Addr_Pair root_pair;
	root_pair.tree_address = getBytesAsNumber(sps_start, s_block.size_of_offsets) + s_block.base_address;
	root_pair.heap_address = getBytesAsNumber(sps_start + s_block.size_of_offsets, s_block.size_of_offsets) + s_block.base_address;
	enqueuePair(root_pair);

	return s_block;
}
char* navigateTo(uint64_t address, int map_index)
{
	//ensure there are always 1024 bytes available for reading if we don't remap
	if (maps[map_index].used && address >= maps[map_index].offset && address < maps[map_index].bytes_mapped - 1024)
	{
		return maps[map_index].map_start + address;
	}
	else
	{
		//unmap current page if used
		if (maps[map_index].used)
		{
			if (munmap(maps[map_index].map_start, maps[map_index].bytes_mapped) != 0)
			{
				printf("munmap() unsuccessful in navigateTo(), Check errno: %d\n", errno);
				printf("1st arg: %s\n2nd arg: %lu\nUsed: %d\n", maps[map_index].map_start, maps[map_index].bytes_mapped, maps[map_index].used);
				exit(EXIT_FAILURE);
			}
			maps[map_index].used = FALSE;
		}

		//map new page at needed location
		size_t page_size = sysconf(_SC_PAGE_SIZE);
		maps[map_index].offset = (off_t)(address/page_size)*page_size;
		maps[map_index].map_start = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, maps[map_index].offset);
		maps[map_index].bytes_mapped = page_size;
		maps[map_index].used = TRUE;
		if (maps[map_index].map_start == NULL || maps[map_index].map_start == MAP_FAILED)
		{
			printf("mmap() unsuccessful, Check errno: %d\n", errno);
			exit(EXIT_FAILURE);
		}
		return maps[map_index].map_start + (address % page_size);
	}
	
}
void readTreeNode(char* tree_pointer)
{
	Addr_Pair pair;
	uint16_t entries_used = 0;
	uint64_t left_sibling, right_sibling;

	entries_used = getBytesAsNumber(tree_pointer + 6, 2);

	//assuming keys do not contain pertinent information
	left_sibling = getBytesAsNumber(tree_pointer + 8, s_block.size_of_offsets);
	if (left_sibling != UNDEF_ADDR)
	{
		pair.tree_address = left_sibling + s_block.base_address;
		pair.heap_address = queue.pairs[queue.front].heap_address;
		enqueuePair(pair);
	}

	right_sibling = getBytesAsNumber(tree_pointer + 8 + s_block.size_of_offsets, s_block.size_of_offsets);
	if (right_sibling != UNDEF_ADDR)
	{
		pair.tree_address = right_sibling + s_block.base_address;
		pair.heap_address = queue.pairs[queue.front].heap_address;
		enqueuePair(pair);
	}

	for (int i = 0; i < entries_used; i++)
	{
		pair.tree_address = getBytesAsNumber(tree_pointer + 2*s_block.size_of_offsets + 16*(i + 1), s_block.size_of_offsets) + s_block.base_address;
		pair.heap_address = queue.pairs[queue.front].heap_address;
		enqueuePair(pair);
	}

}
uint64_t readSnod(char* snod_pointer, char* heap_pointer, char* var_name)
{
	uint16_t num_symbols = getBytesAsNumber(snod_pointer + 6, 2);
	Object* objects = (Object *)malloc(sizeof(Object)*num_symbols);
	uint32_t cache_type;
	Addr_Pair pair;
	uint64_t ret = UNDEF_ADDR;

	//get to entries
	snod_pointer += 8;

	for (int i = 0; i < num_symbols; i++)
	{
		objects[i].name_offset = getBytesAsNumber(snod_pointer + SYM_TABLE_ENTRY_SIZE*i, s_block.size_of_offsets);
		objects[i].obj_header_address = getBytesAsNumber(snod_pointer + SYM_TABLE_ENTRY_SIZE*i + s_block.size_of_offsets, s_block.size_of_offsets) + s_block.base_address;

		cache_type = getBytesAsNumber(snod_pointer + 2*s_block.size_of_offsets, 4);

		//check if we have found the object we're looking for
		if(strcmp(var_name, heap_pointer + 8 + 2*s_block.size_of_lengths + s_block.size_of_offsets + objects[i].name_offset) == 0)
		{
			ret = objects[i].obj_header_address;

			//if another tree exists for this object, put it on the queue
			if (cache_type == 1)
			{
				pair.tree_address = getBytesAsNumber(snod_pointer + 2*s_block.size_of_offsets + 8 + SYM_TABLE_ENTRY_SIZE*i, s_block.size_of_offsets) + s_block.base_address;
				pair.heap_address = getBytesAsNumber(snod_pointer + 3*s_block.size_of_offsets + 8 + SYM_TABLE_ENTRY_SIZE*i, s_block.size_of_offsets) + s_block.base_address;
				flushQueue();
				priorityEnqueuePair(pair);
			}
			break;
		}
	}

	free(objects);
	return ret;
}