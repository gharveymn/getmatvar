#ifndef SUPERBLOCK_H
#define SUPERBLOCK_H

#include "getDataObjects.h"

#define SUPERBLOCK_INTERVAL 512

Superblock getSuperblock(void);
mapObject* findSuperblock(void);
Superblock fillSuperblock(byte* superblock_pointer);

#endif //SUPERBLOCK_H
