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
 * THERMISTOR READOUT (aka Temperature Display)
 *
 * This watch face is designed to work with either the Temperature + GPIO
 * sensor board or the Temperature + Light sensor board. It reads the current
 * temperature from the thermistor voltage divider on the sensor board, and
 * displays the current temperature in degrees Celsius.
 *
 * When the watch is on your wrist, your body heat interferes with an ambient
 * temperature reading, but if you set it on a bedside table, strap it to your
 * bike handlebars or place it outside of your tent while camping, this watch
 * face can act as a digital thermometer for displaying ambient conditions.
 *
 * The temperature sensor watch face automatically samples the temperature
 * once every five seconds, and it illuminates the Signal indicator just
 * before taking a reading.
 *
 * Pressing the ALARM button toggles the unit display from Celsius to
 * Fahrenheit. Technically this sets the global “Metric / Imperial” flag, so
 * any other watch face that displays localizable units will display them in
 * the system selected here.
 */

#include "movement.h"

void temperature_display_face_setup(uint8_t watch_face_index, void ** context_ptr);
void temperature_display_face_activate(void *context);
bool temperature_display_face_loop(movement_event_t event, void *context);
void temperature_display_face_resign(void *context);

#define temperature_display_face ((const watch_face_t){ \
    temperature_display_face_setup, \
    temperature_display_face_activate, \
    temperature_display_face_loop, \
    temperature_display_face_resign, \
    NULL, \
})
