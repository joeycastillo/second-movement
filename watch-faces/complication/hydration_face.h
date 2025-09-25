/*
 * MIT License
 *
 * Copyright (c) 2025 Konrad Rieck
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

/*
 * This watch face tracks daily water intake.
 * 
 * In tracking mode: Display current water intake and percentage
 * - Alarm button: +100ml (or configured glass size)
 * - Light button: -100ml (or configured glass size)
 * - Alarm long press: Display deviation from estimate
 * - Light long press: Switch to settings mode
 * 
 * In settings mode: Configure glass size, daily goal, wake time, sleep time, alert interval
 * - Light button: Switch to next setting
 * - Alarm button: Advance current setting
 * - Alarm long press: Reset to default value
 * - Mode button: Switch to tracking mode
 *
 * Background tasks:
 * - Automatic reset at wake time
 * - Alert if intake below estimate at sleep time
 * - Alert at interval if intake below estimate
 *
 */

#include "movement.h"

typedef enum {
    PAGE_HYDRATION_TRACKING,
    PAGE_HYDRATION_SETTINGS,
} hydration_page_t;

typedef enum {
    HYDRATION_SETTING_WATER_GLASS,
    HYDRATION_SETTING_WATER_GOAL,
    HYDRATION_SETTING_WAKE_TIME,
    HYDRATION_SETTING_SLEEP_TIME,
    HYDRATION_SETTING_ALERT_INTERVAL,
} hydration_setting_t;

typedef struct {
    void (*display)(void *, uint8_t);
    void (*advance)(void *);
} hydration_settings_t;

typedef struct {
    uint16_t water_intake;      /* Current water intake in ml */
    uint16_t water_glass;       /* Glass size in ml (default 100ml) */
    uint16_t water_goal;        /* Target daily water intake (default 2000ml) */
    uint8_t wake_hour;          /* Hour for wake time (default 7) */
    uint8_t sleep_hour;         /* Hour for sleep time (default 22) */
    uint8_t alert_interval;     /* Alert interval in hours (default 2) */
    uint8_t face_index;         /* Face index */
    uint8_t display_deviation;  /* Display deviation from estimate */
    hydration_page_t page;      /* Current page */
    hydration_setting_t settings_page;  /* Current settings page */
    hydration_settings_t *settings;     /* Settings configuration */
} hydration_state_t;

void hydration_face_setup(uint8_t watch_face_index, void **context_ptr);
void hydration_face_activate(void *context);
bool hydration_face_loop(movement_event_t event, void *context);
void hydration_face_resign(void *context);
movement_watch_face_advisory_t hydration_face_advise(void *context);

#define hydration_face ((const watch_face_t){ \
    hydration_face_setup, \
    hydration_face_activate, \
    hydration_face_loop, \
    hydration_face_resign, \
    hydration_face_advise, \
})
