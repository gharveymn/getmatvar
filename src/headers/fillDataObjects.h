#ifndef FILL_DATA_OBJECTS_H
#define FILL_DATA_OBJECTS_H

#include "getDataObjects.h"

int fillVariable(char* variable_name);
void collectMetaData(Data* object, address_t header_address, uint16_t num_msgs, uint32_t header_length);
uint16_t interpretMessages(Data* object, address_t msgs_start_address, uint32_t msgs_length, uint16_t message_num, uint16_t num_msgs, uint16_t repeat_tracker);
int fillObject(Data* object, uint64_t this_obj_address);
void fillFunctionHandleData(Data* fh);
int fillDataTree(Data* object);

#endif //FILL_DATA_OBJECTS_H
