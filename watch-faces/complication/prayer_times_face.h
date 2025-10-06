#ifndef PRAYER_TIMES_FACE_H_
#define PRAYER_TIMES_FACE_H_

#include "movement.h"
#include "adhan_wrapper.h"

typedef enum
{
  PRAYER_TIMES_MODE_DISPLAY,
  PRAYER_TIMES_MODE_SET_METHOD
} prayer_times_mode_t;

typedef struct
{
  calculation_parameters_t calculation_params;
  prayer_times_t prayer_times;
  uint8_t current_prayer_index;
  uint8_t last_calculated_day;
  bool times_calculated;
  bool location_set;
  bool manual_override;
  prayer_times_mode_t mode;
  uint8_t selected_method_index;
  bool alarms_enabled;
  bool first_run;
} prayer_times_state_t;

void prayer_times_face_setup(uint8_t watch_face_index, void **context_ptr);
void prayer_times_face_activate(void *context);
bool prayer_times_face_loop(movement_event_t event, void *context);
void prayer_times_face_resign(void *context);
movement_watch_face_advisory_t prayer_times_face_advise(void *context);

#define prayer_times_face ((const watch_face_t){ \
  prayer_times_face_setup,                     \
  prayer_times_face_activate,                  \
  prayer_times_face_loop,                      \
  prayer_times_face_resign,                    \
  prayer_times_face_advise,                    \
})

#endif // PRAYER_TIMES_FACE_H_