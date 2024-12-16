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
#include "file_demo_face.h"
#include "tc.h"
#include "eic.h"
#include "usb.h"
#include "uart.h"
#include "filesystem.h"

#ifdef HAS_IR_SENSOR

void file_demo_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(file_demo_state_t));
        memset(*context_ptr, 0, sizeof(file_demo_state_t));
        // Do any one-time tasks in here; the inside of this conditional happens only at boot.
    }    
}

void file_demo_face_activate(void *context) {
    file_demo_state_t *state = (file_demo_state_t *)context;
    (void) state;
}

bool file_demo_face_loop(movement_event_t event, void *context) {
    file_demo_state_t *state = (file_demo_state_t *)context;
    (void) state;
    char buf[7];

    switch (event.event_type) {
        case EVENT_NONE:
        case EVENT_ACTIVATE:
REDISPLAY:
            watch_clear_display();
            // display the contents of a file called "1TEST"
            if (filesystem_file_exists("1TEST")) {
                filesystem_read_file("1TEST", buf, sizeof(buf));
                buf[6] = 0;
                watch_display_text_with_fallback(WATCH_POSITION_TOP, "TF", "TST F");
                watch_display_text(WATCH_POSITION_BOTTOM, buf);
            } else {
                watch_display_text(WATCH_POSITION_FULL, "NO   FILE ");
            }
            break;
        case EVENT_TICK:
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            state->delete_enabled = false;
            goto REDISPLAY;
            break;
        case EVENT_ALARM_BUTTON_UP:
            if (state->delete_enabled) {
                filesystem_rm("1TEST");
                movement_force_led_off();
                state->delete_enabled = false;
                goto REDISPLAY;
            } else if (filesystem_file_exists("1TEST")) {
                movement_force_led_on(255, 0, 0);
                watch_clear_display();
                watch_display_text(WATCH_POSITION_BOTTOM, "0ElET?");
                state->delete_enabled = true;
            }
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

void file_demo_face_resign(void *context) {
    file_demo_state_t *state = (file_demo_state_t *)context;

    movement_force_led_off();
    state->delete_enabled = false;
}

#endif // HAS_IR_SENSOR
