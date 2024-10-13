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
#include "uart.h"

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
    HAL_GPIO_IRSENSE_in();
    HAL_GPIO_IRSENSE_pmuxen(HAL_GPIO_PMUX_SERCOM_ALT);
    uart_init_instance(0, UART_TXPO_NONE, UART_RXPO_0, 300);
    uart_set_irda_mode_instance(0, true);
    uart_enable_instance(0);
}

bool light_sensor_face_loop(movement_event_t event, void *context) {
    light_sensor_state_t *state = (light_sensor_state_t *)context;
    (void) state;

    switch (event.event_type) {
        case EVENT_NONE:
        case EVENT_ACTIVATE:
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "IrDA", "IR");
            // fall through
        case EVENT_TICK:
        {
            uint8_t data[32];
            size_t bytes_read = uart_read_instance(0, data, 32);
            if (bytes_read) {
                char buf[7];
                snprintf(buf, 7, "%3db r", bytes_read);
                watch_display_text(WATCH_POSITION_BOTTOM, buf);
                data[31] = 0;
                printf("%s: ", buf);
                // dump as hex
                for(size_t i = 0; i < bytes_read; i++) {
                    printf("%02X ", data[i]);
                }
                printf("\n");
            } else {
                watch_display_text(WATCH_POSITION_BOTTOM, "no dat");
            }
        }
            break;
        case EVENT_LIGHT_BUTTON_UP:
            // You can use the Light button for your own purposes. Note that by default, Movement will also
            // illuminate the LED in response to EVENT_LIGHT_BUTTON_DOWN; to suppress that behavior, add an
            // empty case for EVENT_LIGHT_BUTTON_DOWN.
            break;
        case EVENT_ALARM_BUTTON_UP:
            // Just in case you have need for another button.
            break;
        case EVENT_TIMEOUT:
            // Your watch face will receive this event after a period of inactivity. If it makes sense to resign,
            // you may uncomment this line to move back to the first watch face in the list:
            // movement_move_to_face(0);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            // If you did not resign in EVENT_TIMEOUT, you can use this event to update the display once a minute.
            // Avoid displaying fast-updating values like seconds, since the display won't update again for 60 seconds.
            // You should also consider starting the tick animation, to show the wearer that this is sleep mode:
            // watch_start_sleep_animation(500);
            break;
        default:
            // Movement's default loop handler will step in for any cases you don't handle above:
            // * EVENT_LIGHT_BUTTON_DOWN lights the LED
            // * EVENT_MODE_BUTTON_UP moves to the next watch face in the list
            // * EVENT_MODE_LONG_PRESS returns to the first watch face (or skips to the secondary watch face, if configured)
            // You can override any of these behaviors by adding a case for these events to this switch statement.
            return movement_default_loop_handler(event);
    }

    return false;
}

void light_sensor_face_resign(void *context) {
    (void) context;

    // handle any cleanup before your watch face goes off-screen.
}

void irq_handler_sercom0(void);
void irq_handler_sercom0(void) {
    uart_irq_handler(0);
}
#endif // HAS_IR_SENSOR
