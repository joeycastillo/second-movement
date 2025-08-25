/*
 * MIT License
 *
 * Copyright (c) 2022 Joey Castillo
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

#include "pins.h"

/*
 * TEMPERATURE LOGGING (aka Temperature Log)
 *
 * This watch face automatically logs the temperature once an hour, and
 * maintains a 36-hour log of readings. This watch face is admittedly rather
 * complex, and bears some explanation.
 *
 * The face shows the current temperature readings, and refreshes every 5 seconds.
 * When showing the current temperature, it'll display TE (or TEMP on the custom display).
 * 
 * You can scroll through past temeprature readings by using the "Alarm" button to go to
 * a previous reading. The top left will now display TL (or LOG on the custom display).
 * At the top right, it displays the index of the reading; 0 represents the most recent
 * reading taken, 1 represents one hour earlier, etc. The bottom line in this mode displays
 * the logged temperature.
 *
 * reading; 0 represents the most recent reading taken, 1 represents one
 * hour earlier, etc. The bottom line in this mode displays the logged
 * temperature.
 *
 * A short press of the “Alarm” button advances to the next oldest reading;
 * you will see the number at the top right advance from 0 to 1 to 2, all
 * the way to 35, the oldest reading available.
 *
 * A short press of the "Light" button advances to the next newest reading.
 * Whena t the current reading, it'll advance to the oldest one.
 *
 * A long press of the “Alarm” button will change the watch's units from C to F if you're
 * looking at the current temperature. Otherwise, it will briefly display the timestamp
 * of the reading. The letters at the top left will display the word “At”,
 * and the main line will display the timestamp of the currently displayed
 * data point. The number in the top right will display the day of the month
 * for the given data point; for example, you can read “At 22 3:00 PM” as
 * ”At 3:00 PM on the 22nd”.
 *
 * If you need to illuminate the LED to read the data point, long press the
 * Light button and release it.
 */

#include "movement.h"
#include "watch.h"

#define TEMPERATURE_LOGGING_NUM_DATA_POINTS (36)

typedef struct {
    watch_date_time_t timestamp;
    float temperature_c;
} thermistor_logger_data_point_t;

typedef struct {
    uint8_t display_index;  // the index we are displaying on screen
    uint8_t ts_ticks;       // when the user taps the LIGHT button, we show the timestamp for a few ticks.
    int32_t data_points;    // the absolute number of data points logged
    thermistor_logger_data_point_t data[TEMPERATURE_LOGGING_NUM_DATA_POINTS];
} temperature_logging_state_t;

void temperature_logging_face_setup(uint8_t watch_face_index, void ** context_ptr);
void temperature_logging_face_activate(void *context);
bool temperature_logging_face_loop(movement_event_t event, void *context);
void temperature_logging_face_resign(void *context);
movement_watch_face_advisory_t temperature_logging_face_advise(void *context);

#define temperature_logging_face ((const watch_face_t){ \
    temperature_logging_face_setup, \
    temperature_logging_face_activate, \
    temperature_logging_face_loop, \
    temperature_logging_face_resign, \
    temperature_logging_face_advise, \
})
