//
// Created by Gene on 10/1/2017.
//

#ifndef NUMBER_HELPER_H
#define NUMBER_HELPER_H

#include "getDataObjects.h"

size_t roundUp8(size_t numToRound);
size_t getBytesAsNumber(byte* chunk_start, size_t num_bytes, ByteOrder endianness);
void reverseBytes(byte* data_pointer, size_t num_elems);

#endif //NUMBER_HELPER_H
