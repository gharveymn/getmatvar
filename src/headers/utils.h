//
// Created by Gene on 10/1/2017.
//

#ifndef UTILS_H
#define UTILS_H

#include "getDataObjects.h"

Data* findSubObjectByShortName(Data* object, char* name);
Data* findObjectByHeaderAddress(address_t address);
void readMXError(const char error_id[], const char error_message[], ...);
void readMXWarn(const char warn_id[], const char warn_message[], ...);

#endif //UTILS_H
