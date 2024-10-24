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
#include "irda_demo_face.h"
#include "tc.h"
#include "eic.h"
#include "usb.h"
#include "uart.h"

#ifdef HAS_IR_SENSOR

void irda_demo_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(irda_demo_state_t));
        memset(*context_ptr, 0, sizeof(irda_demo_state_t));
        // Do any one-time tasks in here; the inside of this conditional happens only at boot.
    }    
}

void irda_demo_face_activate(void *context) {
    irda_demo_state_t *state = (irda_demo_state_t *)context;
    (void) state;
    // Rev 05 had the polarity reversed, so we had to bodge a different circuit and bypass the enable pin.
    // HAL_GPIO_IR_ENABLE_out();
    // HAL_GPIO_IR_ENABLE_clr();
    HAL_GPIO_IRSENSE_in();
    HAL_GPIO_IRSENSE_pmuxen(HAL_GPIO_PMUX_SERCOM_ALT);
    uart_init_instance(0, UART_TXPO_NONE, UART_RXPO_0, 300);
    uart_set_irda_mode_instance(0, true);
    uart_enable_instance(0);
}

bool irda_demo_face_loop(movement_event_t event, void *context) {
    irda_demo_state_t *state = (irda_demo_state_t *)context;
    (void) state;

    switch (event.event_type) {
        case EVENT_NONE:
        case EVENT_ACTIVATE:
        case EVENT_TICK:
        {
            uint8_t data[32];
            size_t bytes_read = uart_read_instance(0, data, 32);
            if (bytes_read) {
                char buf[14];
                snprintf(buf, 11, "IR%2d%c%c%c%c%c%c", bytes_read, data[0], data[1], data[2], data[3], data[4], data[5]);
                watch_display_text(WATCH_POSITION_FULL, buf);
                data[31] = 0;
                printf("%s\n", data);
                printf("%s\n", buf);
            } else {
                watch_display_text(WATCH_POSITION_FULL, "    no dat");
            }
        }
            break;
        case EVENT_LIGHT_BUTTON_UP:
            break;
        case EVENT_ALARM_BUTTON_UP:
            break;
        case EVENT_TIMEOUT:
            // movement_move_to_face(0);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            watch_display_text(WATCH_POSITION_TOP_RIGHT, " <");
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return false;
}

void irda_demo_face_resign(void *context) {
    (void) context;
    uart_disable_instance(0);
    HAL_GPIO_IRSENSE_pmuxdis();
    HAL_GPIO_IRSENSE_off();
}

void irq_handler_sercom0(void);
void irq_handler_sercom0(void) {
    uart_irq_handler(0);
}
#endif // HAS_IR_SENSOR
