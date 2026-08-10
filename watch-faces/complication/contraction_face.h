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

#ifndef CONTRACTION_FACE_H_
#define CONTRACTION_FACE_H_

/*
 * CONTRACTION face
 * Face for counting contractions through the stages of labor
 *
 * How to use:
 *   - Press ALARM when a contraction starts. Press again when contraction stops.
 *   - Long press ALARM to reset
 * 
 * Display:
 *   - Upper section displays current state "CON" or "CO" for contracting,
 *     "RES" or "RE" for resting.
 *   - Right side displays number of contractions over the past hour.
 *   - When Contracting
 *     +  Display shows duration of current contraction.
 *   - When Resting
 *     +  HH:MM digits show minutes and seconds spacing of contractions over
 *        the past hour.
 * 
 * Notes:
 *   - You will hear a special chime when contractions average 5 minutes apart
 *     and are 1 minute long for the past 1 hour. Following the "5-1-1 rule,"
 *     this means it's time to head to the hospital!
 *   - Contractions lasting less than 40 seconds are not counted. This deviation
 *     from the 1 minute in the rule is to allow some buffer in case you don't
 *     press the button right on time.
 *     These thresholds are adjustable in the constants below.
 */

#include "movement.h"

// Length of log
#define MAX_LOGGED_CONTRACTIONS 40

#define CONTRACTION_GAP_THRESHOLD_SECS (5 * SECS_PER_MIN) // 5 minutes
#define MIN_SECS_PER_VALID_CONTRACTION 40                 // 1 minute
#define SECS_TO_TRACK (SECS_PER_MIN * MINS_PER_HOUR)      // 1 hour


#define MINS_PER_HOUR 60
#define SECS_PER_MIN 60

typedef enum {
    contracting,
    resting
} contraction_status_t;

typedef struct {
    uint32_t now_ts;
    contraction_status_t con_state;
    uint32_t last_con_start;
    uint8_t con_oldest;
    uint8_t con_newest;
    bool con_log_empty;
    uint32_t con_log[MAX_LOGGED_CONTRACTIONS];
    uint8_t watch_face_index;
    bool chime_played;
} contraction_state_t;

void contraction_face_setup(uint8_t watch_face_index, void ** context_ptr);
void contraction_face_activate(void *context);
bool contraction_face_loop(movement_event_t event, void *context);
void contraction_face_resign(void *context);

#define contraction_face ((const watch_face_t){ \
    contraction_face_setup, \
    contraction_face_activate, \
    contraction_face_loop, \
    contraction_face_resign, \
    NULL, \
})


#endif // contraction_FACE_H_
