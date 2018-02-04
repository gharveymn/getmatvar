//
// Created by Gene on 10/1/2017.
//

#ifndef UTILS_H
#define UTILS_H

#include "getDataObjects.h"


Data* findSubObjectByShortName(Data* object, char* name);
Data* findSubObjectBySCIndex(Data* object, index_t index);
Data* findObjectByHeaderAddress(address_t obj_address);
error_t parseCoordinates(VariableNameToken* vnt);
index_t coordToInd(const index_t* coords, const index_t* dims, uint8_t num_dims);
error_t makeVarnameQueue(char* variable_name);
Data* cloneData(Data* old_object);
void removeSpaces(char* source);
void readMXError(const char error_id[], const char error_message[], ...);
void readMXWarn(const char warn_id[], const char warn_message[], ...);


#endif //UTILS_H
