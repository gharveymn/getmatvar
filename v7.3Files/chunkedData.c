#include "mapping.h"

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

int main()
{
	char* filename = "my_struct1.mat";
	TreeNode* root = malloc(sizeof(TreeNode));
	root->address = 0x15e8;
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
		printf("open() unsuccessful, Check errno: %d\n", errno);
		exit(EXIT_FAILURE);
	}
	
	//get file size
	size_t file_size = (size_t) lseek(fd, 0, SEEK_END);
	
	//find superblock;
	
	s_block = getSuperblock(fd, file_size);
	
	fillChunkTree(root, 3);
	close(fd);
	freeTree(root);
	printf("Complete\n");
	
}


void getChunkedData(Data* object)
{
	TreeNode* root = malloc(sizeof(TreeNode));
	root->address = object->data_address;
	fillChunkTree(root, object->chunked_info.num_chunked_dims);
	decompressChunk(object, root);
	freeTree(root);
}

void decompressChunk(Data* object, TreeNode* node)
{
	//we're just gonna assume MathWorks is only using DEFLATE rn, I don't want to have to deal with the 5 others
	if(node->node_type == NODETYPE_UNDEFINED)
	{
		//we want to select nodes from level 0 since that level holds the keys
		return;
	}
	
	decompressChunk(object, node->left_sibling);
	for(int i = 0; i < node->entries_used; i++)
	{
		decompressChunk(object, &node->children[i]);
	}
	decompressChunk(object, node->right_sibling);
	
	//make sure this is done after the recursive calls since we will run out of memory otherwise
	
	int ret = Z_OK;
	size_t amount_processed = 0;
	z_stream strm;
	uint8_t in[CHUNK_BUFFER_SIZE];
	uint8_t out[CHUNK_BUFFER_SIZE];
	
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit(&strm);
	if (ret != Z_OK)
	{
		printf("Error in intialization of inflate");
		system(EXIT_FAILURE);
	}
	
	
}


void fillChunkTree(TreeNode* root, uint64_t num_chunked_dims)
{
	fillNode(root, num_chunked_dims, root);
}


void fillNode(TreeNode* node, uint64_t num_chunked_dims, TreeNode* previous_node)
{
	
	node->node_type = NODETYPE_UNDEFINED;
	node->leaf_type = LEAFTYPE_UNDEFINED;
	
	if(node->address == UNDEF_ADDR)
	{
		//this is an ending sentinel for the sibling linked list
		node->entries_used = 0;
		node->keys = NULL;
		node->children = NULL;
		node->left_sibling = NULL;
		node->right_sibling = NULL;
		node->node_level = previous_node->node_level;
		return;
	}
	
	char* tree_pointer = navigateTo(node->address, 8, TREE);
	
	if(strncmp(tree_pointer, "TREE", 4) != 0)
	{
		node->entries_used = 0;
		node->keys = NULL;
		node->children = NULL;
		node->left_sibling = NULL;
		node->right_sibling = NULL;
		node->node_level = -1;
		if(strncmp(tree_pointer, "SNOD", 4) == 0)
		{
			//(from group node)
			node->leaf_type = SYMBOL;
		}
		else
		{
			node->leaf_type = RAWDATA;
		}
		return;
	}
	
	node->node_type = (NodeType) getBytesAsNumber(tree_pointer + 4, 1);
	node->node_level = (int16_t) getBytesAsNumber(tree_pointer + 5, 1);
	node->entries_used = (uint16_t) getBytesAsNumber(tree_pointer + 6, 2);
	
	node->keys = malloc((node->entries_used + 1) * sizeof(Key));
	node->children = malloc(node->entries_used * sizeof(*node));
	node->left_sibling = malloc(sizeof(TreeNode));
	node->right_sibling = malloc(sizeof(TreeNode));
	
	for(int k = 0; k < node->entries_used + 1; k++)
	{
		node->keys[k].chunk_start = malloc((num_chunked_dims + 1) * sizeof(uint64_t));
	}
	
	uint64_t key_size;
	uint64_t bytes_needed;
	char* key_pointer;
	uint64_t key_address;
	
	switch(node->node_type)
	{
		
		//unneeded for chunk blocks, but maybe useful
		case GROUP:
			key_size = s_block.size_of_lengths;
			
			bytes_needed = 8 + 2 * s_block.size_of_offsets + 2 * (node->entries_used + 1) * key_size + 2 * (node->entries_used) * s_block.size_of_offsets;
			
			tree_pointer = navigateTo(node->address, bytes_needed, TREE);
			node->left_sibling->address = getBytesAsNumber(tree_pointer + 8, s_block.size_of_offsets);
			if(node->left_sibling->address != UNDEF_ADDR)
			{
				node->left_sibling->address += s_block.base_address;
			}
			if(node->left_sibling->address != previous_node->address)
			{
				fillNode(node->left_sibling, num_chunked_dims, node);
			}
			else
			{
				node->left_sibling = previous_node;
			}
			
			tree_pointer = navigateTo(node->address, bytes_needed, TREE);
			node->right_sibling->address = getBytesAsNumber(tree_pointer + 8 + s_block.size_of_offsets, s_block.size_of_offsets);
			if(node->right_sibling->address != UNDEF_ADDR)
			{
				node->right_sibling->address += s_block.base_address;
			}
			if(node->right_sibling->address != previous_node->address)
			{
				fillNode(node->right_sibling, num_chunked_dims, node);
			}
			else
			{
				node->right_sibling = previous_node;
			}
			
			key_pointer = tree_pointer + 8 + 2 * s_block.size_of_offsets;
			key_address = node->address + 8 + 2 * s_block.size_of_offsets;
			for(int i = 0; i < node->entries_used; i++)
			{
				node->keys[i].local_heap_offset = getBytesAsNumber(key_pointer, s_block.size_of_lengths);
				node->children[i].address = getBytesAsNumber(key_pointer + key_size, s_block.size_of_offsets) + s_block.base_address;
				if(node->children[i].address != previous_node->address)
				{
					fillNode(&node->children[i], num_chunked_dims, node);
				}
				else
				{
					printf("Incestuous nodes, it is not nor it cannot come to good");
					system(EXIT_FAILURE);
				}
				key_pointer += key_size + s_block.size_of_offsets;
				key_address += key_size + s_block.size_of_offsets;
			}
			
			break;
		case CHUNK:
			//1-4: Size of chunk in bytes
			//4-8 Filter mask
			//(Dimensionality+1)*16 bytes
			key_size = 4 + 4 + (num_chunked_dims + 1) * 8;
			
			bytes_needed = 8 + 2 * s_block.size_of_offsets + 2 * (node->entries_used + 1) * key_size + 2 * (node->entries_used) * s_block.size_of_offsets;
			
			tree_pointer = navigateTo(node->address, bytes_needed, TREE);
			node->left_sibling->address = getBytesAsNumber(tree_pointer + 8, s_block.size_of_offsets);
			if(node->left_sibling->address != UNDEF_ADDR)
			{
				node->left_sibling->address += s_block.base_address;
			}
			if(node->left_sibling->address != previous_node->address)
			{
				fillNode(node->left_sibling, num_chunked_dims, node);
			}
			else
			{
				node->left_sibling = previous_node;
			}
			
			
			tree_pointer = navigateTo(node->address, bytes_needed, TREE);
			node->right_sibling->address = getBytesAsNumber(tree_pointer + 8 + s_block.size_of_offsets, s_block.size_of_offsets);
			if(node->right_sibling->address != UNDEF_ADDR)
			{
				node->right_sibling->address += s_block.base_address;
			}
			if(node->right_sibling->address != previous_node->address)
			{
				fillNode(node->right_sibling, num_chunked_dims, node);
			}
			else
			{
				node->right_sibling = previous_node;
			}
			
			tree_pointer = navigateTo(node->address, bytes_needed, TREE);
			key_pointer = tree_pointer + 8 + 2 * s_block.size_of_offsets;
			key_address = node->address + 8 + 2 * s_block.size_of_offsets;
			node->keys[0].size = (uint32_t) getBytesAsNumber(key_pointer, 4);
			node->keys[0].filter_mask = (uint32_t) getBytesAsNumber(key_pointer + 4, 4);
			//node->keys[0].chunk_start = malloc((num_chunked_dims + 1) * sizeof(uint64_t));
			for(int j = 0; j < num_chunked_dims + 1; j++)
			{
				node->keys[0].chunk_start[j] = getBytesAsNumber(key_pointer + 8 + j * 8, 8);
			}
			
			for(int i = 0; i < node->entries_used; i++)
			{
				node->children[i].address = getBytesAsNumber(key_pointer + key_size, s_block.size_of_offsets) + s_block.base_address;
				if(node->children[i].address != previous_node->address)
				{
					fillNode(&(node->children[i]), num_chunked_dims, node);
				}
				else
				{
					printf("Incestuous nodes, it is not nor it cannot come to good");
					system(EXIT_FAILURE);
				}
				navigateTo(node->address, bytes_needed, TREE);
				key_pointer += key_size + s_block.size_of_offsets;
				key_address += key_size + s_block.size_of_offsets;
				
				node->keys[i + 1].size = (uint32_t) getBytesAsNumber(key_pointer, 4);
				node->keys[i + 1].filter_mask = (uint32_t) getBytesAsNumber(key_pointer + 4, 4);
				//node->keys[i + 1].chunk_start = malloc((num_chunked_dims + 1) * sizeof(uint64_t));
				for(int j = 0; j < num_chunked_dims + 1; j++)
				{
					node->keys[i + 1].chunk_start[j] = getBytesAsNumber(key_pointer + 8 + j * 8, 8);
				}
			}
			
			break;
		default:
			printf("Invalid node type %d", node->node_type);
			system(EXIT_FAILURE);
	}
}


void freeTree(TreeNode* node)
{
	if(node != NULL)
	{
		if(node->keys != NULL)
		{
			free(node->keys);
		}
		
		if(node->left_sibling != NULL)
		{
			free(node->left_sibling);
		}
		
		if(node->right_sibling != NULL)
		{
			free(node->right_sibling);
		}
		
		for(int i = 0; i < node->entries_used; i++)
		{
			freeTree(&node->children[i]);
		}
		
		if(node->children != NULL)
		{
			free(node->children);
		}
	}
}