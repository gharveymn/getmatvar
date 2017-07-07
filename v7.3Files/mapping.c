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

#define TRUE 0
#define FALSE 1
#define CHUNK 512
#define FORMAT_SIG "\211HDF\r\n\032\n"
#define MAX_Q_LENGTH 20

typedef struct
{
	long tree_address;
	long heap_address;
} Addr_Pair;

typedef struct
{
	Addr_Pair pairs[MAX_Q_LENGTH];
	int front;
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

char* findSuperblock(int fd, size_t file_size);
long getAddress(char* chunk_start, int num_bytes);
void addToQueue(Addr_Pair pair);
Addr_Pair removeFromQueue();

size_t bytes_mapped;
char* map_start;
Addr_Q queue;

int main (int argc, char* argv[])
{
	char* filename = "my_struct.mat";
	char* chunk_start;
	bytes_mapped = 0;
	Superblock s_block;

	queue.front = 0;
	queue.length = 0;

	//open the file descriptor
	int fd = open(filename, O_RDWR);
	if (fd < 0)
	{
		printf("open() unsuccessful, Check errno: %d\n", errno);
		exit(EXIT_FAILURE);
	}

	//get file size
	size_t file_size = lseek(fd, 0, SEEK_END);

	//find superblock
	chunk_start = findSuperblock(fd, file_size);
	
	//get stuff from superblock, for now assume consistent versions of stuff
	s_block.size_of_offsets = getAddress(chunk_start + 13, 1);
	s_block.size_of_lengths = getAddress(chunk_start + 14, 1);
	s_block.leaf_node_k = getAddress(chunk_start + 16, 2);
	s_block.internal_node_k = getAddress(chunk_start + 18, 2);
	s_block.base_address = getAddress(chunk_start + 24, 8);

	char* sps_start = chunk_start + 80;
	Addr_Pair root_pair;
	root_pair.tree_address = getAddress(sps_start, 8) + s_block.base_address;
	root_pair.heap_address = getAddress(sps_start + 8, 8) + s_block.base_address;;
	addToQueue(root_pair);

	if (munmap(map_start, bytes_mapped) != 0)
	{
		printf("munmap() unsuccessful, Check errno: %d\n", errno);
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}

char* findSuperblock(int fd, size_t file_size)
{
	char* chunk_start = "";
	size_t page_size = sysconf(_SC_PAGE_SIZE);

	//Assuming that superblock is in first 8 512 byte chunks
	map_start = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	bytes_mapped = page_size;
	if (map_start == NULL || map_start == MAP_FAILED)
	{
		printf("mmap() unsuccessful, Check errno: %d\n", errno);
		exit(EXIT_FAILURE);
	}

	chunk_start = map_start;
	
	while (strncmp(FORMAT_SIG, chunk_start, 8) != 0 && (chunk_start - map_start) < page_size)
	{
		chunk_start += 512;
	}

	if ((chunk_start - map_start) >= page_size)
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
void addToQueue(Addr_Pair pair)
{
	int free_index = -1;
	if (queue.front + queue.length < MAX_Q_LENGTH)
	{
		free_index = queue.front + queue.length;
	}
	else if (queue.length < MAX_Q_LENGTH - 1)
	{
		free_index = queue.length - MAX_Q_LENGTH + queue.front;
	}
	else
	{
		printf("Not enough room in queue\n");
		exit(EXIT_FAILURE);
	}
	queue.pairs[free_index].tree_address = pair.tree_address;
	queue.pairs[free_index].heap_address = pair.heap_address;
	queue.length += 1;
}
Addr_Pair removeFromQueue()
{
	Addr_Pair pair = queue.pairs[queue.front];
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