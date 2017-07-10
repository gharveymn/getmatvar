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

char* findSuperblock(int fd, size_t file_size);
long getAddress(char* chunk_start, int num_bytes);
void enqueuePair(Addr_Pair pair);
Addr_Pair dequeuePair();
Superblock fillSuperblock(char* chunk_start);
char* navigateTo(uint64_t address, int map_index);

MemMap maps[2];
Addr_Q queue;
int fd;

int main (int argc, char* argv[])
{
	char* filename = "my_struct.mat";
	char* chunk_start;
	Superblock s_block;
	char* tree_pointer;
	char* heap_pointer;

	//init maps
	maps[0].used = FALSE;
	maps[1].used = FALSE;

	//init queue
	queue.front = 0;
	queue.length = 0;
	queue.back = 0;

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
	chunk_start = findSuperblock(fd, file_size);
	s_block = fillSuperblock(chunk_start);

	//unmap superblock
	if (munmap(maps[0].map_start, maps[0].bytes_mapped) != 0)
	{
		printf("munmap() unsuccessful, Check errno: %d\n", errno);
		exit(EXIT_FAILURE);
	}
	maps[0].used = FALSE;

	while (queue.length > 0)
	{
		tree_pointer = navigateTo(queue.pairs[queue.front].tree_address, TREE);
		assert(strncmp("TREE", tree_pointer, 4) == 0);
		heap_pointer = navigateTo(queue.pairs[queue.front].heap_address, HEAP);
		assert(strncmp("HEAP", heap_pointer, 4) == 0);
		dequeuePair();
	}
	
	exit(EXIT_SUCCESS);
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
long getAddress(char* chunk_start, int num_bytes)
{
	long ret = 0;
	int n = 0;
	while (n < num_bytes)
	{
		ret += (*(chunk_start + n) << (8*n)) & (long)(pow(2, 8*n + 8) - 1);
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
Superblock fillSuperblock(char* chunk_start)
{
	Superblock s_block;
	//get stuff from superblock, for now assume consistent versions of stuff
	s_block.size_of_offsets = getAddress(chunk_start + 13, 1);
	s_block.size_of_lengths = getAddress(chunk_start + 14, 1);
	s_block.leaf_node_k = getAddress(chunk_start + 16, 2);
	s_block.internal_node_k = getAddress(chunk_start + 18, 2);
	s_block.base_address = getAddress(chunk_start + 24, 8);

	//read scratchpad space
	char* sps_start = chunk_start + 80;
	Addr_Pair root_pair;
	root_pair.tree_address = getAddress(sps_start, 8) + s_block.base_address;
	root_pair.heap_address = getAddress(sps_start + 8, 8) + s_block.base_address;
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
		size_t page_size = sysconf(_SC_PAGE_SIZE);
		maps[map_index].offset = (off_t)(address/page_size);
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