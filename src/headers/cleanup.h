//
// Created by Gene on 10/1/2017.
//

#ifndef CLEANUP_H
#define CLEANUP_H

#include "getDataObjects.h"

void freeVarname(void* vn);
void releasePages(address_t address, uint64_t bytes_needed);
void freeDataObject(void* object);
void freeDataObjectTree(Data* data_object);
void destroyPageObjects(void);
void endHooks(void);

#endif //CLEANUP_H
