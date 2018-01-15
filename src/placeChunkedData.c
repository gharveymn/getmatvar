#include "headers/placeChunkedData.h"
#include "headers/getDataObjects.h"
#include "headers/ezq.h"

static Queue** data_page_buckets;
static Queue* mt_data_queue;

errno_t getChunkedData(Data* object)
{
	
	TreeNode* root = malloc(sizeof(TreeNode));
	root->address = object->data_address;
	
	data_page_buckets = malloc(num_pages*sizeof(Queue));
	for(int i = 0; i < num_pages; i++)
	{
		data_page_buckets[i] = initQueue(NULL);
	}
	
	//fill the chunk tree
	errno_t ret = fillNode(root, object->chunked_info.num_chunked_dims);
	if(ret != 0)
	{
		error_flag = TRUE;
		sprintf(error_id, "getmatvar:internalInvalidNodeType");
		sprintf(error_message, "Invalid node type in fillNode()\n\n");
		return ret;
	}
	
	mt_data_queue = mergeQueue(data_page_buckets, (int)num_pages, free);
	for(int i = 0; i < num_pages; i++)
	{
		freeQueue(data_page_buckets[i]);
	}
	free(data_page_buckets);
	
	InflateThreadObj* thread_object = malloc(sizeof(InflateThreadObj));
	thread_object->object = object;
	thread_object->err = 0;
	
	if(will_multithread == TRUE)
	{
		int num_threads;
		
		if(num_threads_user_def != -1)
		{
			num_threads = num_threads_user_def;
		}
		else
		{
			//num_threads = (int)(-5.69E4*pow(object->num_elems, -0.7056) + 7.502);
			//num_threads = MIN(num_threads, num_avail_threads);
			num_threads = num_avail_threads;
			num_threads = MAX(num_threads, 1);
		}
		
		pthread_t* chunk_threads = malloc(num_threads*sizeof(pthread_t));
		pthread_attr_t attr;
		void* status;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		for(int i = 0; i < num_threads; i++)
		{
			pthread_create(&chunk_threads[i], &attr, doInflate_, (void *)thread_object);
		}
		pthread_attr_destroy(&attr);
		for(int i = 0; i < num_threads; i++)
		{
			pthread_join(chunk_threads[i], &status);
		}
		free(chunk_threads);
		
	}
	else
	{
		doInflate_((void*)thread_object);
	}
	free(thread_object);
	
	freeQueue(mt_data_queue);
	freeTree(root);
	
	return ret;
}


errno_t decompressChunk(Data* object)
{
	
	InflateThreadObj* thread_object = malloc(sizeof(InflateThreadObj));
	thread_object->object = object;
	thread_object->err = 0;
	
	if(will_multithread == TRUE)
	{
		enqueue(inflate_thread_obj_queue, thread_object);
		thpool_add_work(threads, (void*)doInflate_, (void*)thread_object);
	}
	
	return 0;
	
}


void* doInflate_(void* t)
{
	//make sure this is done after the recursive calls since we will run out of memory otherwise
	InflateThreadObj* thread_obj = (InflateThreadObj*)t;
	Data* object = thread_obj->object;
	uint64_t these_num_chunked_elems = 0;
	uint32_t these_chunked_dims[HDF5_MAX_DIMS + 1] = {0};
	uint64_t these_chunked_updates[HDF5_MAX_DIMS];
	const size_t actual_size = object->chunked_info.num_chunked_elems*object->elem_size; /* make sure this is non-null */
	
	struct libdeflate_decompressor* ldd = libdeflate_alloc_decompressor();
	byte* decompressed_data_buffer = malloc(object->chunked_info.num_chunked_elems*object->elem_size);
	uint64_t* index_map = malloc(object->chunked_info.num_chunked_elems*sizeof(uint64_t));
	uint32_t chunk_pos[HDF5_MAX_DIMS + 1] = {0};
	
	Key data_key;
	TreeNode* data_node;
	while(mt_data_queue->length > 0)
	{
		
		DataPair* dp = (DataPair*)dequeue(mt_data_queue);
		if(dp == NULL)
		{
			//in case it gets dequeued after the loop check but before the lock
			break;
		}
		else
		{
			data_key = dp->data_key;
			data_node = dp->data_node;
		}
		
		const uint64_t chunk_start_index = findArrayPosition(data_key.chunk_start, object->dims, object->num_dims);
		//const byte* data_pointer = navigateTo(node->children[i]->address, node->keys[i].size);
		const byte* data_pointer = navigateTo(data_node->address, 0);
		thread_obj->err = libdeflate_zlib_decompress(ldd, data_pointer, data_key.size, decompressed_data_buffer, actual_size, NULL);
		//releasePages(node->children[i]->address, node->keys[i].size);
		releasePages(data_node->address, 0);
		switch(thread_obj->err)
		{
			case LIBDEFLATE_BAD_DATA:
				error_flag = TRUE;
				sprintf(error_id, "getmatvar:libdeflateBadData");
				sprintf(error_message, "libdeflate failed to decompress data which was either invalid, corrupt or otherwise unsupported.\n\n");
				return (void*)&thread_obj->err;
			case LIBDEFLATE_SHORT_OUTPUT:
				error_flag = TRUE;
				sprintf(error_id, "getmatvar:libdeflateShortOutput");
				sprintf(error_message, "libdeflate failed failed to decompress because a NULL "
						"'actual_out_nbytes_ret' was provided, but the data would have"
						" decompressed to fewer than 'out_nbytes_avail' bytes.\n\n");
				return (void*)&thread_obj->err;
			case LIBDEFLATE_INSUFFICIENT_SPACE:
				error_flag = TRUE;
				sprintf(error_id, "getmatvar:libdeflateInsufficientSpace");
				sprintf(error_message, "libdeflate failed because the output buffer was not large enough (tried to put "
						"%d bytes into %d byte buffer).\n\n", (int)actual_size, CHUNK_BUFFER_SIZE);
				return (void*)&thread_obj->err;
			default:
				//do nothing
				break;
		}
		
		/* resize chunked dims if the chunk collides with the edge
		 *  ie
		 *   ==================================================
		 *  ||                     |                          ||
		 *  ||      (chunk)        |                          ||
		 *  ||                     |                          ||
		 *  ||---------------------|                          ||
		 *  ||                     |                          ||
		 *  ||      (chunk)        |                          ||
		 *  ||                     |                          ||
		 *  ||---------------------|                          ||
		 *  ||                     |                          ||
		 *  ||  chunk (collides)   |                          ||
		 *   ==================================================
		 */
		
		these_num_chunked_elems = object->chunked_info.num_chunked_elems;
		for(int j = 0; j < object->num_dims; j++)
		{
			these_chunked_dims[j] = object->chunked_info.chunked_dims[j] - MAX((int)(data_key.chunk_start[j] + object->chunked_info.chunked_dims[j] - object->dims[j]), 0);
			these_chunked_updates[j] = object->chunked_info.chunk_update[j];
		}
		
		for(int j = 0; j < object->num_dims; j++)
		{
			if(unlikely(these_chunked_dims[j] != object->chunked_info.chunked_dims[j]))
			{
				makeChunkedUpdates(these_chunked_updates, these_chunked_dims, object->dims, object->num_dims);
				these_num_chunked_elems = 1;
				for(int k = 0; k < object->chunked_info.num_chunked_dims; k++)
				{
					these_num_chunked_elems *= these_chunked_dims[k];
				}
				break;
			}
		}
		
		//copy over data
		memset(chunk_pos, 0, sizeof(chunk_pos));
		uint8_t curr_max_dim = 2;
		uint64_t db_pos = 0;
		for(uint64_t index = chunk_start_index, anchor = 0; index < object->num_elems && db_pos < these_num_chunked_elems; anchor = db_pos)
		{
			for(; index < object->num_elems && db_pos < anchor + these_chunked_dims[0]; db_pos++, index++)
			{
				index_map[db_pos] = index;
			}
			chunk_pos[1]++;
			uint8_t use_update = 0;
			for(uint8_t j = 1; j < curr_max_dim; j++)
			{
				if(chunk_pos[j] == these_chunked_dims[j])
				{
					chunk_pos[j] = 0;
					chunk_pos[j + 1]++;
					curr_max_dim = curr_max_dim <= j + 1? curr_max_dim + (uint8_t)1 : curr_max_dim;
					use_update++;
				}
			}
			index += these_chunked_updates[use_update];
		}
		
		
		placeDataWithIndexMap(object, decompressed_data_buffer, db_pos, object->elem_size, object->byte_order, index_map);
		
	}
	
	libdeflate_free_decompressor(ldd);
	free(decompressed_data_buffer);
	free(index_map);
	return NULL;
	
}


uint64_t findArrayPosition(const uint32_t* coordinates, const uint32_t* array_dims, uint8_t num_chunked_dims)
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
	
	byte* tree_pointer = navigateTo(node->address, 8);
	
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
		releasePages(node->address, 8);
		return 0;
	}
	
	node->node_type = (NodeType)getBytesAsNumber(tree_pointer + 4, 1, META_DATA_BYTE_ORDER);
	node->node_level = (uint16_t)getBytesAsNumber(tree_pointer + 5, 1, META_DATA_BYTE_ORDER);
	node->entries_used = (uint16_t)getBytesAsNumber(tree_pointer + 6, 2, META_DATA_BYTE_ORDER);
	
	node->keys = malloc((node->entries_used + 1)*sizeof(Key));
	node->children = malloc(node->entries_used*sizeof(TreeNode*));
	
	uint64_t key_size;
	uint64_t bytes_needed;
	
	if(node->node_type != CHUNK)
	{
		error_flag = TRUE;
		sprintf(error_id, "getmatvar:wrongNodeType");
		sprintf(error_message, "A group node was found in the chunked data tree.\n\n");
		return 1;
	}
	
	
	//1-4: Size of chunk in bytes
	//4-8 Filter mask
	//(Dimensionality+1)*16 bytes
	key_size = 4 + 4 + (num_chunked_dims + 1)*8;
	
	releasePages(node->address, 8);
	uint64_t key_address = node->address + 8 + 2*s_block.size_of_offsets;
	bytes_needed = 8 + num_chunked_dims*8 + key_size + s_block.size_of_offsets;
	byte* key_pointer = navigateTo(key_address, bytes_needed);
	node->keys[0].size = (uint32_t)getBytesAsNumber(key_pointer, 4, META_DATA_BYTE_ORDER);
	node->keys[0].filter_mask = (uint32_t)getBytesAsNumber(key_pointer + 4, 4, META_DATA_BYTE_ORDER);
	for(int j = 0; j < num_chunked_dims; j++)
	{
		node->keys[0].chunk_start[num_chunked_dims - j - 1] = (uint32_t)getBytesAsNumber(key_pointer + 8 + j*8, 8, META_DATA_BYTE_ORDER);
	}
	node->keys[0].chunk_start[num_chunked_dims] = 0;
	
	for(int i = 0; i < node->entries_used; i++)
	{
		node->children[i] = malloc(sizeof(TreeNode));
		node->children[i]->address = getBytesAsNumber(key_pointer + key_size, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
		
		releasePages(key_address, bytes_needed);
		fillNode(node->children[i], num_chunked_dims);
		
		size_t page_index = node->children[i]->address/alloc_gran;
		if(node->children[i]->leaf_type == RAWDATA)
		{
			page_objects[page_index].total_num_mappings++;
			page_objects[page_index].max_map_end =
					MAX(page_objects[page_index].max_map_end, node->children[i]->address + node->keys[i].size - 1);
			DataPair* dp = malloc(sizeof(DataPair));
			dp->data_key = node->keys[i];
			dp->data_node = node->children[i];
			enqueue(data_page_buckets[page_index], dp);
		}
		
		key_address += key_size + s_block.size_of_offsets;
		key_pointer = navigateTo(key_address, bytes_needed);
		
		node->keys[i + 1].size = (uint32_t)getBytesAsNumber(key_pointer, 4, META_DATA_BYTE_ORDER);
		node->keys[i + 1].filter_mask = (uint32_t)getBytesAsNumber(key_pointer + 4, 4, META_DATA_BYTE_ORDER);
		for(int j = 0; j < num_chunked_dims; j++)
		{
			node->keys[i + 1].chunk_start[num_chunked_dims - j - 1] = (uint32_t)getBytesAsNumber(key_pointer + 8 + j*8, 8, META_DATA_BYTE_ORDER);
		}
		node->keys[i + 1].chunk_start[num_chunked_dims] = 0;
		
		
	}
	
	releasePages(key_address, bytes_needed);
	return 0;
	
}


void freeTree(void* n)
{
	TreeNode* node = (TreeNode*)n;
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
				freeTree(node->children[i]);
			}
			free(node->children);
			node->children = NULL;
		}
		
		free(node);
		
	}
}


#ifdef DO_MEMDUMP
void memdump(const char* type)
{
	
	pthread_mutex_lock(&dump_lock);
	
	fprintf(dump, "%s ", type);
	fflush(dump);
	
	//XXXXXXXXXXXXXXXXXXXXXOOOOOOOOOOOXOXXOXX
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
		//keep flushing so that we know where it crashed if it does
		fflush(dump);
	}
	
	fprintf(dump, "\n\n");
	
	fflush(dump);
	
	pthread_mutex_unlock(&dump_lock);
	
}
#endif


void* garbageCollection_(void* nothing)
{
	
	uint32_t num_gc = 1;
	
	while(is_working == TRUE)
	{
		for(int i = 0; i < num_pages; i++)
		{
			if(pthread_mutex_trylock(&page_objects[i].lock) != EBUSY && page_objects[i].is_mapped == TRUE)
			{
				//see if this page is probably going to be used again
				uint32_t usage_offset = usage_iterator - page_objects[i].last_use_time_stamp;
				if(page_objects[i].num_using == 0 && usage_offset > sum_usage_offset/num_gc)
				{
					if(munmap(page_objects[i].pg_start_p, page_objects[i].map_end - page_objects[i].map_base) != 0)
					{
						readMXError("getmatvar:badMunmapError", "munmap() unsuccessful in freeMap(). Check errno %s\n\n", strerror(errno));
					}
					sum_usage_offset += usage_offset;
				}
				page_objects[i].is_mapped = FALSE;
				pthread_mutex_unlock(&page_objects[i].lock);
			}
		}
	}
	
	pthread_exit(NULL);
	
	return NULL;
	
}


void makeChunkedUpdates(uint64_t chunk_update[32], const uint32_t chunked_dims[32], const uint32_t dims[32], uint8_t num_dims)
{
	uint64_t cu, du;
	for(int i = 0; i < num_dims; i++)
	{
		du = 1;
		cu = 0;
		for(int k = 0; k < i + 1; k++)
		{
			cu += (chunked_dims[k] - 1)*du;
			du *= dims[k];
		}
		chunk_update[i] = du - cu - 1;
	}
}