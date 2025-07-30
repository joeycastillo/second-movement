/*
 * MIT License
 *
 * Copyright (c) 2025 Gábor Nyéki
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

/*
 * Baby kicks face
 *
 * Count the movements of your in-utero baby.
 *
 * Background:
 *
 * This practice is recommended particularly in the third trimester
 * (from week 28 onwards).  The exact recommendations vary as to how to
 * count the baby's movements.  Some recommend drawing a chart with the
 * number of "kicks" within a 12-hour period:
 *
 * - https://en.wikipedia.org/wiki/Kick_chart
 *
 * Others recommend measuring the time that it takes for the baby to
 * "kick" 10 times:
 *
 * - https://my.clevelandclinic.org/health/articles/23497-kick-counts
 * - https://healthy.kaiserpermanente.org/health-wellness/health-encyclopedia/he.pregnancy-kick-counts.aa107042
 *
 * (Of course, not every movement that the baby makes is a kick, and we
 * are interested in all movements, not only kicks.)
 *
 * This watch face follows the latter set of recommendations.  When you
 * start the counter, it measures the number of elapsed minutes, and it
 * tracks the number of movements as you increment the counter.  Since
 * some consecutive movements made by the baby are actually part of a
 * longer maneuver, the watch face also displays the number of
 * one-minute stretches in which the baby moved at least once.
 *
 * Usage:
 *
 * - ALARM button, short press:
 *   * start the counter if it isn't running
 *   * increment the count otherwise
 * - ALARM button, long press: undo the last count
 * - MODE button, long press: reset the count to zero
 *
 * The watch face displays two numbers in the "clock digits" positions:
 *
 * 1. Count of movements (in the "second" and "minute" positions).
 * 2. Count of one-minute stretches in which at least one movement
 *    occurred (in the "hour" position).
 *
 * The number of elapsed minutes, up to and including 29, is shown in
 * the "day digits" position.  Due to the limitations of the classic LCD
 * display, completed 30-minute intervals are shown in the "weekday
 * digits" position.  The total number of elapsed minutes is the sum of
 * these two numbers.
 *
 * The watch face times out after 99 minutes, since it cannot display
 * more than 99 one-minute stretches in the "hour" position.  When this
 * happens, the "weekday digits" position shows "TO".
 */

#include "movement.h"

typedef enum {
    BABY_KICKS_MODE_SPLASH = 0,
    BABY_KICKS_MODE_ACTIVE,
    BABY_KICKS_MODE_TIMED_OUT,
    BABY_KICKS_MODE_LE_MODE,
} baby_kicks_mode_t;

/* Stop counting after 99 minutes.  The classic LCD cannot display any
 * larger number in the "weekday digits" position. */
#define BABY_KICKS_TIMEOUT 99

/* Ring buffer to store and allow undoing up to 10 movements. */
typedef struct {
    /* For each movement in the undo buffer, this array stores the value
     * of `state->stretch_count` right before the movement was
     * recorded.  This is used for decrementing `state->stretch_count`
     * as part of the undo operation if necessary. */
    uint8_t stretches[10];

    /* Index of the next available slot in `.stretches`. */
    uint8_t head;
} baby_kicks_undo_buffer_t;

typedef struct {
    bool currently_displayed;
    baby_kicks_mode_t mode;
    watch_date_time_t now;
    uint32_t start;
    uint32_t latest_stretch_start;
    uint8_t stretch_count;   /* Between 0 and `BABY_KICKS_TIMEOUT`. */
    uint16_t movement_count; /* Between 0 and 9999. */
    baby_kicks_undo_buffer_t undo_buffer;
} baby_kicks_state_t;

void baby_kicks_face_setup(uint8_t watch_face_index, void **context_ptr);
void baby_kicks_face_activate(void *context);
bool baby_kicks_face_loop(movement_event_t event, void *context);
void baby_kicks_face_resign(void *context);
movement_watch_face_advisory_t baby_kicks_face_advise(void *context);

#define baby_kicks_face ((const watch_face_t) { \
    baby_kicks_face_setup, \
    baby_kicks_face_activate, \
    baby_kicks_face_loop, \
    baby_kicks_face_resign, \
    baby_kicks_face_advise, \
})
