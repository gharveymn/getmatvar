#include "../src/mapping.h"

int main()
{
	char* filename = "my_struct1.mat";
	TreeNode root;
	root.address = 0x15e8;
	//init maps
	maps[0].used = FALSE;
	maps[1].used = FALSE;
	
	//init queue
	flushQueue();
	flushHeaderQueue();
	
	//open the file descriptor
	fd = open(filename, O_RDWR);
	if(fd < 0)
	{
		fprintf(stderr, "open() unsuccessful, Check errno: %d\n", errno);
		exit(EXIT_FAILURE);
	}
	
	//get file size
	size_t file_size = (size_t) lseek(fd, 0, SEEK_END);
	
	//find superblock;
	
	s_block = getSuperblock(fd, file_size);
	
	fillChunkTree(&root, 3);
	close(fd);
	freeTree(&root);
	fprintf(stderr, "Complete\n");
	
}

