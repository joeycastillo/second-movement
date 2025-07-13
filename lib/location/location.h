#ifndef LOCATION_H_
#define LOCATION_H_

#include "movement.h"

#define SET_LOCATION_FACE_INDEX 12

void location_persist(movement_location_t location);
movement_location_t location_load(void);

#endif // LOCATION_H_