#ifndef INIT_H
#define INIT_H

#include "getDataObjects.h"

void initialize(void);
errno_t initializeObject(Data* object);
errno_t initializePageObjects(void);

#endif //INIT_H
