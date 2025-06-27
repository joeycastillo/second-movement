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
#include "irda_upload_face.h"
#include "tc.h"
#include "eic.h"
#include "usb.h"
#include "uart.h"
#include "filesystem.h"

#ifdef HAS_IR_SENSOR

void irda_upload_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(irda_demo_state_t));
        memset(*context_ptr, 0, sizeof(irda_demo_state_t));
        // Do any one-time tasks in here; the inside of this conditional happens only at boot.
    }    
}

void irda_upload_face_activate(void *context) {
    irda_demo_state_t *state = (irda_demo_state_t *)context;
    (void) state;
    HAL_GPIO_IR_ENABLE_out();
    HAL_GPIO_IR_ENABLE_clr();
    HAL_GPIO_IRSENSE_in();
    HAL_GPIO_IRSENSE_pmuxen(HAL_GPIO_PMUX_SERCOM_ALT);
    uart_init_instance(0, UART_TXPO_NONE, UART_RXPO_0, 900);
    uart_set_irda_mode_instance(0, true);
    uart_enable_instance(0);
}

bool irda_upload_face_loop(movement_event_t event, void *context) {
    irda_demo_state_t *state = (irda_demo_state_t *)context;
    (void) state;

    switch (event.event_type) {
        case EVENT_NONE:
        case EVENT_ACTIVATE:
        case EVENT_TICK:
        {
            // need more work here: we seem to top out at 64 bytes coming in, and even then it takes a lot of tries.
            char data[256];
            size_t bytes_read = uart_read_instance(0, data, 256);
            watch_clear_display();
            watch_set_indicator(WATCH_INDICATOR_ARROWS);
            if (watch_rtc_get_date_time().unit.second % 4 < 2) watch_display_text_with_fallback(WATCH_POSITION_TOP, "IrDA", "IR");
            else watch_display_text_with_fallback(WATCH_POSITION_TOP, "FREE ", "DF");

            if (bytes_read) {
                // data is in the following format, where S is Size, F is Filename and C is checksum:
                // SSFFFFFFFFFFFFCC
                // after that, we get SS bytes of data, followed by a two-byte checksum
                uint16_t expected_size = ((uint16_t *)data)[0];
                uint16_t expected_checksum = ((uint16_t *)data)[7];

                uint16_t checksum = 0;

                for (size_t i = 0; i < 14; i++) checksum += data[i];

                if (checksum != expected_checksum) {
                    // failed
                    movement_force_led_on(48, 0, 0);
                    watch_display_text_with_fallback(WATCH_POSITION_TOP, "BAD  ", "BA");
                    watch_display_text(WATCH_POSITION_BOTTOM, "HEAdER");
                    break;
                }
                // valid header! To make it easier on ourselves, we're going to make byte 14 zero...
                data[14] = 0;
                // so that now buf[2] points to our null-terminated filename.
                char *filename = data + 2;

                if (expected_size == 0) {
                    // Success! All we need is a header to delete a file.
                    movement_force_led_on(0, 48, 0);
                    filesystem_rm(filename);
                    watch_display_text_with_fallback(WATCH_POSITION_TOP, "FILE ", "FI");
                    watch_display_text_with_fallback(WATCH_POSITION_TOP, "dELETE", " deLet");
                    break;
                }

                // now let's check the data.
                checksum = 0;
                memcpy(&expected_checksum, data + 16 + expected_size, 2);
                for (uint16_t i = 16; i < 16 + expected_size; i++) checksum += data[i];

                if (checksum != expected_checksum) {
                    // failed
                    movement_force_led_on(48, 0, 0);
                    watch_display_text_with_fallback(WATCH_POSITION_TOP, "BAD  ", "BA");
                    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "data  ", " data ");
                    break;
                }

                // Valid data! Write it to the file system.
                filesystem_write_file(filename, data + 16, expected_size);
                watch_display_text_with_fallback(WATCH_POSITION_TOP, "RECVd", "RC");
                movement_force_led_on(0, 48, 0);

                char buf[8];
                sprintf(buf, "%4db ", expected_size);
                watch_display_text(WATCH_POSITION_BOTTOM, buf);
            } else {
                movement_force_led_off();
                char buf[7];
                snprintf(buf, 7, "%4ld b", filesystem_get_free_space());
                watch_display_text(WATCH_POSITION_BOTTOM, buf);
            }
        }
            break;
        case EVENT_LIGHT_BUTTON_UP:
            break;
        case EVENT_ALARM_BUTTON_UP:
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            watch_display_text(WATCH_POSITION_TOP_RIGHT, " <");
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return false;
}

void irda_upload_face_resign(void *context) {
    (void) context;
    uart_disable_instance(0);
    HAL_GPIO_IRSENSE_pmuxdis();
    HAL_GPIO_IRSENSE_off();
    HAL_GPIO_IR_ENABLE_off();
}

void irq_handler_sercom0(void);
void irq_handler_sercom0(void) {
    uart_irq_handler(0);
}
#endif // HAS_IR_SENSOR
