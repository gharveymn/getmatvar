//
// Created by Gene on 10/1/2017.
//

#ifndef READ_MESSAGE_H
#define READ_MESSAGE_H

#include "getDataObjects.h"

void readDataSpaceMessage(Data* object, byte* msg_pointer, address_t msg_address, uint16_t msg_size);
void readDataTypeMessage(Data* object, byte* msg_pointer, address_t msg_address, uint16_t msg_size);
void readDataLayoutMessage(Data* object, byte* msg_pointer, address_t msg_address, uint16_t msg_size);
void readDataStoragePipelineMessage(Data* object, byte* msg_pointer, address_t msg_address, uint16_t msg_size);
void readAttributeMessage(Data* object, byte* msg_pointer, address_t msg_address, uint16_t msg_size);
void selectMatlabClass(Data* object);


#endif //READ_MESSAGE_H
