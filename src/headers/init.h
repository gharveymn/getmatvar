#ifndef INIT_H
#define INIT_H

#include "getDataObjects.h"

void initialize(void);
error_t initializeObject(Data* object);
error_t initializePageObjects(void);

#endif //INIT_H
