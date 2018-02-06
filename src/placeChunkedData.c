#include "headers/placeChunkedData.h"


int num_threads_to_use;
bool_t main_thread_ready;
#ifdef WIN32_LEAN_AND_MEAN
HANDLE thread_sync;
#else
pthread_cond_t thread_sync;
pthread_mutex_t thread_mtx;
#endif

// In case matlab encounters an error and needs to close
// but touching the libdeflate decompressors with mxMalloc
// seems to crash matlab, so leave this out for now
static void freeThreadBuffers(void);
//static void nothingFunc(void);

error_t getChunkedData(Data* object)
{
	
	initializePageObjects();
	
	mt_data_queue = mt_initQueue(freeDP);
//	mt_data_queue = mt_initQueue((void*) freeQueue);
	error_t ret = 0;
#ifdef WIN32_LEAN_AND_MEAN
	HANDLE* chunk_threads = NULL;
	DWORD ThreadID;
#else
	pthread_t* chunk_threads = NULL;
#endif
	data_buffers = mt_initQueue(free);
	libdeflate_decomps = mt_initQueue((void*)libdeflate_free_decompressor);
//#ifndef NO_MEX
	//mexAtExit(freeThreadBuffers);
//#endif
	
	num_threads_to_use = -1;
	main_thread_ready = FALSE;
	
	InflateThreadObj thread_object;
	thread_object.mt_data_queue = mt_data_queue;
	thread_object.object = object;
	thread_object.err = 0;
	
	if(((will_multithread == TRUE) & (object->num_elems > MIN_MT_ELEMS_THRESH)) == TRUE)
	{

#ifdef WIN32_LEAN_AND_MEAN
		thread_sync = CreateEvent(NULL, TRUE, FALSE, NULL);
#else
		pthread_cond_init(&thread_sync, NULL);
		pthread_mutex_init(&thread_mtx, NULL);
#endif
		
		if(num_threads_user_def != -1)
		{
			num_threads_to_use = num_threads_user_def;
		}
		else
		{
			num_threads_to_use = num_avail_threads;
		}
		num_threads_to_use = MAX(num_threads_to_use, 1);

#ifdef WIN32_LEAN_AND_MEAN
		chunk_threads = mxMalloc(num_threads_to_use * sizeof(HANDLE));
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
			chunk_threads[i] = CreateThread(NULL, 0, doInflate_, &thread_object, 0, &ThreadID);
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
	
	TreeNode root;
	root.address = object->data_address;
	root.node_type = NODETYPE_ROOT;
	
	data_page_buckets = malloc(num_pages * sizeof(Queue));
#ifdef NO_MEX
	if(data_page_buckets == NULL)
	{
		sprintf(error_id, "getmatvar:mallocErrDPBPCD");
		sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
		return 1;
	}
#endif
	for(size_t i = 0; i < num_pages; i++)
	{
		if((data_page_buckets[i] = initQueue(NULL)) == NULL)
//		if((data_page_buckets[i] = initQueue(freeDP)) == NULL)
		{
			sprintf(error_id, "getmatvar:initQueueErr");
			sprintf(error_message, "Could not initialize the data_page_buckets[%d] queue", (int) i);
			return 1;
		}
	}
	
	//fill the chunk tree
	ret = fillNode(&root, object->chunked_info.num_chunked_dims);
	if(ret != 0)
	{
		sprintf(error_id, "getmatvar:internalInvalidNodeType");
		sprintf(error_message, "Invalid node type in fillNode()\n\n");
		return ret;
	}

	mt_mergeQueue(mt_data_queue, data_page_buckets, num_pages);
	for(int i = 0; i < num_pages; i++)
	{
		freeQueue(data_page_buckets[i]);
	}
	free(data_page_buckets);
	data_page_buckets = NULL;
	
//	for(int i = 0; i < num_pages; i++)
//	{
//		mt_enqueue(mt_data_queue, data_page_buckets[i]);
//	}
//	free(data_page_buckets);
	
	
	if(will_multithread == TRUE && object->num_elems > MIN_MT_ELEMS_THRESH)
	{
#ifdef WIN32_LEAN_AND_MEAN
		main_thread_ready = TRUE;
		if(SetEvent(thread_sync) == 0)
		{
			sprintf(error_id, "getmatvar:internalError");
			sprintf(error_message, "Critical error: failed to set the thread_sync event.\n\n");
			return 1;
		}
		WaitForMultipleObjects((DWORD) num_threads_to_use, chunk_threads, TRUE, INFINITE);
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
		CloseHandle(thread_sync);

#else
		main_thread_ready = TRUE;
		pthread_mutex_lock(&thread_mtx);
		pthread_cond_broadcast(&thread_sync);
		pthread_mutex_unlock(&thread_mtx);
		error_t* exit_code;
		for(int i = 0; i < num_threads_to_use; i++)
		{
			pthread_join(chunk_threads[i], (void*)&exit_code);
			if(*exit_code != 0)
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
		if(doInflate_((void*) &thread_object) != 0)
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

//#ifndef NO_MEX
//	mexAtExit(nothingFunc);
//#endif
	freeThreadBuffers();
	mt_freeQueue(mt_data_queue);
	mt_data_queue = NULL;
	
	return ret;
}


#ifdef WIN32_LEAN_AND_MEAN


DWORD doInflate_(void* t)
#else
void* doInflate_(void* t)
#endif
{
	//make sure this is done after the recursive calls since we will run out of memory otherwise
	InflateThreadObj* thread_obj = (InflateThreadObj*) t;
	Data* object = thread_obj->object;
	MTQueue* mt_data_queue = thread_obj->mt_data_queue;
	
	index_t these_num_chunked_elems = 0;
	index_t these_chunked_dims[HDF5_MAX_DIMS + 1] = {0};
	index_t these_index_updates[HDF5_MAX_DIMS] = {0};
	index_t these_chunked_updates[HDF5_MAX_DIMS] = {0};
	index_t chunk_pos[HDF5_MAX_DIMS + 1] = {0};
//	memset(these_chunked_dims, 0, sizeof(these_chunked_dims));
//	memset(these_index_updates, 0, sizeof(these_index_updates));
//	memset(these_chunked_updates, 0, sizeof(these_chunked_updates));
//	memset(chunk_pos, 0, sizeof(chunk_pos));
	
	const size_t max_est_decomp_size = (size_t) object->chunked_info.num_chunked_elems * object->elem_size;
	byte* decompressed_data_buffer = malloc(max_est_decomp_size);
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
	struct libdeflate_decompressor* ldd = libdeflate_alloc_decompressor();
	mt_enqueue(data_buffers, decompressed_data_buffer);
	mt_enqueue(libdeflate_decomps, ldd);
	
	address_t data_address;
	uint32_t chunk_size;
	index_t* chunk_start;
	
	if(will_multithread == TRUE && object->num_elems > MIN_MT_ELEMS_THRESH)
	{
#ifdef WIN32_LEAN_AND_MEAN
		//this condition isn't strictly necessary on Windows
		while(main_thread_ready != TRUE)
		{
			DWORD ret = WaitForSingleObject(thread_sync, INFINITE);
			if(ret != WAIT_OBJECT_0)
			{
				memset(error_id, 0, sizeof(error_id));
				memset(error_message, 0, sizeof(error_message));
				sprintf(error_id, "getmatvar:internalError");
				sprintf(error_message, "The thread wait returned unexpectedly.\n\n");
//				free(decompressed_data_buffer);
//				libdeflate_free_decompressor(ldd);
				return 1;
			}
		}
#else
		pthread_mutex_lock(&thread_mtx);
		//make sure that the main thread didn't finish parsing the chunk tree before waiting
		while(main_thread_ready != TRUE)
		{
			pthread_cond_wait(&thread_sync, &thread_mtx);
		}
		pthread_mutex_unlock(&thread_mtx);
#endif
	}
	
	
	while(mt_data_queue->length > 0)
	{
		
		DataPair* dp = (DataPair*)mt_dequeue(mt_data_queue);
//		Queue* data_page = (Queue*) mt_dequeue(mt_data_queue);


		if(dp == NULL)
		{
			//in case it gets dequeued after the loop check but before the lock
			break;
		}
		
//		if(data_page == NULL)
//		{
//			break;
//		}
		
//		while(data_page->length > 0)
//		{
			
//			DataPair* dp = (DataPair*) dequeue(data_page);
			
			data_address = dp->address;
			chunk_size = dp->chunk_size;
			chunk_start = dp->chunk_start;
			
			const index_t chunk_start_index = findArrayPosition(chunk_start, object->dims, object->num_dims);
			const byte* data_pointer = mt_navigateTo(data_address, 0);
			thread_obj->err = libdeflate_zlib_decompress(ldd, data_pointer, chunk_size, decompressed_data_buffer,
												max_est_decomp_size, NULL);
			mt_releasePages(data_address, 0);
			switch(thread_obj->err)
			{
				case LIBDEFLATE_BAD_DATA:
					memset(error_id, 0, sizeof(error_id));
					memset(error_message, 0, sizeof(error_message));
					sprintf(error_id, "getmatvar:libdeflateBadData");
					sprintf(error_message,
						   "Failed to decompress data which was either invalid, corrupt or otherwise unsupported.\n\n");
//					free(decompressed_data_buffer);
//					libdeflate_free_decompressor(ldd);
#ifdef WIN32_LEAN_AND_MEAN
					return 1;
#else
				thread_obj->err = 1;
				return (void*)&thread_obj->err;
#endif
				case LIBDEFLATE_SHORT_OUTPUT:
					memset(error_id, 0, sizeof(error_id));
					memset(error_message, 0, sizeof(error_message));
					sprintf(error_id, "getmatvar:libdeflateShortOutput");
					sprintf(error_message, "Failed to decompress because a NULL "
							"'actual_out_nbytes_ret' was provided, but the data would have"
							" decompressed to fewer than 'out_nbytes_avail' bytes.\n\n");
//					free(decompressed_data_buffer);
//					libdeflate_free_decompressor(ldd);
#ifdef WIN32_LEAN_AND_MEAN
					return 1;
#else
				thread_obj->err = 1;
				return (void*)&thread_obj->err;
#endif
				case LIBDEFLATE_INSUFFICIENT_SPACE:
					memset(error_id, 0, sizeof(error_id));
					memset(error_message, 0, sizeof(error_message));
					sprintf(error_id, "getmatvar:libdeflateInsufficientSpace");
					sprintf(error_message, "Decompression failed because the output buffer was not large enough");
//					free(decompressed_data_buffer);
//					libdeflate_free_decompressor(ldd);
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
				these_chunked_dims[j] = object->chunked_info.chunked_dims[j]
								    - (chunk_start[j] + object->chunked_info.chunked_dims[j] > object->dims[j] ?
									  chunk_start[j] + object->chunked_info.chunked_dims[j] - object->dims[j]
																							    : 0
								    );
				these_index_updates[j] = object->chunked_info.chunk_update[j];
				these_chunked_updates[j] = 0;
			}
			
			for(int j = 0; j < object->num_dims; j++)
			{
				if(these_chunked_dims[j] != object->chunked_info.chunked_dims[j])
				{
					makeChunkedUpdates(these_index_updates, these_chunked_dims, object->dims, object->num_dims);
					makeChunkedUpdates(these_chunked_updates, these_chunked_dims,
								    object->chunked_info.chunked_dims,
								    object->num_dims);
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
			index_t db_pos = 0, num_used = 0;
			for(index_t index = chunk_start_index, anchor = 0;
			    index < object->num_elems && num_used < these_num_chunked_elems; anchor = db_pos)
			{
				placeData(object, decompressed_data_buffer, index, anchor, these_chunked_dims[0],
						(size_t) object->elem_size, object->byte_order);
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
						curr_max_dim = curr_max_dim <= j + 1 ? curr_max_dim + (uint8_t) 1 : curr_max_dim;
						use_update++;
					}
				}
				index += these_index_updates[use_update];
				db_pos += these_chunked_updates[use_update];
			}
		}
//	}
	
	
//	free(decompressed_data_buffer);
//	libdeflate_free_decompressor(ldd);

#ifdef WIN32_LEAN_AND_MEAN
	return 0;
#else
	thread_obj->err = 0;
	return (void*)&thread_obj->err;
#endif

}


index_t findArrayPosition(const index_t* coordinates, const index_t* array_dims, uint8_t num_chunked_dims)
{
	index_t array_pos = 0;
	for(int i = 0; i < num_chunked_dims; i++)
	{
		//[18,4] in array size [200,100]
		//3*200 + 17
		
		index_t temp = coordinates[i];
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
		//node->node_level = -2;
		return 0;
	}
	
	
	mapObject* tree_map_obj = st_navigateTo(node->address, alloc_gran);
	byte* tree_pointer = tree_map_obj->address_ptr;
	
	if(memcmp((char*) tree_pointer, "TREE", 4 * sizeof(char)) != 0)
	{
		node->entries_used = 0;
		//node->node_level = -1;
		if(memcmp((char*) tree_pointer, "SNOD", 4 * sizeof(char)) == 0)
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
	
	node->node_type = (NodeType) getBytesAsNumber(tree_pointer + 4, 1, META_DATA_BYTE_ORDER);
	//node->node_level = (uint16_t)getBytesAsNumber(tree_pointer + 5, 1, META_DATA_BYTE_ORDER);
	node->entries_used = (uint16_t) getBytesAsNumber(tree_pointer + 6, 2, META_DATA_BYTE_ORDER);
	
	
	size_t key_size;
	//size_t bytes_needed;
	
	if(node->node_type != CHUNK)
	{
		sprintf(error_id, "getmatvar:wrongNodeType");
		sprintf(error_message, "A group node was found in the chunked data tree.\n\n");
		return 1;
	}
	
	
	//1-4: Size of chunk in bytes
	//4-8 Filter mask
	//(Dimensionality+1)*16 bytes
	key_size = (size_t) 4 + 4 + (num_chunked_dims + 1) * 8;
	
	st_releasePages(tree_map_obj);
	address_t key_address = node->address + 8 + 2 * s_block.size_of_offsets;
	//bytes_needed = key_size + s_block.size_of_offsets;
	size_t total_bytes_needed = (node->entries_used + 1) * key_size + node->entries_used * s_block.size_of_offsets;
	mapObject* key_map_obj = st_navigateTo(key_address, total_bytes_needed);
	byte* key_pointer = key_map_obj->address_ptr;
	
	for(int i = 0; i < node->entries_used; i++)
	{
		TreeNode sub_node;
		sub_node.address = (address_t) getBytesAsNumber(key_pointer + key_size, s_block.size_of_offsets,
											   META_DATA_BYTE_ORDER) + s_block.base_address;
		sub_node.node_type = NODETYPE_UNDEFINED;
		
		if(fillNode(&sub_node, num_chunked_dims) != 0)
		{
			return 1;
		}
		
		if(sub_node.node_type == RAWDATA)
		{
			
			DataPair* dp = malloc(sizeof(DataPair));
#ifdef NO_MEX
			if(dp == NULL)
			{
				sprintf(error_id, "getmatvar:mallocErrK0FN");
				sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
				return 1;
			}
#endif
			dp->chunk_size = (uint32_t) getBytesAsNumber(key_pointer, 4, META_DATA_BYTE_ORDER);
			//node->keys[0]->filter_mask = (uint32_t)getBytesAsNumber(key_pointer + 4, 4, META_DATA_BYTE_ORDER);
			dp->chunk_start = malloc((num_chunked_dims + 1) * sizeof(index_t));
#ifdef NO_MEX
			if(dp->chunk_start == NULL)
			{
				free(dp);
				sprintf(error_id, "getmatvar:mallocErrK0FNCS");
				sprintf(error_message, "Memory allocation failed. Your system may be out of memory.\n\n");
				return 1;
			}
#endif
			for(int j = 0; j < num_chunked_dims; j++)
			{
				dp->chunk_start[num_chunked_dims - 1 - j] = (index_t) getBytesAsNumber(key_pointer + 8 + j * 8, 8,
																		 META_DATA_BYTE_ORDER);
			}
			dp->chunk_start[num_chunked_dims] = 0;
			
			size_t page_index = (size_t) sub_node.address / alloc_gran;
			page_objects[page_index].total_num_mappings++;
			page_objects[page_index].max_map_size = MAX(page_objects[page_index].max_map_size,
											    (sub_node.address % alloc_gran) + dp->chunk_size);
			dp->address = sub_node.address;
			enqueue(data_page_buckets[page_index], dp);
		}
		
		key_address += key_size + s_block.size_of_offsets;
		key_pointer += key_size + s_block.size_of_offsets;
		
	}
	
	st_releasePages(key_map_obj);
	return 0;
	
}


void freeDP(void* dat)
{
	DataPair* dp = (DataPair*) dat;
	free(dp->chunk_start);
	free(dp);
}


void makeChunkedUpdates(index_t* chunk_update, const index_t* chunked_dims, const index_t* dims, uint8_t num_dims)
{
	index_t cu, du;
	for(int i = 0; i < num_dims; i++)
	{
		cu = 0;
		du = 1;
		for(int k = 0; k < i + 1; k++)
		{
			cu += (chunked_dims[k] - 1) * du;
			du *= dims[k];
		}
		chunk_update[i] = du - cu - 1;
	}
}

static void freeThreadBuffers(void)
{
	mt_freeQueue(data_buffers);
	mt_freeQueue(libdeflate_decomps);
	data_buffers = NULL;
	libdeflate_decomps = NULL;
}
//
//static void nothingFunc(void)
//{
//	;//nothing
//}