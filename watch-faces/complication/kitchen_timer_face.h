/*
 * MIT License
 *
 * Copyright (c) 2024 Joseph Bryant
 * Copyright (c) 2023 Konrad Rieck
 * Copyright (c) 2022 Wesley Ellis
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

#ifndef KITCHEN_TIMER_FACE_H_
#define KITCHEN_TIMER_FACE_H_

/*
 * KITCHEN TIMER face
 *
 * A multi-timer countdown face supporting up to 4 independent timers.
 * Each timer runs independently — you can have timer 1 counting down
 * while setting timer 2, or multiple timers in overtime simultaneously.
 * When a countdown reaches zero, it enters an "overtime" mode that
 * counts UP from zero. When a timer is paused the display blinks. The LAP
 * indicator is shown whenever a timer is in overtime (counting up).
 *
 * The top of the display shows "KT" on the left and the current timer
 * number (1–4) on the right.
 *
 *   - Short press LIGHT to cycle between timer slots (1–4).
 *   - Long press LIGHT to illuminate the LED.
 *   - Long press ALARM on a reset timer to enter setting mode.
 *   - In setting mode, short press LIGHT to advance the cursor
 *     (hours → minutes → seconds → confirm).
 *   - In setting mode, long press LIGHT to clear from cursor onward.
 *   - In setting mode, short press ALARM to increment the selected field.
 *   - Long press ALARM in setting mode for quick increment.
 *   - Short press ALARM to start a reset timer, or pause a running/overtime one.
 *   - When paused, short press ALARM to resume.
 *   - When paused, long press ALARM to reset the timer.
 *   - Starting a timer set to 0:00:00 immediately enters overtime,
 *     acting as a "count up from now" stopwatch.
 *
 * Max countdown is 23 hours, 59 minutes and 59 seconds per timer.
 */

#include "movement.h"

#define KT_MAX_TIMERS 4

typedef enum {
    kt_paused,
    kt_running,
    kt_setting,
    kt_reset,
    kt_overtime,
    kt_overtime_paused
} kitchen_timer_mode_t;

typedef struct {
    uint32_t target_ts;
    uint32_t overtime_start_ts;
    uint32_t overtime_seconds;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t set_hours;
    uint8_t set_minutes;
    uint8_t set_seconds;
    kitchen_timer_mode_t mode;
} kitchen_timer_slot_t;

typedef struct {
    kitchen_timer_slot_t timers[KT_MAX_TIMERS];
    uint32_t now_ts;
    uint8_t timer_idx;
    uint8_t selection;
    uint8_t tap_detection_ticks;
    bool has_tapped_once;
    uint8_t watch_face_index;
} kitchen_timer_state_t;


void kitchen_timer_face_setup(uint8_t watch_face_index, void ** context_ptr);
void kitchen_timer_face_activate(void *context);
bool kitchen_timer_face_loop(movement_event_t event, void *context);
void kitchen_timer_face_resign(void *context);

#define kitchen_timer_face ((const watch_face_t){ \
    kitchen_timer_face_setup, \
    kitchen_timer_face_activate, \
    kitchen_timer_face_loop, \
    kitchen_timer_face_resign, \
    NULL, \
})

#endif // KITCHEN_TIMER_FACE_H_
