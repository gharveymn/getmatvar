#include "headers/placeChunkedData.h"


Queue** data_page_buckets;
//MTQueue** mt_data_page_buckets;
int num_threads_to_use;
//bool_t is_vlarge;

error_t getChunkedData(Data* object)
{
	
	initializePageObjects();
	
	MTQueue* mt_data_queue = mt_initQueue((void*)freeQueue);
	error_t ret = 0;
#ifdef WIN32_LEAN_AND_MEAN
	CONDITION_VARIABLE thread_sync;
	CRITICAL_SECTION thread_mtx;
	HANDLE* chunk_threads = NULL;
	DWORD ThreadID;
#else
	pthread_cond_t thread_sync;
	pthread_mutex_t thread_mtx;
	pthread_t* chunk_threads = NULL;
#endif
	
	num_threads_to_use = -1;
	bool_t main_thread_ready = FALSE;
	
	InflateThreadObj thread_object;
	thread_object.mt_data_queue = mt_data_queue;
	thread_object.object = object;
	thread_object.err = 0;
	thread_object.main_thread_ready = &main_thread_ready;
	
	if(will_multithread == TRUE && object->num_elems > MIN_MT_ELEMS_THRESH)
	{

#ifdef WIN32_LEAN_AND_MEAN
		InitializeCriticalSection(&thread_mtx);
		InitializeConditionVariable(&thread_sync);
		
		thread_object.thread_sync = &thread_sync;
		thread_object.thread_mtx = &thread_mtx;
		
#else
		pthread_mutex_init(&thread_mtx, NULL);
		pthread_cond_init(&thread_sync, NULL);
		
		thread_object.thread_sync = &thread_sync;
		thread_object.thread_mtx = &thread_mtx;
#endif
		
		if(num_threads_user_def != -1)
		{
			num_threads_to_use = num_threads_user_def;
			num_threads_to_use = MAX(num_threads_to_use, 1);
		}
		else
		{
			//num_threads_to_use = (int)(-5.69E4*pow(object->num_elems, -0.7056) + 7.502);
			//num_threads_to_use = MIN(num_threads_to_use, num_avail_threads);
			num_threads_to_use = num_avail_threads;
			num_threads_to_use = MAX(num_threads_to_use, 1);
		}

#ifdef WIN32_LEAN_AND_MEAN
		chunk_threads = mxMalloc(num_threads_to_use*sizeof(HANDLE));
#ifdef NO_MEX
		if(chunk_threads == NULL)
		{
			sprintf(error_id, "getmatvar:mallocErrCTPCDWT");
			sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
			return 1;
		}
#endif
		for(int i = 0; i < num_threads_to_use; i++)
		{
			chunk_threads[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) doInflate_, (LPVOID)&thread_object, 0, &ThreadID);
		}
#else
		chunk_threads = mxMalloc(num_threads_to_use*sizeof(pthread_t));
#ifdef NO_MEX
		if(chunk_threads == NULL)
		{
			sprintf(error_id, "getmatvar:mallocErrCTPCDPT");
			sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
			return 1;
		}
#endif
		for(int i = 0; i < num_threads_to_use; i++)
		{
			pthread_create(&chunk_threads[i], NULL, doInflate_, (void*)&thread_object);
		}
#endif
	
	}
	
	TreeNode* root = mxMalloc(sizeof(TreeNode));
#ifdef NO_MEX
	if(root == NULL)
	{
		sprintf(error_id, "getmatvar:mallocErrRPCD");
		sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
		return 1;
	}
#endif
	root->address = object->data_address;
	root->node_type = NODETYPE_ROOT;
	
	data_page_buckets = mxMalloc(num_pages * sizeof(Queue));
#ifdef NO_MEX
	if(data_page_buckets == NULL)
	{
		sprintf(error_id, "getmatvar:mallocErrDPBPCD");
		sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
		return 1;
	}
#endif
	for(int i = 0; i < num_pages; i++)
	{
		if((data_page_buckets[i] = initQueue(free)) == NULL)
		{
			sprintf(error_id, "getmatvar:initQueueErr");
			sprintf(error_message, "Could not initialize the data_page_buckets[%d] queue",i);
			return 1;
		}
	}
	
	//fill the chunk tree
	ret = fillNode(root, object->chunked_info.num_chunked_dims);
	if(ret != 0)
	{
		error_flag = TRUE;
		sprintf(error_id, "getmatvar:internalInvalidNodeType");
		sprintf(error_message, "Invalid node type in fillNode()\n\n");
		return ret;
	}
	
	/*
	mt_mergeQueue(mt_data_queue, data_page_buckets, num_pages);
	for(int i = 0; i < num_pages; i++)
	{
		freeQueue(data_page_buckets[i]);
	}
	 */
	for(int i = 0; i < num_pages; i++)
	{
		mt_enqueue(mt_data_queue, data_page_buckets[i]);
	}
	mxFree(data_page_buckets);
	
	
	if(will_multithread == TRUE && object->num_elems > MIN_MT_ELEMS_THRESH)
	{
#ifdef WIN32_LEAN_AND_MEAN
		EnterCriticalSection(&thread_mtx);
		main_thread_ready = TRUE;
		WakeAllConditionVariable(&thread_sync);
		LeaveCriticalSection(&thread_mtx);
		WaitForMultipleObjects((DWORD)num_threads_to_use, chunk_threads, TRUE, INFINITE);
		DWORD exit_code = 0;
		for(int i = 0; i < num_threads_to_use; i++)
		{
			GetExitCodeThread(chunk_threads[i], &exit_code);
			if(exit_code != 0)
			{
				ret = 1;
			}
			CloseHandle(chunk_threads[i]);
		}
		mxFree(chunk_threads);
		DeleteCriticalSection(&thread_mtx);
		
#else
		pthread_mutex_lock(&thread_mtx);
		main_thread_ready = TRUE;
		pthread_cond_broadcast(&thread_sync);
		pthread_mutex_unlock(&thread_mtx);
		void* exit_code;
		for(int i = 0; i < num_threads_to_use; i++)
		{
			pthread_join(chunk_threads[i], &exit_code);
			if(*((error_t*)exit_code) != 0)
			{
				ret = 1;
			}
		}
		mxFree(chunk_threads);
		pthread_mutex_destroy(&thread_mtx);
		pthread_cond_destroy(&thread_sync);
#endif
	}
	else
	{
#ifdef WIN32_LEAN_AND_MEAN
		if(doInflate_((void*)&thread_object) != 0)
		{
			ret = 1;
		}
#else
		if(*((error_t*)doInflate_((void*)&thread_object)) != 0)
		{
			ret = 1;
		}
#endif
	}
	
	mt_freeQueue(mt_data_queue);
	freeTree(root);
	
	return ret;
}

#ifdef WIN32_LEAN_AND_MEAN
DWORD doInflate_(void* t)
#else
void* doInflate_(void* t)
#endif
{
	//make sure this is done after the recursive calls since we will run out of memory otherwise
	InflateThreadObj* thread_obj = (InflateThreadObj*)t;
	Data* object = thread_obj->object;
	MTQueue* mt_data_queue = thread_obj->mt_data_queue;
#ifdef WIN32_LEAN_AND_MEAN
	CRITICAL_SECTION* thread_mtx = thread_obj->thread_mtx;
	CONDITION_VARIABLE* thread_sync = thread_obj->thread_sync;
#else
	pthread_mutex_t* thread_mtx = thread_obj->thread_mtx;
	pthread_cond_t* thread_sync = thread_obj->thread_sync;
#endif
	bool_t* main_thread_ready = thread_obj->main_thread_ready;
	
	uint64_t these_num_chunked_elems = 0;
	uint64_t these_chunked_dims[HDF5_MAX_DIMS + 1] = {0};
	uint64_t these_index_updates[HDF5_MAX_DIMS] = {0};
	uint64_t these_chunked_updates[HDF5_MAX_DIMS] = {0};
	uint64_t chunk_pos[HDF5_MAX_DIMS + 1] = {0};
//	memset(these_chunked_dims, 0, sizeof(these_chunked_dims));
//	memset(these_index_updates, 0, sizeof(these_index_updates));
//	memset(these_chunked_updates, 0, sizeof(these_chunked_updates));
//	memset(chunk_pos, 0, sizeof(chunk_pos));
	
	const size_t max_est_decomp_size = object->chunked_info.num_chunked_elems*object->elem_size; /* make sure this is non-null */
	
	byte* decompressed_data_buffer = mxMalloc(max_est_decomp_size);
#ifdef NO_MEX
	if(decompressed_data_buffer == NULL)
	{
		memset(error_id, 0, sizeof(error_id));
		memset(error_message, 0, sizeof(error_id));
		sprintf(error_id, "getmatvar:mallocErrDDBDI");
		sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
#ifdef WIN32_LEAN_AND_MEAN
		return 1;
#else
		thread_obj->err = 1;
		return (void*)&thread_obj->err;
#endif
	}
#endif
	struct libdeflate_decompressor* ldd = libdeflate_alloc_decompressor();
	//uint64_t* index_map = mxMalloc(object->chunked_info.num_chunked_elems*sizeof(uint64_t));
	//uint64_t* db_index_sequence = mxMalloc(object->chunked_info.num_chunked_elems*sizeof(uint64_t));
	
	Key* data_key;
	TreeNode* data_node;
	
	if(will_multithread == TRUE && object->num_elems > MIN_MT_ELEMS_THRESH)
	{
#ifdef WIN32_LEAN_AND_MEAN
		EnterCriticalSection(thread_mtx);
		while(*main_thread_ready != TRUE)
		{
			SleepConditionVariableCS (thread_sync, thread_mtx, INFINITE);
		}
		LeaveCriticalSection(thread_mtx);
#else
		pthread_mutex_lock(thread_mtx);
		while(*main_thread_ready != TRUE)
		{
			pthread_cond_wait(thread_sync, thread_mtx);
		}
		pthread_mutex_unlock(thread_mtx);
#endif
	}
	
	while(mt_data_queue->length > 0)
	{
		
		//each thread gets its own page to work on, so the only contentions possible are here
		Queue* data_page_bucket = (Queue*)mt_dequeue(mt_data_queue);
		
		if(data_page_bucket == NULL)
		{
			//in case it gets dequeued after the loop check but before the lock
			break;
		}
		
		while(data_page_bucket->length > 0)
		{
			DataPair* dp = (DataPair*)dequeue(data_page_bucket);

//		if(dp == NULL)
//		{
//			//in case it gets dequeued after the loop check but before the lock
//			break;
//		}
			
			data_key = dp->data_key;
			data_node = dp->data_node;
			
			const uint64_t chunk_start_index = findArrayPosition(data_key->chunk_start, object->dims, object->num_dims);
			//const byte* data_pointer = st_navigateTo(node->children[i]->address, node->keys[i].size);
			const byte* data_pointer = mt_navigateTo(data_node->address, 0);
			thread_obj->err = libdeflate_zlib_decompress(ldd, data_pointer, data_key->chunk_size, decompressed_data_buffer, max_est_decomp_size, NULL);
			//st_releasePages(node->children[i]->address, node->keys[i].size);
			mt_releasePages(data_node->address, 0);
			switch(thread_obj->err)
			{
				case LIBDEFLATE_BAD_DATA:
					memset(error_id, 0, sizeof(error_id));
					memset(error_message, 0, sizeof(error_id));
					sprintf(error_id, "getmatvar:libdeflateBadData");
					sprintf(error_message, "Failed to decompress data which was either invalid, corrupt or otherwise unsupported.\n\n");
					libdeflate_free_decompressor(ldd);
					mxFree(decompressed_data_buffer);
#ifdef WIN32_LEAN_AND_MEAN
					return 1;
#else
				thread_obj->err = 1;
				return (void*)&thread_obj->err;
#endif
				case LIBDEFLATE_SHORT_OUTPUT:
					memset(error_id, 0, sizeof(error_id));
					memset(error_message, 0, sizeof(error_id));
					sprintf(error_id, "getmatvar:libdeflateShortOutput");
					sprintf(error_message, "Failed to decompress because a NULL "
							"'actual_out_nbytes_ret' was provided, but the data would have"
							" decompressed to fewer than 'out_nbytes_avail' bytes.\n\n");
					libdeflate_free_decompressor(ldd);
					mxFree(decompressed_data_buffer);
#ifdef WIN32_LEAN_AND_MEAN
					return 1;
#else
				thread_obj->err = 1;
				return (void*)&thread_obj->err;
#endif
				case LIBDEFLATE_INSUFFICIENT_SPACE:
					memset(error_id, 0, sizeof(error_id));
					memset(error_message, 0, sizeof(error_id));
					sprintf(error_id, "getmatvar:libdeflateInsufficientSpace");
					sprintf(error_message, "Decompression failed because the output buffer was not large enough");
					libdeflate_free_decompressor(ldd);
					mxFree(decompressed_data_buffer);
#ifdef WIN32_LEAN_AND_MEAN
					return 1;
#else
				thread_obj->err = 1;
				return (void*)&thread_obj->err;
#endif
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
				//if the chunk collides with the edge, make sure the dimensions of the chunk respect that
				//these_chunked_dims[j] = object->chunked_info.chunked_dims[j] - MAX((uint64_t)(data_key->chunk_start[j] + object->chunked_info.chunked_dims[j] - object->dims[j]), 0);
				these_chunked_dims[j] = object->chunked_info.chunked_dims[j]
								    - (data_key->chunk_start[j] + object->chunked_info.chunked_dims[j] > object->dims[j]? data_key->chunk_start[j] + object->chunked_info.chunked_dims[j]
																										- object->dims[j] : 0
								    );
				these_index_updates[j] = object->chunked_info.chunk_update[j];
				these_chunked_updates[j] = 0;
			}
			
			for(int j = 0; j < object->num_dims; j++)
			{
				if(unlikely(these_chunked_dims[j] != object->chunked_info.chunked_dims[j]))
				{
					makeChunkedUpdates(these_index_updates, these_chunked_dims, object->dims, object->num_dims);
					makeChunkedUpdates(these_chunked_updates, these_chunked_dims, object->chunked_info.chunked_dims, object->num_dims);
					these_num_chunked_elems = 1;
					for(int k = 0; k < object->chunked_info.num_chunked_dims; k++)
					{
						these_num_chunked_elems *= these_chunked_dims[k];
					}
					break;
				}
			}
			
			//copy over data
			//memset(index_map, 0xFF, object->chunked_info.num_chunked_elems*sizeof(uint64_t));
			memset(chunk_pos, 0, sizeof(chunk_pos));
			uint8_t curr_max_dim = 2;
			uint64_t db_pos = 0, num_used = 0;
			for(uint64_t index = chunk_start_index, anchor = 0; index < object->num_elems && num_used < these_num_chunked_elems; anchor = db_pos)
			{
//			for(; index < object->num_elems && db_pos < anchor + these_chunked_dims[0]; db_pos++, index++, num_used++)
//			{
//				index_map[db_pos] = index;
//				db_index_sequence[num_used] = db_pos;
//			}
				placeData(object, decompressed_data_buffer, index, anchor, these_chunked_dims[0], object->elem_size, object->byte_order);
				db_pos += these_chunked_dims[0];
				index += these_chunked_dims[0];
				
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
				index += these_index_updates[use_update];
				db_pos += these_chunked_updates[use_update];
			}
			
			//placeDataWithIndexMap(object, decompressed_data_buffer, num_used, object->elem_size, object->byte_order, index_map, db_index_sequence);
		}
	}
	
	libdeflate_free_decompressor(ldd);
	mxFree(decompressed_data_buffer);
	//mxFree(index_map);
	//mxFree(db_index_sequence);
#ifdef WIN32_LEAN_AND_MEAN
	return 0;
#else
	return NULL;
#endif

}


uint64_t findArrayPosition(const uint64_t* coordinates, const uint64_t* array_dims, uint8_t num_chunked_dims)
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


error_t fillNode(TreeNode* node, uint8_t num_chunked_dims)
{
	
	if(node->address == UNDEF_ADDR)
	{
		//this is an ending sentinel for the sibling linked list
		node->entries_used = 0;
		node->keys = NULL;
		node->children = NULL;
		//node->node_level = -2;
		return 0;
	}
	

	mapObject* tree_map_obj = st_navigateTo(node->address, 8);
	byte* tree_pointer = tree_map_obj->address_ptr;
	
	if(memcmp((char*)tree_pointer, "TREE", 4*sizeof(char)) != 0)
	{
		node->entries_used = 0;
		node->keys = NULL;
		node->children = NULL;
		//node->node_level = -1;
		if(memcmp((char*)tree_pointer, "SNOD", 4*sizeof(char)) == 0)
		{
			//(from group node)
			node->node_type = SYMBOL;
		}
		else
		{
			node->node_type = RAWDATA;
		}
		st_releasePages(tree_map_obj);
		return 0;
	}
	
	node->node_type = (NodeType)getBytesAsNumber(tree_pointer + 4, 1, META_DATA_BYTE_ORDER);
	//node->node_level = (uint16_t)getBytesAsNumber(tree_pointer + 5, 1, META_DATA_BYTE_ORDER);
	node->entries_used = (uint16_t)getBytesAsNumber(tree_pointer + 6, 2, META_DATA_BYTE_ORDER);
	
	node->keys = mxMalloc((node->entries_used + 1)*sizeof(Key*));
#ifdef NO_MEX
	if(node->keys == NULL)
	{
		sprintf(error_id, "getmatvar:mallocErrKFN");
		sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
		return 1;
	}
#endif
	node->children = mxMalloc(node->entries_used*sizeof(TreeNode*));
#ifdef NO_MEX
	if(node->children == NULL)
	{
		mxFree(node->keys);
		sprintf(error_id, "getmatvar:mallocErrCFN");
		sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
		return 1;
	}
#endif
	
	uint64_t key_size;
	//uint64_t bytes_needed;
	
	if(node->node_type != CHUNK)
	{
		mxFree(node->keys);
		mxFree(node->children);
		sprintf(error_id, "getmatvar:wrongNodeType");
		sprintf(error_message, "A group node was found in the chunked data tree.\n\n");
		return 1;
	}
	
	
	//1-4: Size of chunk in bytes
	//4-8 Filter mask
	//(Dimensionality+1)*16 bytes
	key_size = 4 + 4 + (num_chunked_dims + 1)*8;
	
	st_releasePages(tree_map_obj);
	uint64_t key_address = node->address + 8 + 2*s_block.size_of_offsets;
	//bytes_needed = key_size + s_block.size_of_offsets;
	uint64_t total_bytes_needed = (node->entries_used + 1)*key_size + node->entries_used*s_block.size_of_offsets;
	mapObject* key_map_obj = st_navigateTo(key_address, total_bytes_needed);
	byte* key_pointer = key_map_obj->address_ptr;
	node->keys[0] = mxMalloc(sizeof(Key));
#ifdef NO_MEX
	if(node->keys[0] == NULL)
	{
		mxFree(node->keys);
		mxFree(node->children);
		sprintf(error_id, "getmatvar:mallocErrK0FN");
		sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
		return 1;
	}
#endif
	node->keys[0]->chunk_size = (uint32_t)getBytesAsNumber(key_pointer, 4, META_DATA_BYTE_ORDER);
	//node->keys[0]->filter_mask = (uint32_t)getBytesAsNumber(key_pointer + 4, 4, META_DATA_BYTE_ORDER);
	node->keys[0]->chunk_start = mxMalloc((num_chunked_dims + 1)*sizeof(uint64_t));
	for(int j = 0; j < num_chunked_dims; j++)
	{
		node->keys[0]->chunk_start[num_chunked_dims - 1 - j] = (uint64_t)getBytesAsNumber(key_pointer + 8 + j*8, 8, META_DATA_BYTE_ORDER);
	}
	node->keys[0]->chunk_start[num_chunked_dims] = 0;
	
	for(int i = 0; i < node->entries_used; i++)
	{
		node->children[i] = mxMalloc(sizeof(TreeNode));
#ifdef NO_MEX
		if(node->children[i] == NULL)
		{
			for(int j = 0; j < i; j++)
			{
				mxFree(node->keys[j]->chunk_start);
				mxFree(node->keys[j]);
			}
			mxFree(node->keys);
			for(int j = 0; j < i - 1; j++)
			{
				freeTree(node->children[j]);
			}
			mxFree(node->children);
			sprintf(error_id, "getmatvar:mallocErrC%dFN",i);
			sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
			return 1;
		}
#endif
		node->children[i]->address = getBytesAsNumber(key_pointer + key_size, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
		node->children[i]->node_type = NODETYPE_UNDEFINED;
		
		if(fillNode(node->children[i], num_chunked_dims) != 0)
		{
			return 1;
		}
		
		size_t page_index = node->children[i]->address/alloc_gran;
		if(node->children[i]->node_type == RAWDATA)
		{
			page_objects[page_index].total_num_mappings++;
			page_objects[page_index].max_map_size = MAX(page_objects[page_index].max_map_size, (node->children[i]->address % alloc_gran) + node->keys[i]->chunk_size);
			DataPair* dp = mxMalloc(sizeof(DataPair));
#ifdef NO_MEX
			if(dp == NULL)
			{
				for(int j = 0; j < i; j++)
				{
					mxFree(node->keys[j]->chunk_start);
					mxFree(node->keys[j]);
				}
				mxFree(node->keys);
				for(int j = 0; j < i; j++)
				{
					freeTree(node->children[j]);
				}
				mxFree(node->children);
				sprintf(error_id, "getmatvar:mallocErrDPFN");
				sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
				return 1;
			}
#endif
			dp->data_key = node->keys[i];
			dp->data_node = node->children[i];
			enqueue(data_page_buckets[page_index], dp);
		}
		
		key_address += key_size + s_block.size_of_offsets;
		key_pointer += key_size + s_block.size_of_offsets;
		
		node->keys[i + 1] = mxMalloc(sizeof(Key));
#ifdef NO_MEX
		if(node->keys[i + 1] == NULL)
		{
			for(int j = 0; j < i; j++)
			{
				mxFree(node->keys[j]->chunk_start);
				mxFree(node->keys[j]);
			}
			mxFree(node->keys);
			for(int j = 0; j < i; j++)
			{
				freeTree(node->children[j]);
			}
			mxFree(node->children);
			sprintf(error_id, "getmatvar:mallocErrK%dFN",i+1);
			sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
			return 1;
		}
#endif
		node->keys[i + 1]->chunk_size = (uint32_t)getBytesAsNumber(key_pointer, 4, META_DATA_BYTE_ORDER);
		//node->keys[i + 1]->filter_mask = (uint32_t)getBytesAsNumber(key_pointer + 4, 4, META_DATA_BYTE_ORDER);
		node->keys[i + 1]->chunk_start = mxMalloc((num_chunked_dims + 1)*sizeof(uint64_t));
#ifdef NO_MEX
		if(node->keys[i + 1]->chunk_start == NULL)
		{
			for(int j = 0; j < i; j++)
			{
				mxFree(node->keys[j]->chunk_start);
				mxFree(node->keys[j]);
			}
			mxFree(node->keys[i+1]);
			mxFree(node->keys);
			for(int j = 0; j < i; j++)
			{
				freeTree(node->children[j]);
			}
			mxFree(node->children);
			sprintf(error_id, "getmatvar:mallocErrK%dFN",i+1);
			sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
			return 1;
		}
#endif
		for(int j = 0; j < num_chunked_dims; j++)
		{
			node->keys[i + 1]->chunk_start[num_chunked_dims - j - 1] = (uint64_t)getBytesAsNumber(key_pointer + 8 + j*8, 8, META_DATA_BYTE_ORDER);
		}
		node->keys[i + 1]->chunk_start[num_chunked_dims] = 0;
		
		
	}
	
	st_releasePages(key_map_obj);
	return 0;
	
}


void makeChunkedUpdates(uint64_t* chunk_update, const uint64_t* chunked_dims, const uint64_t* dims, uint8_t num_dims)
{
	uint64_t cu, du;
	for(int i = 0; i < num_dims; i++)
	{
		cu = 0;
		du = 1;
		for(int k = 0; k < i + 1; k++)
		{
			cu += (chunked_dims[k] - 1)*du;
			du *= dims[k];
		}
		chunk_update[i] = du - cu - 1;
	}
}


void freeTree(void* n)
{
	TreeNode* node = (TreeNode*)n;
	if(node != NULL)
	{
		
		if(node->keys != NULL)
		{
			for(int i = 0; i < node->entries_used + 1; i++)
			{
				mxFree(node->keys[i]->chunk_start);
				mxFree(node->keys[i]);
			}
			mxFree(node->keys);
			node->keys = NULL;
		}
		
		if(node->children != NULL)
		{
			for(int i = 0; i < node->entries_used; i++)
			{
				freeTree(node->children[i]);
			}
			mxFree(node->children);
			node->children = NULL;
		}
		
		mxFree(node);
		
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


/*
#ifdef WIN32_LEAN_AND_MEAN
DWORD mt_fillNode(void* fno)
#else
void* mt_fillNode(void* fno)
#endif
{
	TreeNode* node = ((FillNodeObj*)fno)->node;
	uint8_t num_chunked_dims = ((FillNodeObj*)fno)->num_chunked_dims;
	
	bool_t is_root = FALSE;
#ifdef WIN32_LEAN_AND_MEAN
	HANDLE* tree_threads = NULL;
	DWORD ThreadID;
#else
	pthread_t* tree_threads = NULL;
	void* status;
#endif
	if(node->node_type == NODETYPE_ROOT)
	{
		is_root = TRUE;
	}
	
	if(node->address == UNDEF_ADDR)
	{
		//this is an ending sentinel for the sibling linked list
		node->entries_used = 0;
		node->keys = NULL;
		node->children = NULL;
		//node->node_level = -2;
#ifdef WIN32_LEAN_AND_MEAN
		return 0;
#else
		return NULL;
#endif
	}
	
	//1-4: Size of chunk in bytes
	//4-8 Filter mask
	//(Dimensionality+1)*8 bytes
	uint64_t key_size = (uint64_t)4 + 4 + (num_chunked_dims + 1)*8;
	uint64_t bytes_needed = key_size + s_block.size_of_offsets;
	
	//maps the header + the first key/address pair
	byte* tree_pointer = mt_navigateTo(node->address, 8 + 2*s_block.size_of_offsets + bytes_needed);
	
	if(memcmp((char*)tree_pointer, "TREE", 4*sizeof(char)) != 0)
	{
		node->entries_used = 0;
		node->keys = NULL;
		node->children = NULL;
		//node->node_level = -1;
		if(memcmp((char*)tree_pointer, "SNOD", 4*sizeof(char)) == 0)
		{
			//(from group node)
			node->node_type = SYMBOL;
		}
		else
		{
			node->node_type = RAWDATA;
		}
		mt_releasePages(node->address, 8 + 2*s_block.size_of_offsets + bytes_needed);
#ifdef WIN32_LEAN_AND_MEAN
		return 0;
#else
		return NULL;
#endif
	}
	
	node->node_type = (NodeType)getBytesAsNumber(tree_pointer + 4, 1, META_DATA_BYTE_ORDER);
	//node->node_level = (uint16_t)getBytesAsNumber(tree_pointer + 5, 1, META_DATA_BYTE_ORDER);
	node->entries_used = (uint16_t)getBytesAsNumber(tree_pointer + 6, 2, META_DATA_BYTE_ORDER);
	
	if(is_root == TRUE)
	{
#ifdef WIN32_LEAN_AND_MEAN
		tree_threads = mxMalloc(MIN(node->entries_used, num_threads_to_use)*sizeof(HANDLE));
#else
		tree_threads = mxMalloc(MIN(node->entries_used, num_threads_to_use)*sizeof(pthread_t));
#endif
	}
	
	node->keys = mxMalloc((node->entries_used + 1)*sizeof(Key*));
	node->children = mxMalloc(node->entries_used*sizeof(TreeNode*));
	
	if(node->node_type != CHUNK)
	{
		error_flag = TRUE;
		sprintf(error_id, "getmatvar:wrongNodeType");
		sprintf(error_message, "A group node was found in the chunked data tree.\n\n");
		mt_releasePages(node->address, 8 + 2*s_block.size_of_offsets + bytes_needed);
#ifdef WIN32_LEAN_AND_MEAN
		return 1;
#else
		((FillNodeObj*)fno)->err = 1;
		return &((FillNodeObj*)fno)->err;
#endif
	}
	
	mt_releasePages(node->address, 8 + 2*s_block.size_of_offsets + bytes_needed);
	uint64_t key_address = node->address + 8 + 2*s_block.size_of_offsets;
	//uint64_t total_bytes_needed = (node->entries_used + 1)*key_size + node->entries_used*s_block.size_of_offsets;
	//this should reuse the previous mapping
	byte* key_pointer = mt_navigateTo(key_address, bytes_needed);
	node->keys[0] = mxMalloc(sizeof(Key));
	node->keys[0]->chunk_size = (uint32_t)getBytesAsNumber(key_pointer, 4, META_DATA_BYTE_ORDER);
	//node->keys[0]->filter_mask = (uint32_t)getBytesAsNumber(key_pointer + 4, 4, META_DATA_BYTE_ORDER);
	node->keys[0]->chunk_start = mxMalloc((num_chunked_dims + 1)*sizeof(uint64_t));
	for(int j = 0; j < num_chunked_dims; j++)
	{
		//skip 8 byte bitfield and get the dims
		node->keys[0]->chunk_start[num_chunked_dims - 1 - j] = (uint64_t)getBytesAsNumber(key_pointer + 8 + j*8, 8, META_DATA_BYTE_ORDER);
	}
	node->keys[0]->chunk_start[num_chunked_dims] = 0;
	
	int num_threads_used = 0;
	FillNodeObj** fn_sub_objs = mxMalloc(node->entries_used*sizeof(FillNodeObj*));
	for(int i = 0; i < node->entries_used; i++)
	{
		node->children[i] = mxMalloc(sizeof(TreeNode));
		node->children[i]->address = getBytesAsNumber(key_pointer + key_size, s_block.size_of_offsets, META_DATA_BYTE_ORDER) + s_block.base_address;
		node->children[i]->node_type = NODETYPE_UNDEFINED;
		
		mt_releasePages(key_address, bytes_needed);
		
		fn_sub_objs[i] = mxMalloc(sizeof(FillNodeObj));
		fn_sub_objs[i]->node = node->children[i];
		fn_sub_objs[i]->num_chunked_dims = num_chunked_dims;
		
		if(is_root == TRUE && num_threads_used < num_threads_to_use)
		{
#ifdef WIN32_LEAN_AND_MEAN
			tree_threads[num_threads_used] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) mt_fillNode, (LPVOID)fn_sub_objs[i], 0, &ThreadID);
#else
			pthread_create(&tree_threads[num_threads_used], NULL, mt_fillNode, (void*)fn_sub_objs[i]);
#endif
			num_threads_used++;
		}
		else
		{
#ifdef WIN32_LEAN_AND_MEAN
			if(mt_fillNode(fn_sub_objs[i]) != 0)
			{
				return 1;
			}
#else
			mt_fillNode(fn_sub_objs[i]);
#endif
		}
		
		size_t page_index = node->children[i]->address/alloc_gran;
		if(node->children[i]->node_type == RAWDATA)
		{
			page_objects[page_index].total_num_mappings++;
			page_objects[page_index].max_map_size = MAX(page_objects[page_index].max_map_size, (node->children[i]->address % alloc_gran) + node->keys[i]->chunk_size);
			DataPair* dp = mxMalloc(sizeof(DataPair));
			dp->data_key = node->keys[i];
			dp->data_node = node->children[i];
			mt_enqueue(mt_data_page_buckets[page_index], dp);
		}
		
		key_address += bytes_needed;
		key_pointer = mt_navigateTo(key_address, bytes_needed);
		
		node->keys[i + 1] = mxMalloc(sizeof(Key));
		node->keys[i + 1]->chunk_size = (uint32_t)getBytesAsNumber(key_pointer, 4, META_DATA_BYTE_ORDER);
		//node->keys[i + 1]->filter_mask = (uint32_t)getBytesAsNumber(key_pointer + 4, 4, META_DATA_BYTE_ORDER);
		node->keys[i + 1]->chunk_start = mxMalloc((num_chunked_dims + 1)*sizeof(uint64_t));
		for(int j = 0; j < num_chunked_dims; j++)
		{
			node->keys[i + 1]->chunk_start[num_chunked_dims - j - 1] = (uint64_t)getBytesAsNumber(key_pointer + 8 + j*8, 8, META_DATA_BYTE_ORDER);
		}
		node->keys[i + 1]->chunk_start[num_chunked_dims] = 0;
	}

#ifdef WIN32_LEAN_AND_MEAN
	DWORD ret = 0;
	if(is_root == TRUE)
	{
		WaitForMultipleObjects((DWORD) num_threads_used, tree_threads, TRUE, INFINITE);
		DWORD exit_code = 0;
		for(int i = 0; i < num_threads_used; i++)
		{
			GetExitCodeThread(tree_threads[i], &exit_code);
			if(exit_code != 0)
			{
				ret = 1;
			}
			CloseHandle(tree_threads[i]);
		}
		mxFree(tree_threads);
	}
#else
	if(is_root== TRUE)
	{
		for(int i = 0; i < num_threads_used; i++)
		{
			pthread_join(tree_threads[i], &status);
		}
		mxFree(tree_threads);
	}
#endif
	
	for(int i = 0; i < node->entries_used; i++)
	{
		mxFree(fn_sub_objs[i]);
	}
	mxFree(fn_sub_objs);
	
	mt_releasePages(key_address, bytes_needed);
#ifdef WIN32_LEAN_AND_MEAN
	return ret;
#else
	return NULL;
#endif

}
*/