/*
 * MIT License
 *
 * Copyright (c) 2025 Ola Kåre Risa
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

#ifndef BINARY_FACE_H_
#define BINARY_FACE_H_

/*
 * BINARY FACE
 *
 * This watch face displays the current time in binary format, adapting to
 * the type of LCD display:
 *
 * - On classic displays (e.g., original Casio-style), there are not enough
 *   segments to show hours in full binary for values above 15, so hours are
 *   displayed in hexadecimal instead. In 12-hour format, there are in principle
 *   enough positions to show the numbers 1–12 in binary, but the original
 *   Casio display is only designed to display the digits 1, 2, and 3 in the
 *   tens place for the day, so a 0 written there would not render well.
 *
 * - On custom displays, all hour bits can be shown in full binary format.
 *
 * Minutes are always shown in binary on both display types.
 *
 * Additional features:
 * - The 24h mode and AM/PM indicators are handled according to user settings.
 * - Alarm and hourly time signal indicators are displayed when enabled.
 * - Low battery status is indicated differently depending on display type:
 *   - Custom display: interlocking arrows
 *   - Classic display: LAP indicator
 *
 * The watch face also optimizes rendering by updating only changed segments
 * since the previous tick.
 */

#include "movement.h"

typedef struct {
    struct {
        watch_date_time_t previous;
    } date_time;
    uint8_t last_battery_check;
    uint8_t watch_face_index;
    bool time_signal_enabled;
    bool battery_low;
} binary_state_t;

void binary_face_setup(uint8_t watch_face_index, void ** context_ptr);
void binary_face_activate(void *context);
bool binary_face_loop(movement_event_t event, void *context);
void binary_face_resign(void *context);

#define binary_face ((const watch_face_t){ \
    binary_face_setup, \
    binary_face_activate, \
    binary_face_loop, \
    binary_face_resign, \
    NULL, \
})

#endif // BINARY_FACE_H_

