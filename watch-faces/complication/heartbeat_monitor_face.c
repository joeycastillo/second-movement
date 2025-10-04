/*
 * MIT License
 *
 * Copyright (c) 2025 spipm
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
#include "heartbeat_monitor_face.h"
#include "watch.h"

void heartbeat_monitor_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(heartbeat_monitor_state_t));
        memset(*context_ptr, 0, sizeof(heartbeat_monitor_state_t));
    }
}

void heartbeat_monitor_face_activate(void *context) {
    heartbeat_monitor_state_t *state = (heartbeat_monitor_state_t *)context;
    state->tap_count = 0;
    state->bpm = 0;
    
    // Request fast tick rate for subsecond precision
    movement_request_tick_frequency(8); // 8Hz = 125ms precision
    
    watch_display_text(WATCH_POSITION_FULL, "Hr");
}

// Get timestamp in milliseconds
static uint32_t get_timestamp_ms(movement_event_t event) {
    watch_date_time_t dt = movement_get_local_date_time();
    uint32_t seconds = dt.unit.second + 
                       dt.unit.minute * 60 + 
                       dt.unit.hour * 3600;
    uint32_t ms = seconds * 1000 + (event.subsecond * 125); // 8Hz = 125ms per tick
    return ms;
}

// Get current age from birth year
static uint8_t get_age(void) {
    watch_date_time_t dt = movement_get_local_date_time();
    // The Sensor Watch stores the date as reference year (2020) + 2 digit
    uint16_t current_year = dt.unit.year + WATCH_RTC_REFERENCE_YEAR;
    return current_year - BIRTH_YEAR;
}

// Determine heart rate zone
static char get_zone(uint8_t bpm, uint8_t age) {
    uint8_t max_hr = 220 - age;
    
    if (bpm < max_hr * LOW_RATE_UPPER       / 100) return 'L'; // Low
    if (bpm < max_hr * CENTER_RATE_UPPER    / 100) return 'C'; // Medium (center, because no 'M')
    return 'H'; // High
}

bool heartbeat_monitor_face_loop(movement_event_t event, void *context) {
    heartbeat_monitor_state_t *state = (heartbeat_monitor_state_t *)context;
    char buf[20];
    
    switch (event.event_type) {
        case EVENT_ALARM_BUTTON_UP:
            if (state->tap_count < 4) {
                // Record taps
                state->tap_times[state->tap_count] = get_timestamp_ms(event);
                state->tap_count++;
                
                sprintf(buf, "Tap %d", state->tap_count);
                watch_display_text(WATCH_POSITION_FULL, buf);
                
                // Calculate BPM after 4 taps
                if (state->tap_count == 4) {
                    uint32_t total_ms = 0;
                    for (int i = 1; i < 4; i++) {
                        total_ms += state->tap_times[i] - state->tap_times[i - 1];
                    }
                    uint32_t avg_ms = total_ms / 3;
                    
                    if (avg_ms > 0) {
                        state->bpm = 60000 / avg_ms; // 60000 ms/min / avg_interval_ms
                        uint8_t age = get_age();
                        char zone = get_zone(state->bpm, age);
                        sprintf(buf, "%d %c", state->bpm, zone);
                        watch_display_text(WATCH_POSITION_FULL, buf);
                    }
                    
                    state->tap_count = 0; // Reset
                }
            }
            break;
            
        case EVENT_LIGHT_BUTTON_UP:
            // Reset
            state->tap_count = 0;
            state->bpm = 0;
            watch_display_text(WATCH_POSITION_FULL, "Hr");
            break;
            
        default:
            return movement_default_loop_handler(event);
    }
    
    return true;
}

void heartbeat_monitor_face_resign(void *context) {
    (void) context;
    movement_request_tick_frequency(1);
}