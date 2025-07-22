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
 * This watch face displays the current reading of the LIS2DW12 accelerometer.
 * The axis (x,y,z) can be selected using the alarm button. 
 * 
 * A long press on the light button allows to configure the sensor, including 
 * its mode, data rate, low power mode, bandwidth filtering, range, filter type,
 * and low noise mode.
 * 
 * The watch face is mainly designed for experimenting with the sensor and 
 * configuring it for other developing other watch faces.
 */

#include "movement.h"

typedef enum {
    PAGE_LIS2DW_MONITOR,
    PAGE_LIS2DW_SETTINGS,
} lis2dw_monitor_page_t;

typedef struct {
    lis2dw_mode_t mode;
    lis2dw_data_rate_t data_rate;
    lis2dw_low_power_mode_t low_power;
    lis2dw_bandwidth_filtering_mode_t bwf_mode;
    lis2dw_range_t range;
    lis2dw_filter_t filter;
    bool low_noise;
} lis2dw_device_state_t;

typedef struct {
    void (*display)(void *, uint8_t);
    void (*advance)(void *);
} lis2dw_settings_t;

typedef struct {
    uint8_t axis:2;             /* Axis to display */
    lis2dw_reading_t reading;   /* Current reading */
    lis2dw_monitor_page_t page; /* Displayed page */
    lis2dw_device_state_t ds;   /* Device state */
    uint8_t settings_page:3;    /* Subpage in settings */
    lis2dw_settings_t *settings;        /* Settings config */
    uint8_t show_title:6;       /* Display face title */
} lis2dw_monitor_state_t;

void lis2dw_monitor_face_setup(uint8_t watch_face_index, void **context_ptr);
void lis2dw_monitor_face_activate(void *context);
bool lis2dw_monitor_face_loop(movement_event_t event, void *context);
void lis2dw_monitor_face_resign(void *context);
movement_watch_face_advisory_t lis2dw_monitor_face_advise(void *context);

#define lis2dw_monitor_face ((const watch_face_t){ \
    lis2dw_monitor_face_setup, \
    lis2dw_monitor_face_activate, \
    lis2dw_monitor_face_loop, \
    lis2dw_monitor_face_resign, \
    lis2dw_monitor_face_advise, \
})
