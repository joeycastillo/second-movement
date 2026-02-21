/*
 * MIT License
 *
 * Copyright (c) 2025 Raffael Mancini
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
 * LOCAL SOLAR TIME FACE
 *
 * Displays solar time information based on the user's location.
 * Formulas follow the notation from:
 *   https://www.pveducation.org/pvcdrom/properties-of-sunlight/solar-time
 *
 * Variables (pveducation.org notation):
 *   LSTM  - Local Standard Time Meridian [degrees]  = 15 * ΔTUTC
 *   B     - Seasonal angle [degrees]                = (360/365) * (d - 81)
 *   EoT   - Equation of Time [minutes]              = 9.87*sin(2B) - 7.53*cos(B) - 1.5*sin(B)
 *   TC    - Time Correction Factor [minutes]        = 4*(Longitude - LSTM) + EoT
 *   LST   - Local Solar Time [hours]                = LT + TC/60
 *   HRA   - Hour Angle [degrees]                    = 15*(LST - 12)
 *
 * B, EoT and TC only depend on the day-of-year d, so they are cached
 * in state and recomputed exactly once per day: the cache key is d itself
 * (1-366). Zero-initialisation of state guarantees a recompute on first use.
 *
 * Requires the location to be set via the Sunrise/Sunset face, stored in
 * "location.u32" on the filesystem.  If no location is set, displays
 * "SO  no Loc".
 *
 * Display modes (cycle with the Alarm / start-stop button):
 *   SO  HH:MM:SS  — Local Solar Time (LST), live seconds display
 *   nO  HH:MM     — Solar Noon in local clock time
 *   Hr  ±DDD      — Hour Angle (HRA) in degrees; negative=morning, positive=afternoon
 */

#include "movement.h"

typedef enum {
    LOCAL_SOLAR_TIME_MODE_LST  = 0,  /* Local Solar Time    SO HH:MM:SS */
    LOCAL_SOLAR_TIME_MODE_NOON = 1,  /* Solar Noon (local)  nO HH:MM    */
    LOCAL_SOLAR_TIME_MODE_HRA  = 2,  /* Hour Angle          Hr ±DDD     */
    LOCAL_SOLAR_TIME_NUM_MODES
} local_solar_time_mode_t;

typedef struct {
    local_solar_time_mode_t mode;
    uint16_t last_calc_d;  /* day-of-year (1-366) when EoT/TC were last computed;
                              0 (zero-init) guarantees recomputation on first tick */
    float    EoT;          /* Equation of Time [minutes]       */
    float    TC;           /* Time Correction Factor [minutes] */
} local_solar_time_state_t;

void local_solar_time_face_setup(uint8_t watch_face_index, void **context_ptr);
void local_solar_time_face_activate(void *context);
bool local_solar_time_face_loop(movement_event_t event, void *context);
void local_solar_time_face_resign(void *context);

#define local_solar_time_face ((const watch_face_t){ \
    local_solar_time_face_setup,    \
    local_solar_time_face_activate, \
    local_solar_time_face_loop,     \
    local_solar_time_face_resign,   \
    NULL,                           \
})
