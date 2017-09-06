#include "getMatVar.h"
#include "extlib/zlib/zlib.h"

void getChunkedData(Data* object)
{
	TreeNode root;
	root.address = object->data_address;
	fillChunkTree(&root, object->chunked_info.num_chunked_dims);
	decompressChunk(object, &root);
	freeTree(&root);
	free(root.left_sibling);
	root.left_sibling = NULL;
	free(root.right_sibling);
	root.right_sibling = NULL;
}

void decompressChunk(Data* object, TreeNode* node)
{
	//this function just filters out all nodes which aren't one level above the leaves
	
	if(node->address == UNDEF_ADDR || node->node_type == NODETYPE_UNDEFINED || node->node_level < 0 || node->children == NULL)
	{
		//don't do the decompression from the leaf level or for any undefined object
		return;
	}
	
	for(int i = 0; i < node->entries_used; i++)
	{
		decompressChunk(object, &node->children[i]);
	}
	
	//there must be at least one child
	if(node->children[0].leaf_type != RAWDATA)
	{
		//only want nodes which are parents of the leaves to save memory
		return;
	}
	
	doInflate(object, node);
	
}

void doInflate(Data* object, TreeNode* node)
{
	//make sure this is done after the recursive calls since we will run out of memory otherwise
	
	uint64_t chunk_start_index = 0, chunk_end_index = 0;
	uint64_t chunk_end_index_offset = 1;
	for(int i = 0; object->chunked_info.chunked_dims[i] > 0; i++)
	{
		chunk_end_index_offset *= object->chunked_info.chunked_dims[i];
	}
	
	int ret = Z_OK;
	z_stream strm;
	Bytef decompressed_data_buffer[CHUNK_BUFFER_SIZE];
	Bytef* data_pointer;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;

	ret = inflateInit(&strm);
	if (ret != Z_OK)
	{
		readMXError("getmatvar:internalError", "Error in intialization of inflate\n\n", "");
		//fprintf(stderr, "Error in intialization of inflate, ret = %d\n", ret);
		//exit(EXIT_FAILURE);
	}
	
	for(int i = 0; i < node->entries_used; i++)
	{
		
		data_pointer = (Bytef*)navigateTo(node->children[i].address, node->keys[i].size, TREE);
		strm.avail_in = node->keys[i].size;
		strm.next_in = data_pointer;
		
		do
		{
			
			do
			{
				strm.avail_out = CHUNK_BUFFER_SIZE;
				strm.next_out = decompressed_data_buffer;
				ret = inflate(&strm, Z_FINISH);
				assert(ret != Z_STREAM_ERROR);
				switch(ret)
				{
					case Z_NEED_DICT:
						ret = Z_DATA_ERROR;     /* and fall through */
					case Z_DATA_ERROR:
					case Z_MEM_ERROR:
						(void)inflateEnd(&strm);
						readMXError("getmatvar:internalError", "There was an error with inflating the chunk\n\n", "");
						//fprintf(stderr, "There was an error with inflating the chunk at %llu. Error: ret = %d\n", node->children[i].address, ret);
						//exit(EXIT_FAILURE);
				}
				
			} while (strm.avail_out == 0);
			
		} while(ret != Z_STREAM_END);
		
		inflateReset(&strm);
		
		chunk_start_index = findArrayPosition(node->keys[i].chunk_start, object->dims, object->num_dims);
		chunk_end_index = findArrayPosition(node->keys[i+1].chunk_start, object->dims, object->num_dims);

		//copy over data
		placeData(object, (char*)&decompressed_data_buffer[0], chunk_start_index, MIN(chunk_end_index, object->num_elems), object->elem_size, object->byte_order);
		
	}
	
	(void)inflateEnd(&strm);
	
	
}

uint64_t findArrayPosition(const uint64_t* coordinates, const uint32_t* array_dims, uint8_t num_chunked_dims)
{
	uint64_t array_pos = 0;
	uint64_t temp;
	for(int i = 0; i < num_chunked_dims; i++)
	{
		//[18,4] in array size [200,100]
		//3*200 + 17
		
		temp = coordinates[i];
		for(int j = i + 1; j < num_chunked_dims; j++)
		{
			temp *= array_dims[j];
		}
		array_pos += temp;
	}
	return array_pos;
}




void fillChunkTree(TreeNode* root, uint64_t num_chunked_dims)
{
	fillNode(root, num_chunked_dims);
	
	//for the sake of completeness
	root->left_sibling = malloc(sizeof(TreeNode));
	root->left_sibling->address = UNDEF_ADDR;
	root->left_sibling->keys = NULL;
	root->left_sibling->children = NULL;
	fillNode(root->left_sibling, num_chunked_dims);
	
	root->right_sibling = malloc(sizeof(TreeNode));
	root->right_sibling->address = UNDEF_ADDR;
	root->right_sibling->keys = NULL;
	root->right_sibling->children = NULL;
	fillNode(root->right_sibling, num_chunked_dims);
}


void fillNode(TreeNode* node, uint64_t num_chunked_dims)
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
		node->node_level = -2;
		return;
	}
	
	char* tree_pointer = navigateTo(node->address, 8, TREE);
	
	if(strncmp(tree_pointer, "TREE", 4) != 0)
	{
		node->entries_used = 0;
		node->keys = NULL;
		node->children = NULL;
		//node->left_sibling = NULL;
		//node->right_sibling = NULL;
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
	
	node->node_type = (NodeType) getBytesAsNumber(tree_pointer + 4, 1, META_DATA_BYTE_ORDER);
	node->node_level = (int16_t) getBytesAsNumber(tree_pointer + 5, 1, META_DATA_BYTE_ORDER);
	node->entries_used = (uint16_t) getBytesAsNumber(tree_pointer + 6, 2, META_DATA_BYTE_ORDER);
	
	node->keys = malloc((node->entries_used + 1) * sizeof(Key));
	node->children = malloc(node->entries_used * sizeof(TreeNode));
	
//	for(int k = 0; k < node->entries_used + 1; k++)
//	{
//		node->keys[k].chunk_start = malloc((num_chunked_dims + 1) * sizeof(uint64_t));
//	}
	
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
			key_pointer = tree_pointer + 8 + 2 * s_block.size_of_offsets;
			key_address = node->address + 8 + 2 * s_block.size_of_offsets;
			for(int i = 0; i < node->entries_used; i++)
			{
				node->keys[i].local_heap_offset = getBytesAsNumber(key_pointer, s_block.size_of_lengths, META_DATA_BYTE_ORDER);
				node->children[i].address = getBytesAsNumber(key_pointer + key_size, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
				fillNode(&node->children[i], num_chunked_dims);
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
			key_pointer = tree_pointer + 8 + 2 * s_block.size_of_offsets;
			key_address = node->address + 8 + 2 * s_block.size_of_offsets;
			node->keys[0].size = (uint32_t) getBytesAsNumber(key_pointer, 4, META_DATA_BYTE_ORDER);
			node->keys[0].filter_mask = (uint32_t) getBytesAsNumber(key_pointer + 4, 4, META_DATA_BYTE_ORDER);
			node->keys[0].chunk_start = malloc((num_chunked_dims + 1) * sizeof(uint64_t));
			for(int j = 0; j < num_chunked_dims + 1; j++)
			{
				node->keys[0].chunk_start[j] = getBytesAsNumber(key_pointer + 8 + j * 8, 8, META_DATA_BYTE_ORDER);
			}
			
			for(int i = 0; i < node->entries_used; i++)
			{
				node->children[i].address = getBytesAsNumber(key_pointer + key_size, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
				fillNode(&node->children[i], num_chunked_dims);
				
				if(i > 0 && i < node->entries_used-1 && node->node_level >= 0)
				{
					node->children[i].left_sibling = &node->children[i-1];
					node->children[i-1].right_sibling = &node->children[i];
				}
				
				tree_pointer = navigateTo(node->address, bytes_needed, TREE);
				key_pointer = tree_pointer + 8 + 2 * s_block.size_of_offsets + (i+1)*(key_size + s_block.size_of_offsets);
				key_address += key_size + s_block.size_of_offsets;
				
				node->keys[i + 1].size = (uint32_t) getBytesAsNumber(key_pointer, 4, META_DATA_BYTE_ORDER);
				node->keys[i + 1].filter_mask = (uint32_t) getBytesAsNumber(key_pointer + 4, 4, META_DATA_BYTE_ORDER);
				node->keys[i + 1].chunk_start = malloc((num_chunked_dims + 1) * sizeof(uint64_t));
				for(int j = 0; j < num_chunked_dims + 1; j++)
				{
					node->keys[i + 1].chunk_start[j] = getBytesAsNumber(key_pointer + 8 + j * 8, 8, META_DATA_BYTE_ORDER);
				}
			}
			
			if(node->node_level >= 0)
			{
				node->children[0].left_sibling = malloc(sizeof(TreeNode));
				node->children[0].left_sibling->address = UNDEF_ADDR;
				node->children[0].left_sibling->keys = NULL;
				node->children[0].left_sibling->children = NULL;
				fillNode(node->children[0].left_sibling, num_chunked_dims);
				
				node->children[node->entries_used-1].right_sibling = malloc(sizeof(TreeNode));
				node->children[node->entries_used-1].right_sibling->address = UNDEF_ADDR;
				node->children[node->entries_used-1].right_sibling->keys = NULL;
				node->children[node->entries_used-1].right_sibling->children = NULL;
				fillNode(node->children[node->entries_used-1].right_sibling, num_chunked_dims);
			}
			
			break;
		default:
			readMXError("getmatvar:internalError", "Invalid node type in fillNode()\n\n", "");
			//fprintf(stderr, "Invalid node type %d\n", node->node_type);
			//exit(EXIT_FAILURE);
	}
}


void freeTree(TreeNode* node)
{
	if(node != NULL)
	{
		if(node->keys != NULL)
		{
			for(int i = 0; i < node->entries_used+1; i++)
			{
				free(node->keys[i].chunk_start);
				node->keys[i].chunk_start = NULL;
			}
			
			free(node->keys);
			node->keys = NULL;
			
		}
		
		if(node->children != NULL)
		{
			free(node->children[0].left_sibling);
			node->children[0].left_sibling = NULL;
			free(node->children[node->entries_used-1].right_sibling);
			node->children[node->entries_used-1].right_sibling = NULL;
			for(int i = 0; i < node->entries_used; i++)
			{
				freeTree(&node->children[i]);
			}
			free(node->children);
			node->children = NULL;
		}
	}
}