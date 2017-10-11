#ifndef NAVIGATE_H
#define NAVIGATE_H

#include "getDataObjects.h"

byte* renavigateTo(uint64_t address, uint64_t bytes_needed);
byte* navigateTo(address_t address, uint64_t bytes_needed);
void releasePages(address_t address, uint64_t bytes_needed);

#endif //NAVIGATE_H
