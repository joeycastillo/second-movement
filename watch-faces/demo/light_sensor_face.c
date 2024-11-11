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
#include "light_sensor_face.h"
#include "tc.h"
#include "eic.h"
#include "usb.h"
#include "adc.h"

#ifdef HAS_IR_SENSOR

void light_sensor_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(light_sensor_state_t));
        memset(*context_ptr, 0, sizeof(light_sensor_state_t));
        // Do any one-time tasks in here; the inside of this conditional happens only at boot.
    }    
}

void light_sensor_face_activate(void *context) {
    light_sensor_state_t *state = (light_sensor_state_t *)context;
    (void) state;
    HAL_GPIO_IR_ENABLE_out();
    HAL_GPIO_IR_ENABLE_clr();
    HAL_GPIO_IRSENSE_pmuxen(HAL_GPIO_PMUX_ADC);
    adc_init();
    adc_enable();
    movement_request_tick_frequency(8);
}

bool light_sensor_face_loop(movement_event_t event, void *context) {
    light_sensor_state_t *state = (light_sensor_state_t *)context;
    (void) state;

    switch (event.event_type) {
        case EVENT_NONE:
        case EVENT_ACTIVATE:
        case EVENT_TICK:
        {
            char buf[14];
            uint16_t light_level = adc_get_analog_value(HAL_GPIO_IRSENSE_pin());
            snprintf(buf, 14, "LL  %-6d", light_level);
            watch_display_text(WATCH_POSITION_FULL, buf);
            printf("%s\n", buf);
        }
            break;
        case EVENT_LIGHT_BUTTON_UP:
            break;
        case EVENT_ALARM_BUTTON_UP:
            break;
        case EVENT_TIMEOUT:
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            watch_display_text(WATCH_POSITION_TOP_RIGHT, " <");
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

#endif // HAS_IR_SENSOR
