/*
 * MIT License
 *
 * Copyright (c) 2022 Andreas Nebinger, based on Wesley Ellis’ countdown face.
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

#ifndef BABY_511_FACE_H_
#define BABY_511_FACE_H_

/*
 * TIMER face
 * Advanced timer/countdown face with pre-set timer lengths
 * 
 * This watch face provides the functionality of starting a countdown by choosing 
 * one out of nine programmable timer presets. A timer/countdown can be 23 hours,
 * 59 minutes, and 59 seconds max. A timer can also be set to auto-repeat, which
 * is indicated by the lap indicator.
 *
 * How to use in NORMAL mode:
 *   - Short-pressing the alarm button cycles through all pre-set timer lengths.
 *     Find the current timer slot number in the upper right-hand corner.
 *   - Long-pressing the alarm button starts the timer.
 *   - Long-pressing the light button initiates settings mode.
 * 
 * How to use in SETTINGS mode:
 *   - There are up to nine slots for storing a timer setting. The current slot is 
 *     indicated by the number in the upper right-hand corner.
 *   - Short-pressing the light button cycles through the settings values of each
 *     timer slot in the following order: hours - minutes - seconds - timer repeat
 *   - Short-pressing the alarm button alters the current settings value.
 *   - Long-pressing the light button resumes to normal mode.
 * 
 */

#include "movement.h"

#define MAX_LOGGED_CONTRACTIONS 40
#define MIN_SECS_PER_CONTRACTION 30
#define MINS_PER_HOUR 60
#define SECS_PER_MIN 60
#define SECS_TO_TRACK (SECS_PER_MIN * MINS_PER_HOUR)
#define CONTRACTION_GAP_THRESHOLD_SECS (5 * SECS_PER_MIN)

typedef enum {
    contracting,
    resting
} contraction_state_t;

typedef struct {
    uint32_t now_ts;
    contraction_state_t con_state;
    uint32_t last_con_start;
    uint8_t con_oldest;
    uint8_t con_newest;
    bool con_log_empty;
    uint32_t con_log[MAX_LOGGED_CONTRACTIONS];
    uint8_t watch_face_index;
} baby_state_t;

void baby_511_face_setup(uint8_t watch_face_index, void ** context_ptr);
void baby_511_face_activate(void *context);
bool baby_511_face_loop(movement_event_t event, void *context);
void baby_511_face_resign(void *context);

#define baby_511_face ((const watch_face_t){ \
    baby_511_face_setup, \
    baby_511_face_activate, \
    baby_511_face_loop, \
    baby_511_face_resign, \
    NULL, \
})


#endif // BABY_511_FACE_H_
