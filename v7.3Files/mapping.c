#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#define TRUE 0
#define FALSE 1

typedef struct
{
	char* tree_address;
	char* heap_address;
} Addr_Pair;

typedef struct
{
	Addr_Pair pairs[20];
	int front;
	int length;
} Addr_Q;

int main (int argc, char* argv[])
{
	char* filename = "my_struct.mat";

	int fd = open(filename, O_RDWR);
	if (fd < 0)
	{
		printf("open() unsuccessful, Check errno: %d\n", errno);
		exit(EXIT_FAILURE);
	}

	size_t file_size = lseek(fd, 0, SEEK_END);

	char* file_start = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (file_start == NULL || file_start == MAP_FAILED)
	{
		printf("mmap() unsuccessful, Check errno: %d\n", errno);
		exit(EXIT_FAILURE);
	}

	if (munmap(file_start, file_size) != TRUE)
	{
		printf("munmap() unsuccessful, Check errno: %d\n", errno);
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}