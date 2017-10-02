//
// Created by Gene on 10/1/2017.
//

#ifndef GET_SYSTEM_INFO_H
#define GET_SYSTEM_INFO_H

#include "getDataObjects.h"

size_t getPageSize(void);
size_t getAllocGran(void);
int getNumProcessors(void);
ByteOrder getByteOrder(void);

#endif //GET_SYSTEM_INFO_H
