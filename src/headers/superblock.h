#ifndef SUPERBLOCK_H
#define SUPERBLOCK_H

#include "getDataObjects.h"

Superblock getSuperblock(void);
byte* findSuperblock(void);
Superblock fillSuperblock(byte* superblock_pointer);

#endif //SUPERBLOCK_H
