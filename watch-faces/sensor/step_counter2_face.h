/*
 * MIT License
 *
 * Copyright (c) 2025, 2026 Konrad Rieck
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
 * Step Counter 2 Face
 *
 * An alternative step counter for Sensor Watch. Steps are detected with a
 * simple heuristic over the LIS2DW accelerometer data, which is faster than
 * common step-counting libraries. A candidate peak has to clear an amplitude
 * threshold, rise slowly enough to look like a stride rather than a flick of
 * the wrist, and belong to a run of rhythmic steps. The counter runs in the
 * background and pauses tracking when there is no movement. For now it only
 * tracks the current step count and does not keep a history. Sampling runs at
 * 25 Hz, which gave significantly better results than lower rates; the
 * rise-time test in particular does not survive a lower rate.
 *
 * The design goal is to keep changes to the movement core small and build the
 * feature mostly as a watch face. Background counting is done by chaining
 * scheduled tasks, one per second, and movement recognition goes through the
 * existing movement API.
 *
 * For full operation the face relies on the wake-on-motion feature. Its calls
 * are guarded so the face still builds and counts steps without it. Without
 * wake-on-motion the watch does not wake from sleep on motion, so movement that
 * starts during sleep is missed until the watch wakes for some other reason.
 */

#include "movement.h"

/* Main pages of the watch face */
typedef enum {
    PAGE_COUNTER,
    PAGE_SETTINGS,
} step_counter2_page_t;

/* Settings pages, in the order they are built in setup() */
typedef enum {
    STEP_COUNTER2_SETTING_DAILY_GOAL,
    STEP_COUNTER2_SETTING_THRESHOLD,
    STEP_COUNTER2_SETTING_MIN_STEP,
    STEP_COUNTER2_SETTING_MAX_STEP,
    STEP_COUNTER2_SETTING_MIN_STREAK,
    STEP_COUNTER2_SETTING_MAX_RISE,
} step_counter2_setting_t;

/* Length of the rise-time window, in samples, and the history needed to form
 * its differences. Unlike the other parameters this one is fixed at compile
 * time: it measures the rise of the step impulse itself and so follows the
 * sample rate, not the cadence. */
#define STEP_COUNTER2_RISE_WIN 7
#define STEP_COUNTER2_RISE_HIST (STEP_COUNTER2_RISE_WIN + 1)

/* Setting display and advance functions */
typedef struct {
    void (*display)(void *, uint8_t);
    void (*advance)(void *);
} step_counter2_settings_t;

typedef struct {
    step_counter2_page_t page;   /* Displayed page */
    step_counter2_settings_t *settings;  /* Settings config */
    uint8_t settings_page:3;    /* Subpage in settings */

    /* Step detection data */
    uint32_t steps;             /* Number of confirmed steps taken */
    uint32_t subticks;          /* Subticks for timing */
    uint32_t last_step;         /* Subtick of the previous detected step */
    bool have_last_step;        /* Whether last_step holds a valid step */
    uint16_t streak_len;        /* Length of the current rhythmic streak */
    uint8_t mag_hist[STEP_COUNTER2_RISE_HIST];  /* Recent magnitudes, for the rise gate */
    uint8_t mag_idx;            /* Ring buffer write position, and thus oldest entry */

    /* Steps that make a day */
    uint16_t daily_goal;        /* Daily step goal */

    /* Parameters for step detection */
    uint16_t threshold;         /* Threshold for step detection */
    uint8_t min_step;           /* Minimum time between two steps */
    uint8_t max_step;           /* Maximum gap between two steps in a streak */
    uint8_t min_streak;         /* Required streak of consecutive steps to count */
    uint8_t max_rise;           /* Max climb into a peak, percent of peak height */

    /* Day-of-month of the last daily reset (0 until first tick) */
    uint8_t last_reset_day;

    /* Background counting */
    uint8_t watch_face_index;   /* Our face index, for scheduling background tasks */
    bool is_foreground;         /* True while the face is on screen */
    bool background_armed;      /* True while a 1 Hz background drain is scheduled */
    uint32_t drain_target;      /* Unix time the current background drain is scheduled for */
} step_counter2_state_t;

void step_counter2_face_setup(uint8_t watch_face_index, void **context_ptr);
void step_counter2_face_activate(void *context);
bool step_counter2_face_loop(movement_event_t event, void *context);
void step_counter2_face_resign(void *context);
movement_watch_face_advisory_t step_counter2_face_advise(void *context);

#define step_counter2_face ((const watch_face_t){ \
    step_counter2_face_setup, \
    step_counter2_face_activate, \
    step_counter2_face_loop, \
    step_counter2_face_resign, \
    step_counter2_face_advise, \
})
