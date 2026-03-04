/*
 * MIT License
 *
 * Copyright (c) 2026 Diego Perez
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

#ifndef TEMPERATURE_FORECAST_FACE_H_
#define TEMPERATURE_FORECAST_FACE_H_

/*
 * TEMPERATURE FORECAST face
 *
 * Displays 7-day weather forecast using the forecast_table API.
 *
 * Usage:
 * - MODE short: Cycle through 7 days (today → 6 days out)
 * - ALARM short: Toggle between temperature and daylight hours display
 * - Shows "no da" when forecast_table is not valid
 * - Falls back to homebase seasonal average when forecast unavailable
 *
 * Display format:
 * Line 1: Day abbreviation (Mo, Tu, We, Th, Fr, Sa, Su)
 * Line 2: Temperature in Celsius (converted from c10)
 * Indicator: "FC" when forecast data is valid
 */

#include "movement.h"

typedef struct {
    uint8_t current_day;     // 0-6 (today through 6 days out)
    bool show_daylight;      // false = temp, true = daylight
} temperature_forecast_state_t;

void temperature_forecast_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr);
void temperature_forecast_face_activate(movement_settings_t *settings, void *context);
bool temperature_forecast_face_loop(movement_event_t event, movement_settings_t *settings, void *context);
void temperature_forecast_face_resign(movement_settings_t *settings, void *context);

#define temperature_forecast_face ((const watch_face_t){ \
    temperature_forecast_face_setup, \
    temperature_forecast_face_activate, \
    temperature_forecast_face_loop, \
    temperature_forecast_face_resign, \
    NULL, \
})

#endif // TEMPERATURE_FORECAST_FACE_H_
