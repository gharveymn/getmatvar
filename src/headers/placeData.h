#ifndef PLACE_DATA_H
#define PLACE_DATA_H

#include "getDataObjects.h"


errno_t allocateSpace(Data* object);
void placeData(Data* object, byte* data_pointer, uint64_t dst_ind, uint64_t src_ind, uint64_t num_elems, size_t elem_size, ByteOrder data_byte_order);
void placeDataAtIndex(Data* object, byte* data_pointer, uint64_t dst_ind, uint64_t src_ind, uint64_t num_elems, size_t elem_size, ByteOrder data_byte_order);
void placeDataWithIndexMap(Data* object, byte* data_pointer, uint64_t num_elems, size_t elem_size, ByteOrder data_byte_order, const uint64_t* index_map, const uint64_t* index_sequence);

#endif //PLACE_DATA_H
