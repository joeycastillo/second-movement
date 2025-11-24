#ifndef SET_LOCATION_FACE_H_
#define SET_LOCATION_FACE_H_

#include "movement.h"

typedef enum
{
  LOCATION_MODE_SET_LAT,
  LOCATION_MODE_SET_LON
} set_location_mode_t;

typedef struct
{
  set_location_mode_t mode;
  uint8_t active_digit;
  bool location_changed;
  int16_t working_latitude;
  int16_t working_longitude;
  bool sign_is_negative;
} set_location_state_t;

void set_location_face_setup(uint8_t watch_face_index, void **context_ptr);
void set_location_face_activate(void *context);
bool set_location_face_loop(movement_event_t event, void *context);
void set_location_face_resign(void *context);

#define set_location_face ((const watch_face_t){ \
    set_location_face_setup,                     \
    set_location_face_activate,                  \
    set_location_face_loop,                      \
    set_location_face_resign,                    \
    NULL,                                        \
})

#endif // SET_LOCATION_FACE_H_