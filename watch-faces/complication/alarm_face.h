/*
 * MIT License
 *
 * Copyright (c) 2022 Josh Berson
 * Copyright (c) 2025 Joey Castillo
 * Copyright (c) 2025 Alessandro Genova
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

/*
 * Alarm Face modeled after the module in the GW-M5610U.
 *
 * This face is a port of the alarm face that can be found on more advanced Casios,
 * with minor tweaks to make the UI work with 3 buttons instead of the typical 4.
 * 
 * In this face you can configure:
 * - 4 daily alarms
 * - 1 daily snooze alarm (will repeat 7 times at 5 minutes interval, unless the user enters the face)
 * - 1 hourly chime
 * 
 * Bonus feature unique to this implementation, the hourly chime can be set for minutes other than :00.
 *
 * Basic daily alarm clock face. Seems useful if nothing else in the interest
 * of feature parity with the F-91W’s OEM module, 593.
 */

#include "movement.h"

// enum for setting alarm time
typedef enum {
    ALARM_FACE_SETTING_MODE_NONE = 0,
    ALARM_FACE_SETTING_MODE_SETTING_HOUR,
    ALARM_FACE_SETTING_MODE_SETTING_MINUTE
} alarm_face_setting_mode_t;

#define ALARM_FACE_NUM_ALARMS 6
#define ALARM_FACE_SNOOZE_ALARM_INDEX (ALARM_FACE_NUM_ALARMS - 2)
#define ALARM_FACE_CHIME_INDEX (ALARM_FACE_NUM_ALARMS - 1)
#define ALARM_FACE_SNOOZE_DELAY 5
#define ALARM_FACE_SNOOZE_REPETITIONS 7

typedef struct {
    uint32_t hour : 5;
    uint32_t minute : 6;
    uint32_t enabled : 1;
} alarm_face_alarm_t;

typedef struct {
    uint8_t alarm_index;
    alarm_face_alarm_t alarms[ALARM_FACE_NUM_ALARMS];
    alarm_face_alarm_t next_snooze_alarm;
    uint8_t remaining_snooze_repetitions;
    uint32_t quick_increase: 1;
    alarm_face_setting_mode_t setting_mode : 2;
    uint32_t play_alarm: 1;
    uint32_t play_signal: 1;
} alarm_face_state_t;

void alarm_face_setup(uint8_t watch_face_index, void **context_ptr);
void alarm_face_activate(void *context);
bool alarm_face_loop(movement_event_t event, void *context);
void alarm_face_resign(void *context);
movement_watch_face_advisory_t alarm_face_advise(void *context);

#define alarm_face ((const watch_face_t){ \
    alarm_face_setup, \
    alarm_face_activate, \
    alarm_face_loop, \
    alarm_face_resign, \
    alarm_face_advise \
})
