/*
 * MIT License
 *
 * Copyright (c) 2022 Spencer Bywater
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

#ifndef PROBABILITY_FACE_H_
#define PROBABILITY_FACE_H_

/*
 * PROBABILITY face
 *
 * This face is a dice-rolling random number generator.
 * Supports dice with 2, 4, 6, 8, 10, 12, 20, or 100 sides.
 *
 * Display format:
 * - Top: "Prb" (custom LCD) / "PR" (classic LCD)
 * - Top right: Die type (2, 4, 6, 8, 10, 12, 20, or "00" for d100)
 * - Hours:Minutes: Rolled value
 *   - No roll: "--:--"
 *   - Values 1-99: "  :XX"
 *   - Value 100: " 1:00"
 *   - Coin flip: "HE:AD:S " or "TA:IL:S "
 *
 * Controls:
 * - LIGHT button: Cycle through die type
 * - ALARM button: Roll the selected die
 * - Single tap: Cycle through die type (accelerometer)
 * - Double tap: Roll the selected die (accelerometer)
 *
 * Note: Accelerometer is enabled for 5 seconds when face activates and
 * after each tap to conserve battery. It automatically disables after
 * 5 seconds of no tap input.
 */

#include "movement.h"

typedef struct {
    uint8_t dice_sides;
    uint8_t rolled_value;
    uint8_t animation_frame;
    bool is_rolling;
    uint8_t tap_detection_ticks;
} probability_state_t;

void probability_face_setup(uint8_t watch_face_index, void ** context_ptr);
void probability_face_activate(void *context);
bool probability_face_loop(movement_event_t event, void *context);
void probability_face_resign(void *context);

#define probability_face ((const watch_face_t){ \
    probability_face_setup, \
    probability_face_activate, \
    probability_face_loop, \
    probability_face_resign, \
    NULL, \
})

#endif // PROBABILITY_FACE_H_

