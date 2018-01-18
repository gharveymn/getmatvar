#ifndef NAVIGATE_H
#define NAVIGATE_H

#include "getDataObjects.h"

byte* st_renavigateTo(byte* page_start_pointer, uint64_t address, uint64_t bytes_needed);
byte* st_navigateTo(address_t address, uint64_t bytes_needed);
void st_releasePages(byte* start_page_pointer, address_t address, uint64_t bytes_needed);

byte* mt_renavigateTo(uint64_t address, uint64_t bytes_needed);
byte* mt_navigateTo(address_t address, uint64_t bytes_needed);
void mt_releasePages(address_t address, uint64_t bytes_needed);

#endif //NAVIGATE_H
