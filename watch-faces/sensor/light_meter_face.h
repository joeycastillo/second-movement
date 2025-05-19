/*
 * MIT License
 *
 * Copyright (c) 2025 Joey Castillo
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

#ifdef HAS_IR_SENSOR

/*
 * LIGHT METER
 *
 * EXTREME WORK IN PROGRESS on a photographic light meter.
 * Currently not even remotely calibrated! I did one afternoon of tests with light
 * transmitted through the custom LCD, and half-assedly applied a curve that almost
 * not not quite entirely fit the data. Pull requests welcome!
 *
 */

typedef enum {
    LIGHT_METER_APERTURE_F1 = -1,   // f/1 will not appear as an aperture priority option
    LIGHT_METER_APERTURE_F1_4,      // numbered 0, f/1.4 is the first aperture priority option
    LIGHT_METER_APERTURE_F2,
    LIGHT_METER_APERTURE_F2_8,
    LIGHT_METER_APERTURE_F4,
    LIGHT_METER_APERTURE_F5_6,
    LIGHT_METER_APERTURE_F8,
    LIGHT_METER_APERTURE_F11,
    LIGHT_METER_APERTURE_F16,
    LIGHT_METER_APERTURE_F22,
    LIGHT_METER_APERTURE_F32,
    LIGHT_METER_APERTURE_COUNT
} light_meter_aperture_t;

typedef enum {
    LIGHT_METER_SHUTTER_1_SEC = -2, // 1 second and 1/2 second will not appear as a shutter priority option
    LIGHT_METER_SHUTTER_1_2,
    LIGHT_METER_SHUTTER_1_4, // numbered 0, 1/4 second is the first one to appear in AP list
    LIGHT_METER_SHUTTER_1_8,
    LIGHT_METER_SHUTTER_1_15,
    LIGHT_METER_SHUTTER_1_30,
    LIGHT_METER_SHUTTER_1_60,
    LIGHT_METER_SHUTTER_1_125,
    LIGHT_METER_SHUTTER_1_250,
    LIGHT_METER_SHUTTER_1_500,
    LIGHT_METER_SHUTTER_1_1000,
    LIGHT_METER_SHUTTER_1_2000,
    LIGHT_METER_SHUTTER_1_4000,
    LIGHT_METER_SHUTTER_COUNT
} light_meter_shutter_speed_t;

typedef enum {
    LIGHT_METER_ISO_25 = 0,
    LIGHT_METER_ISO_50,
    LIGHT_METER_ISO_100,
    LIGHT_METER_ISO_200,
    LIGHT_METER_ISO_400,
    LIGHT_METER_ISO_800,
    LIGHT_METER_ISO_1600,
    LIGHT_METER_ISO_3200,
    LIGHT_METER_ISO_COUNT
} light_meter_iso_t;

typedef enum {
    LIGHT_METER_MODE_APERTURE_PRIORITY = 0,
    LIGHT_METER_MODE_SHUTTER_PRIORITY,
    LIGHT_METER_MODE_AP_IS_SETTING_ISO,
    LIGHT_METER_MODE_SP_IS_SETTING_ISO
} light_meter_mode_t;

typedef struct {
    light_meter_mode_t mode;
    light_meter_iso_t iso;
    light_meter_aperture_t aperture_priority;
    light_meter_shutter_speed_t shutter_priority;
} light_meter_state_t;

void light_meter_face_setup(uint8_t watch_face_index, void ** context_ptr);
void light_meter_face_activate(void *context);
bool light_meter_face_loop(movement_event_t event, void *context);
void light_meter_face_resign(void *context);

#define light_meter_face ((const watch_face_t){ \
    light_meter_face_setup, \
    light_meter_face_activate, \
    light_meter_face_loop, \
    light_meter_face_resign, \
    NULL, \
})

#endif // HAS_IR_SENSOR
