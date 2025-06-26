/*
 * MIT License
 *
 * Copyright (c) 2025 Daniel Bergman
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
#include "ish_face.h"
#include "watch.h"
#include "watch_utility.h"
#include <stdio.h>

// Minimum and maximum vagueness levels
#define ISH_LEVEL_MIN 1
#define ISH_LEVEL_MAX 3

// Check if the vague time should update based on the current minute
static bool ish_face_should_update(ish_face_state_t *state, watch_date_time_t date_time) {
    uint8_t current_minute = date_time.unit.minute;
    
    // Always update if this is a different minute than last displayed
    if (current_minute != state->last_displayed_minute) {
        state->last_displayed_minute = current_minute;
        return true;
    }
    
    return false;
}

// Updates the display with the current vague time and date
static void ish_face_update_display(ish_face_state_t *state, watch_date_time_t date_time) {
    char buf[8];
    uint8_t hour = date_time.unit.hour;
    uint8_t minute = date_time.unit.minute;
    // Support 12/24h mode
    if (movement_clock_mode_24h() == MOVEMENT_CLOCK_MODE_12H) {
        hour = hour % 12;
        if (hour == 0) hour = 12;
    }
    // Compute vague time string based on the current vagueness level
    switch (state->vagueness_level) {
        case 1: { // Level 1: Hour, switch at 30 minute mark
            if (minute >= 30) hour = (hour + 1) % 24;
            snprintf(buf, sizeof(buf), "%02d", hour);
            break;
        }
        case 2: { // Level 2: Half hour, explicit mapping, o instead of 0 to signify vagueness
            uint8_t h = hour;
            const char *min_str;
            if (minute < 15 || minute >= 45) {
                if (minute >= 45) h = (hour + 1) % 24;
                min_str = "0o";
            } else {
                min_str = "3o";
            }
            snprintf(buf, sizeof(buf), "%02d%s", h, min_str);
            break;
        }
        case 3: { // Level 3: Quarter hour, explicit mapping
            uint8_t h = hour;
            const char *min_str;
            if (minute < 8) {
                min_str = "00";
            } else if (minute < 23) {
                min_str = "15";
            } else if (minute < 38) {
                min_str = "30";
            } else if (minute < 53) {
                min_str = "45";
            } else {
                h = (hour + 1) % 24;
                min_str = "00";
            }
            snprintf(buf, sizeof(buf), "%02d%s", h, min_str);
            break;
        }
        default:
            snprintf(buf, sizeof(buf), "%02d", hour);
            break;
    }
    // Pad buf with spaces to 5 characters to clear leftover segments
    size_t len = strlen(buf);
    while (len < 5) buf[len++] = ' ';
    buf[len] = '\0';
    
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "ISH", "SH");
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
    watch_set_colon();
    watch_display_text(WATCH_POSITION_SECONDS, "  ");
}

// Start the tick-tock animation for low power mode
static void ish_face_start_tick_tock_animation(void) {
    if (!watch_sleep_animation_is_running()) {
        watch_start_sleep_animation(500);
        watch_start_indicator_blink_if_possible(WATCH_INDICATOR_COLON, 500);
    }
}

// Stop the tick-tock animation
static void ish_face_stop_tick_tock_animation(void) {
    if (watch_sleep_animation_is_running()) {
        watch_stop_sleep_animation();
        watch_stop_blink();
    }
}

// Initializes the face state, sets default vagueness level
void ish_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(ish_face_state_t));
        memset(*context_ptr, 0, sizeof(ish_face_state_t));
        ish_face_state_t *state = (ish_face_state_t *)*context_ptr;
        state->vagueness_level = 1; // Default to level 1 on initial load
        state->last_displayed_minute = 0xFF; // Force initial update
    }
}

// Called when the face is activated; forces a display update
void ish_face_activate(void *context) {
    ish_face_state_t *state = (ish_face_state_t *)context;
    state->last_displayed_minute = 0xFF; // Force update on activation
    watch_date_time_t date_time = movement_get_local_date_time();
    ish_face_update_display(state, date_time);
    // Start colon blink at 500ms interval
    watch_start_indicator_blink_if_possible(WATCH_INDICATOR_COLON, 500);
}

// Main event loop for the face
bool ish_face_loop(movement_event_t event, void *context) {
    ish_face_state_t *state = (ish_face_state_t *)context;
    switch (event.event_type) {
        case EVENT_TICK: {
            // Check for updates every second
            watch_date_time_t date_time = movement_get_local_date_time();
            if (ish_face_should_update(state, date_time)) {
                ish_face_update_display(state, date_time);
            }
            break;
        }
        case EVENT_LOW_ENERGY_UPDATE: {
            // Start tick-tock animation for low power mode
            ish_face_start_tick_tock_animation();
            // Check for updates in low energy mode
            watch_date_time_t date_time = movement_get_local_date_time();
            if (ish_face_should_update(state, date_time)) {
                ish_face_update_display(state, date_time);
            }
            break;
        }
        case EVENT_ALARM_BUTTON_UP: {
            // Cycle through vagueness levels 1→2→3→1
            state->vagueness_level++;
            if (state->vagueness_level > ISH_LEVEL_MAX) state->vagueness_level = ISH_LEVEL_MIN;
            state->last_displayed_minute = 0xFF; // Force update
            watch_date_time_t date_time = movement_get_local_date_time();
            ish_face_update_display(state, date_time);
            break;
        }
        default:
            // Use default handler for all other events
            return movement_default_loop_handler(event);
    }
    return true;
}

// No cleanup needed on resign
void ish_face_resign(void *context) {
    (void) context;
    // Stop colon blink and sleep animation when leaving the face
    ish_face_stop_tick_tock_animation();
}

