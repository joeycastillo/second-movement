/*
 * MIT License
 *
 * Copyright (c) 2023 Wesley Ellis <https://github.com/tahnok>
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
#include "beats_face.h"
#include "watch.h"

const uint8_t BEAT_REFRESH_FREQUENCY = 8;

void beats_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    (void) context_ptr;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(beats_face_state_t));
    }
}

void beats_face_activate(void *context) {
    beats_face_state_t *state = (beats_face_state_t *)context;
    state->next_subsecond_update = 0;
    state->last_centibeat_displayed = 0;
    movement_request_tick_frequency(BEAT_REFRESH_FREQUENCY);
}

bool beats_face_loop(movement_event_t event, void *context) {
    beats_face_state_t *state = (beats_face_state_t *)context;
    if (event.event_type == EVENT_TICK && event.subsecond != state->next_subsecond_update) {
         return true; // math is hard, don't do it if we don't have to.
    }

    char buf[16];
    uint32_t centibeats;

    watch_date_time_t date_time;
    uint8_t bmt_hour; // BMT = Biel Mean Time
    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            date_time = movement_get_utc_date_time();
            bmt_hour = (date_time.unit.hour + 1) % 24;
            centibeats = clock2beats(bmt_hour, date_time.unit.minute, date_time.unit.second, event.subsecond);
            if (centibeats == state->last_centibeat_displayed) {
                // we missed this update, try again next subsecond
                state->next_subsecond_update = (event.subsecond + 1) % BEAT_REFRESH_FREQUENCY;
            } else {
                state->next_subsecond_update = (event.subsecond + 1 + (BEAT_REFRESH_FREQUENCY * 2 / 3)) % BEAT_REFRESH_FREQUENCY;
                state->last_centibeat_displayed = centibeats;
            }
            sprintf(buf, "%6lu", centibeats);

            watch_display_text_with_fallback(WATCH_POSITION_TOP, "beat", "bt");
            watch_display_text(WATCH_POSITION_BOTTOM, buf);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            if (!watch_sleep_animation_is_running()) watch_start_sleep_animation(432);
            date_time = movement_get_utc_date_time();
            bmt_hour = (date_time.unit.hour + 1) % 24;
            centibeats = clock2beats(bmt_hour, date_time.unit.minute, date_time.unit.second, event.subsecond);
            sprintf(buf, "%4lu  ", centibeats / 100);

            watch_display_text_with_fallback(WATCH_POSITION_TOP, "beat", "bt");
            watch_display_text(WATCH_POSITION_BOTTOM, buf);
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void beats_face_resign(void *context) {
    (void) context;
}

uint32_t clock2beats(uint32_t hours, uint32_t minutes, uint32_t seconds, uint32_t subseconds) {
    // Calculate total milliseconds since midnight
    uint32_t ms = (hours * 3600 + minutes * 60 + seconds) * 1000 + (subseconds * 1000) / BEAT_REFRESH_FREQUENCY;
    // 1 beat = 86.4 seconds = 86400 ms, so 1 centibeat = 864 ms
    uint32_t centibeats = ms / 864;
    centibeats %= 100000;
    return centibeats;
}
