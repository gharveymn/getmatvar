#ifndef PLACE_DATA_H
#define PLACE_DATA_H

#include "getDataObjects.h"


error_t allocateSpace(Data* object);
void placeData(Data* object, byte* data_pointer, index_t dst_ind, index_t src_ind, index_t num_elems, size_t elem_size, ByteOrder data_byte_order);
void placeDataAtIndex(Data* object, byte* data_pointer, index_t dst_ind, index_t src_ind, index_t num_elems, size_t elem_size, ByteOrder data_byte_order);
void placeDataWithIndexMap(Data* object, byte* data_pointer, index_t num_elems, index_t elem_size, ByteOrder data_byte_order, const index_t* index_map, const index_t* index_sequence);

#endif //PLACE_DATA_H
