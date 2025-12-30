/*
 * MIT License
 *
 * Copyright (c) 2025 Giorgio Ciacchella
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

#include "movement.h"

/*
 * WATCH/RADIO ALARM FACE
 *
 * Currently basic daily alarm clock face.
 *
 */

// enum for setting alarm time
typedef enum {
    WATCH_RADIO_ALARM_FACE_SETTING_MODE_NONE = 0,
    WATCH_RADIO_ALARM_FACE_SETTING_MODE_SETTING_HOUR,
    WATCH_RADIO_ALARM_FACE_SETTING_MODE_SETTING_MINUTE,
    WATCH_RADIO_ALARM_FACE_SETTING_MODE_SETTING_ALARM_PERIOD,
    WATCH_RADIO_ALARM_FACE_SETTING_MODE_SETTING_CHIME_PERIOD
} watch_radio_alarm_face_setting_mode_t;

// enum for alarm period
typedef enum {
    WATCH_RADIO_ALARM_FACE_PERIOD_AMPM = 0,
    WATCH_RADIO_ALARM_FACE_PERIOD_AM,
    WATCH_RADIO_ALARM_FACE_PERIOD_PM
} watch_radio_alarm_face_alarm_period_t;

// array for chime periods
static const uint32_t chime_periods[] = {60, 30, 15};

// enum for background event type
typedef enum {
    WATCH_RADIO_ALARM_FACE_BG_TASK_TYPE_NONE = 0,
    WATCH_RADIO_ALARM_FACE_BG_TASK_TYPE_ALARM,
    WATCH_RADIO_ALARM_FACE_BG_TASK_TYPE_CHIME
} watch_radio_alarm_face_bg_task_type_t;

typedef struct {
    // Anything you need to keep track of, put it here!
    uint32_t hour : 5;
    uint32_t minute : 6;
    watch_radio_alarm_face_alarm_period_t alarm_period : 2;
    uint32_t alarm_is_on : 2;
    uint32_t chime_period_idx : 2;
    uint32_t chime_is_on : 2;
    watch_radio_alarm_face_bg_task_type_t bg_task_type : 2;
    watch_radio_alarm_face_setting_mode_t setting_mode : 3;
} watch_radio_alarm_face_state_t;

void watch_radio_alarm_face_setup(uint8_t watch_face_index, void ** context_ptr);
void watch_radio_alarm_face_activate(void *context);
bool watch_radio_alarm_face_loop(movement_event_t event, void *context);
void watch_radio_alarm_face_resign(void *context);
movement_watch_face_advisory_t watch_radio_alarm_face_advise(void *context);

#define watch_radio_alarm_face ((const watch_face_t){ \
    watch_radio_alarm_face_setup, \
    watch_radio_alarm_face_activate, \
    watch_radio_alarm_face_loop, \
    watch_radio_alarm_face_resign, \
    watch_radio_alarm_face_advise, \
})
