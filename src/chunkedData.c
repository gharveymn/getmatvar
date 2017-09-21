#include "getmatvar_.h"
#include "extlib/thpool/thpool.h"


threadpool threads;
int num_threads;
static Data* object;
static int decompressor_iterator;
TreeNode root;
bool_t is_working;
pthread_t gc;
pthread_attr_t attr;

typedef struct inflate_thread_obj_ inflate_thread_obj;
struct inflate_thread_obj_
{
	TreeNode* node;
	int thread_decompressor_index;
	errno_t err;
};

Queue* thread_object_queue;

#if UINTPTR_MAX == 0xffffffff
#include "extlib/libdeflate/x86/libdeflate.h"
#elif UINTPTR_MAX == 0xffffffffffffffff
#include "extlib/libdeflate/x64/libdeflate.h"
#else
//you need at least 19th century hardware to run this
#endif

void* garbageCollection(void* nothing);


errno_t getChunkedData(Data* obj)
{
	object = obj;
	root.address = object->data_address;
	pthread_mutex_init(&thread_acquisition_lock, NULL);
	
	//fill the chunk tree
	errno_t ret = fillNode(&root, object->chunked_info.num_chunked_dims);
	if(ret != 0)
	{
		object->type = ERROR_DATA;
		sprintf(object->name, "getmatvar:internalInvalidNodeType");
		sprintf(object->matlab_class, "Invalid node type in fillNode()\n\n");
		freeTree(&root);
		return ret;
	}
	
	thread_object_queue = initQueue(NULL);
	
	if(num_threads_to_use != -1)
	{
		num_threads = num_threads_to_use;
	}
	else
	{
		num_threads = (int)(-5.69E4 * pow(object->num_elems, -0.7056) + 7.502);
		num_threads = MIN(num_threads, NUM_THREAD_MAPS);
		num_threads = MIN(num_threads, num_avail_threads);
		num_threads = MAX(num_threads, 1);
	}

//	decompressors = malloc(num_threads* sizeof(struct libdeflate_decompressor*));
	
	
	threads = thpool_init(num_threads);
	
	//TODO after multithreaded mapping is stable work on GC system using global iterator
	//TODO capture sections using the acquisition lock?
//	pthread_t gc;
//
//	is_working = TRUE;
//	pthread_attr_init(&attr);
//	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
//	pthread_create(&gc, &attr, garbageCollection, NULL);
	
	decompressor_iterator = 0;
	ret = decompressChunk(&root);
	
	thpool_wait(threads);
	thpool_destroy(threads);

//	is_working = FALSE;
//	pthread_join(gc, NULL);

	pthread_mutex_destroy(&thread_acquisition_lock);
	
	freeQueue(thread_object_queue);
	freeTree(&root);
	return ret;
}


errno_t decompressChunk(TreeNode* node)
{
	//this function just filters out all nodes which aren't one level above the leaves
	
	if(node->address == UNDEF_ADDR || node->node_type == NODETYPE_UNDEFINED || node->node_level < 0 || node->children == NULL)
	{
		//don't do the decompression from the leaf level or for any undefined object
		return 0;
	}
	
	for(int i = 0; i < node->entries_used; i++)
	{
		if(decompressChunk(&node->children[i]) != 0)
		{
			return 1;
		}
	}
	
	//there must be at least one child
	if(node->children[0].leaf_type != RAWDATA)
	{
		//only want nodes which are parents of the leaves to save memory
		return 0;
	}

//	for(int i = 0; i < node->entries_used; i++)
//	{
//		for (uint64_t j = node->children[i].address/alloc_gran;
//			j <= (node->children[i].address+node->keys[i].size)/alloc_gran;
//			j++)
//		{
//			pthread_mutex_init(&page_objects[j].lock, NULL);
//		}
//	}
	
	inflate_thread_obj* thread_object = malloc(sizeof(inflate_thread_obj));
	thread_object->thread_decompressor_index = decompressor_iterator;
	thread_object->node = node;
	thread_object->err = 0;
	enqueue(thread_object_queue, thread_object);
	thpool_add_work(threads, (void*)doInflate_, (void*)thread_object);
	decompressor_iterator = (decompressor_iterator + 1) % num_threads;
	
	return 0;
	
}


void* doInflate_(void* t)
{
	//make sure this is done after the recursive calls since we will run out of memory otherwise
	inflate_thread_obj* thread_obj = (inflate_thread_obj*)t;
	TreeNode* node = thread_obj->node;
	
	const size_t actual_size = object->chunked_info.num_chunked_elems * object->elem_size; /* make sure this is non-null */
	
	struct libdeflate_decompressor* ldd = libdeflate_alloc_decompressor();
	//struct libdeflate_decompressor* ldd = decompressors[thread_obj->thread_decompressor_index];
	volatile byte* decompressed_data_buffer = malloc(object->chunked_info.num_chunked_elems * object->elem_size);
	uint32_t chunk_pos[HDF5_MAX_DIMS + 1] = {0};
	uint64_t* index_map = malloc(object->chunked_info.num_chunked_elems * sizeof(uint64_t));
	
	for(int i = 0; i < node->entries_used; i++)
	{
		
		const uint64_t chunk_start_index = findArrayPosition(node->keys[i].chunk_start, object->dims, object->num_dims);
		//const byte* data_pointer = navigateWithMapIndex(node->children[i].address, node->keys[i].size, THREAD, thread_obj->thread_decompressor_index);
		const byte* data_pointer = navigatePolitely(node->children[i].address, node->keys[i].size);
		//thread_obj->err = libdeflate_zlib_decompress(ldd, data_pointer, node->keys[i].size, decompressed_data_buffer, actual_size, NULL);
		size_t retsz = 1;
		thread_obj->err = libdeflate_zlib_decompress(ldd, data_pointer, node->keys[i].size, decompressed_data_buffer, actual_size, &retsz);
		releasePages(node->children[i].address, node->keys[i].size);
		switch(thread_obj->err)
		{
			case LIBDEFLATE_BAD_DATA:
				object->type = ERROR_DATA;
				sprintf(object->name, "getmatvar:libdeflateBadData");
				sprintf(object->matlab_class, "libdeflate failed to decompress data which was either invalid, corrupt or otherwise unsupported.\n\n");
				return (void*)&thread_obj->err;
			case LIBDEFLATE_SHORT_OUTPUT:
				object->type = ERROR_DATA;
				sprintf(object->name, "getmatvar:libdeflateShortOutput");
				sprintf(object->matlab_class, "libdeflate failed failed to decompress because a NULL "
					   "'actual_out_nbytes_ret' was provided, but the data would have"
					   " decompressed to fewer than 'out_nbytes_avail' bytes.\n\n");
				return (void*)&thread_obj->err;
			case LIBDEFLATE_INSUFFICIENT_SPACE:
				object->type = ERROR_DATA;
				sprintf(object->name, "getmatvar:libdeflateInsufficientSpace");
				sprintf(object->matlab_class, "libdeflate failed because the output buffer was not large enough (tried to put "
					   "%d bytes into %d byte buffer).\n\n", (int)actual_size, CHUNK_BUFFER_SIZE);
				return (void*)&thread_obj->err;
			default:
				//do nothing
				break;
		}
		
		//copy over data
		memset(chunk_pos, 0, sizeof(chunk_pos));
		uint8_t curr_max_dim = 2;
		uint64_t db_pos = 0;
		for(uint64_t index = chunk_start_index, anchor = 0; index < object->num_elems && db_pos < object->chunked_info.num_chunked_elems; anchor = db_pos)
		{
			for(; db_pos < anchor + object->chunked_info.chunked_dims[0] && index < object->num_elems; db_pos++, index++)
			{
				index_map[db_pos] = index;
			}
			chunk_pos[1]++;
			uint8_t use_update = 0;
			for(uint8_t j = 1; j < curr_max_dim; j++)
			{
				if(chunk_pos[j] == object->chunked_info.chunked_dims[j])
				{
					chunk_pos[j] = 0;
					chunk_pos[j + 1]++;
					curr_max_dim = curr_max_dim <= j + 1 ? curr_max_dim + (uint8_t)1 : curr_max_dim;
					use_update++;
				}
			}
			index += object->chunked_info.chunk_update[use_update];
		}
		
		
		for(int k = 0; k < db_pos; k++)
		{
			if(*(double*)(decompressed_data_buffer + k*object->elem_size) != 1)
			{
				printf("");
			}
		}
		
		
		placeDataWithIndexMap(object, decompressed_data_buffer, db_pos, object->elem_size, object->byte_order, index_map);
		
	}
	
	free(decompressed_data_buffer);
	free(index_map);
	libdeflate_free_decompressor(ldd);
	return NULL;
	
}


uint64_t findArrayPosition(const uint64_t* coordinates, const uint32_t* array_dims, uint8_t num_chunked_dims)
{
	uint64_t array_pos = 0;
	for(int i = 0; i < num_chunked_dims; i++)
	{
		//[18,4] in array size [200,100]
		//3*200 + 17
		
		uint64_t temp = coordinates[i];
		for(int j = i - 1; j >= 0; j--)
		{
			temp *= array_dims[j];
		}
		array_pos += temp;
	}
	return array_pos;
}


errno_t fillNode(TreeNode* node, uint64_t num_chunked_dims)
{
	
	node->node_type = NODETYPE_UNDEFINED;
	node->leaf_type = LEAFTYPE_UNDEFINED;
	
	if(node->address == UNDEF_ADDR)
	{
		//this is an ending sentinel for the sibling linked list
		node->entries_used = 0;
		node->keys = NULL;
		node->children = NULL;
		node->node_level = -2;
		return 0;
	}
	
	byte* tree_pointer = navigateTo(node->address, 8, TREE);
	
	if(strncmp((char*)tree_pointer, "TREE", 4) != 0)
	{
		node->entries_used = 0;
		node->keys = NULL;
		node->children = NULL;
		node->node_level = -1;
		if(strncmp((char*)tree_pointer, "SNOD", 4) == 0)
		{
			//(from group node)
			node->leaf_type = SYMBOL;
		}
		else
		{
			node->leaf_type = RAWDATA;
		}
		return 0;
	}
	
	node->node_type = (NodeType)getBytesAsNumber(tree_pointer + 4, 1, META_DATA_BYTE_ORDER);
	node->node_level = (int16_t)getBytesAsNumber(tree_pointer + 5, 1, META_DATA_BYTE_ORDER);
	node->entries_used = (uint16_t)getBytesAsNumber(tree_pointer + 6, 2, META_DATA_BYTE_ORDER);
	
	node->keys = malloc((node->entries_used + 1) * sizeof(Key));
	node->children = malloc(node->entries_used * sizeof(TreeNode));
	
	uint64_t key_size;
	uint64_t bytes_needed;
	byte* key_pointer;
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
			node->keys[0].size = (uint32_t)getBytesAsNumber(key_pointer, 4, META_DATA_BYTE_ORDER);
			node->keys[0].filter_mask = (uint32_t)getBytesAsNumber(key_pointer + 4, 4, META_DATA_BYTE_ORDER);
			for(int j = 0; j < num_chunked_dims; j++)
			{
				node->keys[0].chunk_start[num_chunked_dims - j - 1] = getBytesAsNumber(key_pointer + 8 + j * 8, 8, META_DATA_BYTE_ORDER);
			}
			node->keys[0].chunk_start[num_chunked_dims] = 0;
			
			for(int i = 0; i < node->entries_used; i++)
			{
				node->children[i].address = getBytesAsNumber(key_pointer + key_size, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
				fillNode(&node->children[i], num_chunked_dims);
				
				if(i > 0 && node->node_level >= 0)
				{
					node->children[i - 1].right_sibling = &node->children[i];
					node->children[i].left_sibling = &node->children[i - 1];
				}
				
				tree_pointer = navigateTo(node->address, bytes_needed, TREE);
				key_pointer = tree_pointer + 8 + 2 * s_block.size_of_offsets + (i + 1) * (key_size + s_block.size_of_offsets);
				key_address += key_size + s_block.size_of_offsets;
				
				node->keys[i + 1].size = (uint32_t)getBytesAsNumber(key_pointer, 4, META_DATA_BYTE_ORDER);
				node->keys[i + 1].filter_mask = (uint32_t)getBytesAsNumber(key_pointer + 4, 4, META_DATA_BYTE_ORDER);
				for(int j = 0; j < num_chunked_dims; j++)
				{
					node->keys[i + 1].chunk_start[num_chunked_dims - j - 1] = getBytesAsNumber(key_pointer + 8 + j * 8, 8, META_DATA_BYTE_ORDER);
				}
				node->keys[i + 1].chunk_start[num_chunked_dims] = 0;
			}
			
			break;
		default:
			return 1;
	}
	
	return 0;
	
}


void freeTree(TreeNode* node)
{
	if(node != NULL)
	{
		if(node->keys != NULL)
		{
			free(node->keys);
			node->keys = NULL;
		}
		
		if(node->children != NULL)
		{
			for(int i = 0; i < node->entries_used; i++)
			{
				freeTree(&node->children[i]);
			}
			free(node->children);
			node->children = NULL;
		}
	}
}


void memdump(const char* type)
{
	
	pthread_mutex_lock(&dump_lock);
	
	fprintf(dump, type);
	fflush(dump);
	
	//XXXXXXXXXXXXXXXXXXXXXOOOOOOOOOOOXOXXOXX
	//XXXXXXXXXXXXXXXXXXXXXRRRRRRRRRRRXRXXRXX
	for(int i = 0; i < num_pages; i++)
	{
		if(page_objects[i].is_mapped == TRUE)
		{
			fprintf(dump, "O");
		}
		else
		{
			fprintf(dump, "X");
		}
		fflush(dump);
	}
	
	fprintf(dump, "\n  ");
	fflush(dump);
	
//	for(int i = 0; i < num_pages; i++)
//	{
//		if(page_objects[i].is_cont_right == TRUE)
//		{
//			fprintf(dump, "R");
//		}
//		else
//		{
//			fprintf(dump, "X");
//		}
//		fflush(dump);
//	}
	
	fprintf(dump, "\n\n");
	
	fflush(dump);
	
	pthread_mutex_unlock(&dump_lock);
	
}


void* garbageCollection(void* nothing)
{
	while(is_working == TRUE)
	{
		for(int i = 0; i < num_pages; i++)
		{
			if(pthread_mutex_trylock(&page_objects[i].lock) != EBUSY && page_objects[i].is_mapped == TRUE)
			{
				if(munmap(page_objects[i].pg_start_p, page_objects[i].pg_end_a - page_objects[i].pg_start_a) != 0)
				{
					readMXError("getmatvar:badMunmapError", "munmap() unsuccessful in freeMap(). Check errno %s\n\n", strerror(errno));
				}
			}
		}
	}
	
	pthread_exit(NULL);
	
	return NULL;
	
}