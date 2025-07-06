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

#include <stdlib.h>
#include <string.h>
#include "peek_memory_face.h"
#include "watch.h"
#include "sam.h"
#include "watch_utility.h"
#include "watch_common_display.h"

void peek_memory_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(peek_memory_state_t));
        peek_memory_state_t *state = (peek_memory_state_t *)*context_ptr;
        memset(*context_ptr, 0, sizeof(peek_memory_state_t));
#if __EMSCRIPTEN__
        // note: DOES NOT WORK IN SIMULATOR! Needs custom LCD to display hex
        static uint32_t dummy_value = 0x12345678;
        state->location = &dummy_value;
        state->format = PEEK_MEMORY_FORMAT_HEX;
#else
        state->location = (void *)&(RTC->MODE2.TIMESTAMP.reg);
        state->format = PEEK_MEMORY_FORMAT_DATE;
#endif
    }
}

void peek_memory_face_activate(void *context) {
    (void) context;
}

bool peek_memory_face_loop(movement_event_t event, void *context) {
    peek_memory_state_t *state = (peek_memory_state_t *)context;
    uint32_t value;
    watch_date_time_t *datetime;
    char buf[11];

    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            memcpy(&value, state->location, 4);
            switch (state->format) {
                case PEEK_MEMORY_FORMAT_HEX:
                    sprintf(buf, "%08lX", value);
                    watch_display_character('M', 0);
                    watch_display_character(buf[0], 1);
                    watch_display_character(buf[1], 10);
                    watch_display_character(buf[2], 2);
                    watch_display_character(buf[3], 3);
                    watch_display_character(buf[4], 4);
                    watch_display_character(buf[5], 5);
                    watch_display_character(buf[6], 6);
                    watch_display_character(buf[7], 7);
                    break;
                case PEEK_MEMORY_FORMAT_DATE:
                    datetime = (rtc_date_time_t *)&value;
                    sprintf(buf, "M%d", datetime->unit.month);
                    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, buf, buf + 1);
                    sprintf(buf, "%2d", datetime->unit.day);
                    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
                    sprintf(buf, "%02d%02d%02d", datetime->unit.hour, datetime->unit.minute, datetime->unit.second);
                    watch_display_text(WATCH_POSITION_BOTTOM, buf);
                    watch_set_colon();
                    break;
            }
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void peek_memory_face_resign(void *context) {
    (void) context;
}
