#include "mapping.h"

//doubly linked list
#define NUM_SIBLINGS 2

typedef enum
{
	GROUP = (uint8_t) 0,
	CHUNK = (uint8_t) 1,
	NODETYPE_UNDEFINED
} NodeType;

typedef enum
{
	LEAFTYPE_UNDEFINED,
	RAWDATA,
	SYMBOL
} LeafType;

typedef struct
{
	uint32_t size;
	uint32_t filter_mask;
	uint64_t *chunk_start;
	uint64_t local_heap_offset;
} Key;

typedef struct tree_node_ TreeNode;
struct tree_node_
{
	uint64_t address;
	NodeType node_type;
	LeafType leaf_type;
	uint8_t node_level;
	uint16_t entries_used;
	Key *keys;
	TreeNode* children;
	TreeNode* left_sibling;
	TreeNode* right_sibling;
};

void fillNode(TreeNode *node, int num_chunked_dims);
void freeTree(TreeNode *node);


int main()
{
	char *filename = "my_struct1.mat";
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
	if (fd < 0)
	{
		printf("open() unsuccessful, Check errno: %d\n", errno);
		exit(EXIT_FAILURE);
	}
	
	//get file size
	size_t file_size = lseek(fd, 0, SEEK_END);
	
	//find superblock;
	
	s_block = getSuperblock(fd, file_size);
	
	fillNode(&root, 3);
	close(fd);
	freeTree(&root);
	printf("Complete\n");
	
}

//void getChunkedData(char* node_pointer)
//{
//
//}


void fillNode(TreeNode *node, int num_chunked_dims)
{
	
	if (node->address == UNDEF_ADDR)
	{
		node->entries_used = 0;
		node->node_type = NODETYPE_UNDEFINED;
		node->leaf_type = LEAFTYPE_UNDEFINED;
		return;
	}
	
	char *tree_pointer = navigateTo(node->address, 8, TREE);
	
	if (strncmp(tree_pointer, "TREE", 4) != 0)
	{
		node->entries_used = 0;
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
	
	node->keys = calloc((node->entries_used + 1), sizeof(Key) + (num_chunked_dims + 1)*sizeof(uint64_t));
	node->children = malloc(node->entries_used * sizeof(*node));
	node->left_sibling = malloc(sizeof(TreeNode));
	node->right_sibling = malloc(sizeof(TreeNode));
	
	for (int k = 0; k < node->entries_used + 1; k++)
	{
		node->keys[k].chunk_start = malloc((num_chunked_dims + 1)*sizeof(uint64_t));
	}
	
	node->node_type = getBytesAsNumber(tree_pointer + 4, 1);
	node->node_level = getBytesAsNumber(tree_pointer + 5, 1);
	node->entries_used = getBytesAsNumber(tree_pointer + 6, 2);
	
	int key_size;
	int bytes_needed;
	char *key_pointer;
	int key_address;
	int tree_address = node->address;
	uint64_t chunk_start_val = 0;
	
	switch (node->node_type)
	{
		case GROUP:
			key_size = s_block.size_of_lengths;
			
			bytes_needed = 8 + 2 * s_block.size_of_offsets + 2 * (node->entries_used + 1) * key_size + 2 * (node->entries_used) * s_block.size_of_offsets;
			tree_pointer = navigateTo(node->address, bytes_needed, TREE);
			
			node->left_sibling->address = getBytesAsNumber(tree_pointer + 8, s_block.size_of_offsets);
			fillNode(node->left_sibling, num_chunked_dims);
			
			key_pointer = tree_pointer + 8 + 2 * s_block.size_of_offsets;
			key_address = tree_address + 8 + 2 * s_block.size_of_offsets;
			for (int i = 0; i < node->entries_used; i++)
			{
				node->keys[i].local_heap_offset = getBytesAsNumber(key_pointer, s_block.size_of_lengths);
				node->children[i].address = getBytesAsNumber(key_pointer + key_size, s_block.size_of_offsets) + s_block.base_address;
				fillNode(&node->children[i], num_chunked_dims);
				//re-navigate because the memory may have been unmapped in the above call
				tree_pointer = navigateTo(node->address, bytes_needed, TREE);
				key_pointer += key_size + s_block.size_of_offsets;
				key_address += key_size + s_block.size_of_offsets;
			}
			
			node->right_sibling->address = getBytesAsNumber(tree_pointer + 8 + s_block.size_of_offsets, s_block.size_of_offsets);
			fillNode(node->right_sibling, num_chunked_dims);
			
			break;
		case CHUNK:
			//1-4: Size of chunk in bytes
			//4-8 Filter mask
			//(Dimensionality+1)*16 bytes
			key_size = 4 + 4 + (num_chunked_dims + 1) * 8;
			
			bytes_needed = 8 + 2 * s_block.size_of_offsets + 2 * (node->entries_used + 1) * key_size + 2 * (node->entries_used) * s_block.size_of_offsets;
			tree_pointer = navigateTo(node->address, bytes_needed, TREE);
			
			node->left_sibling->address = getBytesAsNumber(tree_pointer + 8, s_block.size_of_offsets);
			fillNode(node->left_sibling, num_chunked_dims);
			
			//there are 2k+1 keys
			key_pointer = tree_pointer + 8 + 2 * s_block.size_of_offsets;
			key_address = tree_address + 8 + 2 * s_block.size_of_offsets;
			node->keys[0].size = getBytesAsNumber(key_pointer, 4);
			node->keys[0].filter_mask = getBytesAsNumber(key_pointer + 4, 4);
			//node->keys[0].chunk_start = malloc((num_chunked_dims + 1) * sizeof(uint64_t));
			for (int j = 0; j < num_chunked_dims + 1; j++)
			{
				node->keys[0].chunk_start[j] = getBytesAsNumber(key_pointer + 8 + j * 8, 8);
			}
			
			for (int i = 0; i < node->entries_used; i++)
			{
				node->children[i].address = getBytesAsNumber(key_pointer + key_size, s_block.size_of_offsets) + s_block.base_address;
				fillNode(&(node->children[i]), num_chunked_dims);
				//re-navigate because the memory may have been unmapped in the above call
				tree_pointer = navigateTo(node->address, bytes_needed, TREE);
				key_pointer += key_size + s_block.size_of_offsets;
				key_address += key_size + s_block.size_of_offsets;
				
				node->keys[i + 1].size = getBytesAsNumber(key_pointer, 4);
				node->keys[i + 1].filter_mask = getBytesAsNumber(key_pointer + 4, 4);
				//node->keys[i + 1].chunk_start = malloc((num_chunked_dims + 1) * sizeof(uint64_t));
				for (int j = 0; j < num_chunked_dims + 1; j++)
				{
					node->keys[i + 1].chunk_start[j] = getBytesAsNumber(key_pointer + 8 + j * 8, 8);
				}
			}
			
			node->right_sibling->address = getBytesAsNumber(tree_pointer + 8 + s_block.size_of_offsets, s_block.size_of_offsets);
			fillNode(node->right_sibling, num_chunked_dims);
			
			break;
		default:
			system(EXIT_FAILURE);
	}
}


void freeTree(TreeNode *node)
{
	if (node != NULL)
	{
		if (node->keys != NULL)
		{
			free(node->keys);
		}
		
		for (int i = 0; i < node->entries_used; i++)
		{
			freeTree(&node->children[i]);
		}
		
		if (node->children != NULL)
		{
			free(node->children);
		}
	}
}