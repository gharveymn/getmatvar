//
// Created by Gene on 10/1/2017.
//

#ifndef READ_MESSAGE_H
#define READ_MESSAGE_H

#include "getDataObjects.h"

void readDataSpaceMessage(Data* object, byte* msg_pointer, uint64_t msg_address, uint16_t msg_size, error_t* err_flag);
void readDataTypeMessage(Data* object, byte* msg_pointer, address_t msg_address, uint16_t msg_size, error_t* err_flag);
void readDataLayoutMessage(Data* object, byte* msg_pointer, address_t msg_address, uint16_t msg_size, error_t* err_flag);
void readDataStoragePipelineMessage(Data* object, byte* msg_pointer, address_t msg_address, uint16_t msg_size, error_t* err_flag);
void readAttributeMessage(Data* object, byte* msg_pointer, address_t msg_address, uint16_t msg_size, error_t* err_flag);
void selectMatlabClass(Data* object);


#endif //READ_MESSAGE_H
