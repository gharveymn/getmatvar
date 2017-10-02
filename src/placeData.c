#include "headers/placeData.h"

errno_t allocateSpace(Data* object)
{
	//maybe figure out a way to just pass this to a single array
	switch(object->matlab_internal_type)
	{
		case mxINT8_CLASS:
		case mxUINT8_CLASS:
		case mxINT16_CLASS:
		case mxUINT16_CLASS:
		case mxINT32_CLASS:
		case mxUINT32_CLASS:
		case mxINT64_CLASS:
		case mxUINT64_CLASS:
		case mxSINGLE_CLASS:
		case mxDOUBLE_CLASS:
		case mxLOGICAL_CLASS:
		case mxCHAR_CLASS:
			object->data_arrays.data = mxMalloc(object->num_elems*object->elem_size);
			break;
		case mxCELL_CLASS:
			//STORE ADDRESSES IN THE UDOUBLE_DATA ARRAY; THESE ARE NOT ACTUAL ELEMENTS
			object->data_arrays.sub_object_header_offsets = malloc(object->num_elems*object->elem_size);
			break;
		case mxSTRUCT_CLASS:
		case mxFUNCTION_CLASS:
			//Don't allocate anything yet. This will be handled later
			object->num_elems = 1;
			object->num_dims = 2;
			object->dims[0] = 1;
			object->dims[1] = 1;
			object->dims[2] = 0;
			break;
		case mxOBJECT_CLASS:
		case mxSPARSE_CLASS:
			//do nothing
			break;
		default:
			//this shouldn't happen
			error_flag = TRUE;
			sprintf(error_id, "getmatvar:thisShouldntHappen");
			sprintf(error_message, "Tried to allocate space for an unknown type.\n\n");
			return 1;
		
	}
	
	return 0;
	
}


void placeData(Data* object, byte* data_pointer, uint64_t starting_index, uint64_t condition, size_t elem_size, ByteOrder data_byte_order)
{
	
	//reverse the bytes if the byte order doesn't match the cpu architecture
	if(__byte_order__ != data_byte_order)
	{
		for(uint64_t j = 0; j < condition - starting_index; j += elem_size)
		{
			reverseBytes(data_pointer + j, elem_size);
		}
	}
	
	switch(object->matlab_internal_type)
	{
		case mxINT8_CLASS:
		case mxUINT8_CLASS:
		case mxINT16_CLASS:
		case mxUINT16_CLASS:
		case mxINT32_CLASS:
		case mxUINT32_CLASS:
		case mxINT64_CLASS:
		case mxUINT64_CLASS:
		case mxSINGLE_CLASS:
		case mxDOUBLE_CLASS:
		case mxLOGICAL_CLASS:
		case mxCHAR_CLASS:
			memcpy(object->data_arrays.data + elem_size*starting_index, data_pointer, (condition - starting_index)*elem_size);
			break;
		case mxCELL_CLASS:
			memcpy(&object->data_arrays.sub_object_header_offsets[starting_index], data_pointer, (condition - starting_index)*elem_size);
			break;
		case mxSTRUCT_CLASS:
		case mxFUNCTION_CLASS:
		case mxOBJECT_CLASS:
		case mxSPARSE_CLASS:
		default:
			//nothing to be done
			break;
		
	}
	
}


void placeDataWithIndexMap(Data* object, byte* data_pointer, uint64_t num_elems, size_t elem_size, ByteOrder data_byte_order, const uint64_t* index_map)
{
	
	//reverse the bytes if the byte order doesn't match the cpu architecture
	if(__byte_order__ != data_byte_order)
	{
		for(uint64_t j = 0; j < num_elems; j += elem_size)
		{
			reverseBytes(data_pointer + j, elem_size);
		}
	}
	
	
	int object_data_index = 0;
	switch(object->matlab_internal_type)
	{
		case mxINT8_CLASS:
		case mxUINT8_CLASS:
		case mxINT16_CLASS:
		case mxUINT16_CLASS:
		case mxINT32_CLASS:
		case mxUINT32_CLASS:
		case mxINT64_CLASS:
		case mxUINT64_CLASS:
		case mxSINGLE_CLASS:
		case mxDOUBLE_CLASS:
		case mxLOGICAL_CLASS:
		case mxCHAR_CLASS:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(object->data_arrays.data + elem_size*index_map[j], data_pointer + object_data_index*elem_size, elem_size);
				object_data_index++;
			}
			break;
		case mxCELL_CLASS:
			for(uint64_t j = 0; j < num_elems; j++)
			{
				memcpy(&object->data_arrays.sub_object_header_offsets[index_map[j]], data_pointer + object_data_index*elem_size, elem_size);
				object_data_index++;
			}
			break;
		case mxSTRUCT_CLASS:
		case mxFUNCTION_CLASS:
		case mxOBJECT_CLASS:
		case mxSPARSE_CLASS:
		default:
			//nothing to be done
			break;
		
	}
	
}
