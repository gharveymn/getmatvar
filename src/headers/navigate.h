#ifndef NAVIGATE_H
#define NAVIGATE_H

#include "getDataObjects.h"

mapObject* st_navigateTo(address_t address, size_t bytes_needed);
void st_releasePages(mapObject* map_obj);

byte* mt_navigateTo(address_t address, size_t bytes_needed);
void mt_releasePages(address_t address, size_t bytes_needed);

#endif //NAVIGATE_H
