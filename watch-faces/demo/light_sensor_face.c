/*
 * MIT License
 *
 * Copyright (c) 2024 Joey Castillo
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "light_sensor_face.h"
#include "lux_rx.h"
#include "tc.h"
#include "eic.h"
#include "usb.h"
#include "adc.h"

typedef enum {
    LIGHT_SENSOR_MODE_LEVEL,
    LIGHT_SENSOR_MODE_BINARY,
} light_sensor_mode_t;

static light_sensor_mode_t light_sensor_mode = LIGHT_SENSOR_MODE_LEVEL;
static uint16_t light_level;
static bool has_reading = false;

static void display_reading(void) {
    char buf[7];
    if (!has_reading) {
        watch_display_text(WATCH_POSITION_BOTTOM, "      ");
        return;
    }
    switch (light_sensor_mode) {
        case LIGHT_SENSOR_MODE_LEVEL:
            snprintf(buf, 7, "%-6d", light_level);
            watch_display_text(WATCH_POSITION_BOTTOM, buf);
            break;
        case LIGHT_SENSOR_MODE_BINARY:
            if (light_level < LUX_RX_BRIGHT_THRESHOLD) {
                watch_display_text(WATCH_POSITION_BOTTOM, "BRIGHT");
            } else {
                watch_display_text(WATCH_POSITION_BOTTOM, " DARK ");
            }
            break;
    }
}

static void display_mode_label(void) {
    switch (light_sensor_mode) {
        case LIGHT_SENSOR_MODE_LEVEL:
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "LEVEL", "LE");
            break;
        case LIGHT_SENSOR_MODE_BINARY:
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "BI nA", "BI");
            break;
    }
}

void light_sensor_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    (void) context_ptr;
}

static uint16_t take_light_reading(void) {
    HAL_GPIO_IR_ENABLE_out();
    HAL_GPIO_IR_ENABLE_clr();
    HAL_GPIO_IRSENSE_pmuxen(HAL_GPIO_PMUX_ADC);
    adc_init();
    adc_enable();
    uint16_t val = adc_get_analog_value(HAL_GPIO_IRSENSE_pin());
    adc_disable();
    HAL_GPIO_IRSENSE_pmuxdis();
    HAL_GPIO_IRSENSE_off();
    HAL_GPIO_IR_ENABLE_off();
    return val;
}

void light_sensor_face_activate(void *context) {
    (void) context;
    has_reading = false;
}

bool light_sensor_face_loop(movement_event_t event, void *context) {
    (void) context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            display_mode_label();
            display_reading();
            break;
        case EVENT_ALARM_BUTTON_UP:
            light_sensor_mode = (light_sensor_mode + 1) % 2;
            display_mode_label();
            display_reading();
            break;
        case EVENT_ALARM_LONG_PRESS:
            light_level = take_light_reading();
            has_reading = true;
            display_reading();
            break;
        case EVENT_LIGHT_BUTTON_UP:
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return false;
}

void light_sensor_face_resign(void *context) {
    (void) context;
    adc_disable();
    HAL_GPIO_IRSENSE_pmuxdis();
    HAL_GPIO_IRSENSE_off();
    HAL_GPIO_IR_ENABLE_off();
}

