/*
 * MIT License
 *
 * Copyright (c) 2026 Raffael Vogler
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
 * Sun and Moon compass.
 *
 * Uses UTC date/time and configured latitude/longitude to estimate the
 * direction of North relative to the Sun and Moon.
 *
 * The hours field shows the Sun-based result; the minutes field shows
 * the Moon-based result. Each value is a dial position from 00 to 59,
 * increasing clockwise, with one position representing 6 degrees.
 *
 * Hold the watch level and point its 12 o'clock direction towards the
 * Sun or Moon. The corresponding displayed dial position indicates North.
 * This is a calculated direction aid, not a magnetic compass.
 *
 * Positions are recalculated on activation, periodically while active,
 * and on low-energy updates. The seconds field shows the active update
 * countdown, or blank during low-energy updates.
 *
 * Set LAT and LON in the source or via sunrise/sunset face for your location.
 */

typedef struct {
    float lat;
    float lon;
    float sun_pos;
    float moon_pos;
    int8_t ctr;
    bool location_is_set;
} sun_moon_compass_state_t;

void sun_moon_compass_face_setup(uint8_t watch_face_index, void ** context_ptr);
void sun_moon_compass_face_activate(void *context);
bool sun_moon_compass_face_loop(movement_event_t event, void *context);
void sun_moon_compass_face_resign(void *context);


#define sun_moon_compass_face ((const watch_face_t){ \
    sun_moon_compass_face_setup, \
    sun_moon_compass_face_activate, \
    sun_moon_compass_face_loop, \
    sun_moon_compass_face_resign, \
    NULL, \
})
