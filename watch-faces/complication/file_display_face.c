/*
 * MIT License
 *
 * Copyright (c) 2024-2025 Joey Castillo
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
#include "file_display_face.h"
#include "tc.h"
#include "eic.h"
#include "usb.h"
#include "uart.h"
#include "filesystem.h"

#ifdef HAS_IR_SENSOR

void file_display_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(file_demo_state_t));
        memset(*context_ptr, 0, sizeof(file_demo_state_t));
        // active_file is now 0 for FILE0000.TXT
    }    
}

void file_display_face_activate(void *context) {
    file_demo_state_t *state = (file_demo_state_t *)context;
    (void) state;
}

bool file_display_face_loop(movement_event_t event, void *context) {
    file_demo_state_t *state = (file_demo_state_t *)context;
    (void) state;
    char data[13] = {0};
    char filename[13];

    switch (event.event_type) {
        case EVENT_NONE:
        case EVENT_ACTIVATE:
REDISPLAY:
            watch_clear_display();
            sprintf(filename, "FILE%04d.TXT", state->active_file);
            if (filesystem_file_exists(filename)) {
                filesystem_read_file(filename, data, sizeof(data) - 1);
                watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, data + 0, data + 0);
                watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, data + 3, data + 3);
                watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, data + 5, data + 5);
                // Treat last byte as flag for indicators
                for (int i = 0; i < 8; i++) {
                    bool indicator = (data[11] >> i) & 1;
                    if (indicator) {
                        switch (i) {
                        case 0:
                            // bit 0 lights up the bell indicator
                            watch_set_indicator(WATCH_INDICATOR_BELL);
                            break;
                        case 1:
                            // bit 1 sets the PM indicator
                            watch_set_indicator(WATCH_INDICATOR_PM);
                            break;
                        case 2:
                            // bit 2 sets the colon
                            watch_set_colon();
                            break;
                        case 3:
                            // bit 3 sets the LAP indicator
                            watch_set_indicator(WATCH_INDICATOR_LAP);
                            break;
                        case 4:
                            // bit 5 sets the decimal point on the custom LCD
                            watch_set_decimal_if_available();
                            break;
                        case 6:
                            // bit 6 sets the leading 1 on the custom LCD
                            if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
                                watch_set_pixel(0, 22);
                            }
                            break;
                        case 7:
                        case 5:
                        default:
                            // bit 5 must be set to make this a valid ASCII character (>= 0x20)
                            // bit 7 also must be unset
                            // neither of these bits correspond to an indicator.
                            break;
                        }
                    }
                }
            } else {
                watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "No ", "NO");
                watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "F{es", " FILES");
            }
            break;
        case EVENT_TICK:
            break;
        case EVENT_ALARM_BUTTON_UP:
            state->active_file++;
            sprintf(filename, "FILE%04d.TXT", state->active_file);
            if (!filesystem_file_exists(filename)) {
                state->active_file = 0;
            }
            goto REDISPLAY;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            // we should resign before this ever happens
            if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
                watch_set_indicator(WATCH_INDICATOR_SLEEP);
            } else {
                watch_clear_display();
                watch_display_text(WATCH_POSITION_BOTTOM, "SLEEP ");
            }
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return false;
}

void file_display_face_resign(void *context) {
    file_demo_state_t *state = (file_demo_state_t *)context;

    movement_force_led_off();
    state->active_file = 0;
}

#endif // HAS_IR_SENSOR
