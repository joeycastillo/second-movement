#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "watch.h"

typedef enum {
    EVENT_NONE = 0,             // There is no event to report.
    EVENT_ACTIVATE,             // Your watch face is entering the foreground.
    EVENT_TICK,                 // Most common event type. Your watch face is being called from the tick callback.
    EVENT_LOW_ENERGY_UPDATE,    // If the watch is in low energy mode and you are in the foreground, you will get a chance to update the display once per minute.
    EVENT_BACKGROUND_TASK,      // Your watch face is being invoked to perform a background task. Don't update the display here; you may not be in the foreground.
    EVENT_TIMEOUT,              // Your watch face has been inactive for a while. You may want to resign, depending on your watch face's intended use case.

    EVENT_LIGHT_BUTTON_DOWN,    // The light button has been pressed, but not yet released.
    EVENT_LIGHT_BUTTON_UP,      // The light button was pressed for less than half a second, and released.
    EVENT_LIGHT_LONG_PRESS,     // The light button was held for over half a second, but not yet released.
    EVENT_LIGHT_LONG_UP,        // The light button was held for over half a second, and released.
    EVENT_MODE_BUTTON_DOWN,     // The mode button has been pressed, but not yet released.
    EVENT_MODE_BUTTON_UP,       // The mode button was pressed for less than half a second, and released.
    EVENT_MODE_LONG_PRESS,      // The mode button was held for over half a second, but not yet released.
    EVENT_MODE_LONG_UP,         // The mode button was held for over half a second, and released. NOTE: your watch face will resign immediately after receiving this event.
    EVENT_ALARM_BUTTON_DOWN,    // The alarm button has been pressed, but not yet released.
    EVENT_ALARM_BUTTON_UP,      // The alarm button was pressed for less than half a second, and released.
    EVENT_ALARM_LONG_PRESS,     // The alarm button was held for over half a second, but not yet released.
    EVENT_ALARM_LONG_UP,        // The alarm button was held for over half a second, and released.

    EVENT_ACCELEROMETER_WAKE,   // The accelerometer has detected motion and woken up.
    EVENT_SINGLE_TAP,           // Accelerometer detected a single tap. This event is not yet implemented.
    EVENT_DOUBLE_TAP,           // Accelerometer detected a double tap. This event is not yet implemented.
} movement_event_type_t;

typedef struct {
    uint8_t event_type;
    uint8_t subsecond;
} movement_event_t;

bool movement_default_loop_handler(movement_event_t event);
void movement_move_to_face(uint8_t watch_face_index);
void movement_move_to_next_face(void);

watch_date_time_t movement_get_local_date_time(void);
int32_t movement_get_current_timezone_offset_for_zone(uint8_t zone_index);
int32_t movement_get_current_timezone_offset(void);
void movement_request_tick_frequency(uint8_t freq);

void movement_mock_set_local_date_time(watch_date_time_t date_time);
void movement_mock_clear_local_date_time(void);
