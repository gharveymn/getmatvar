//
// Created by Gene on 10/1/2017.
//

#ifndef UTILS_H
#define UTILS_H

#include "getDataObjects.h"

Data* findSubObjectByShortName(Data* object, char* name);
Data* findSubObjectBySCIndex(Data* object, uint64_t index);
Data* findObjectByHeaderAddress(address_t address);
uint16_t getRealNameLength(Data* object);
void parseCoordinates(VariableNameToken* vnt);
uint64_t coordToInd(const uint32_t* coords, const uint32_t* dims, uint8_t num_dims);
void makeVarnameQueue(char* variable_name);
Data* cloneData(Data* old_object);
void removeSpaces(char* source);
void readMXError(const char error_id[], const char error_message[], ...);
void readMXWarn(const char warn_id[], const char warn_message[], ...);


#endif //UTILS_H
