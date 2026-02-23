/*
 * MIT License
 *
 * Copyright (c) 2026 Diego Perez
 */

#include <stdlib.h>
#include <string.h>
#include "emergence_face.h"

#ifdef PHASE_ENGINE_ENABLED
#include "../../lib/metrics/metrics.h"
#include "../../lib/phase/zone_words.h"

// Forward declarations
extern void metrics_get(const metrics_engine_t *engine, metrics_snapshot_t *out);
extern movement_state_t movement_state;

static void _emergence_face_update_display(emergence_face_state_t *state) {
    char buf[11] = {0};
    
    // Get current metrics
    metrics_snapshot_t metrics = {0};
    metrics_get(NULL, &metrics);
    
    // Display based on mode
    switch (state->display_mode) {
        case 0:  // Word 1
            watch_display_text(WATCH_POSITION_TOP, "          ");  // Clear top
            watch_display_text(WATCH_POSITION_BOTTOM, state->selected_words[0]);
            break;
            
        case 1:  // Word 2
            watch_display_text(WATCH_POSITION_TOP, "          ");  // Clear top
            watch_display_text(WATCH_POSITION_BOTTOM, state->selected_words[1]);
            break;
            
        case 2: {  // Stats: "ER     Lv2" (top) + "EM  45" (bottom)
            word_level_t level = zone_words_get_level(metrics.em);
            
            // Top row: ER (positions 0-1) + Lv2 (positions 2-3)
            snprintf(buf, sizeof(buf), "ER     Lv%d", level);
            watch_display_text(WATCH_POSITION_TOP, buf);
            
            // Bottom row: EM + metric value
            snprintf(buf, sizeof(buf), "EM  %d      ", metrics.em);
            watch_display_text(WATCH_POSITION_BOTTOM, buf);
            break;
        }
            
        default:
            state->display_mode = 0;
            watch_display_text(WATCH_POSITION_TOP, "          ");
            watch_display_text(WATCH_POSITION_BOTTOM, state->selected_words[0]);
            break;
    }
}

void emergence_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(emergence_face_state_t));
        memset(*context_ptr, 0, sizeof(emergence_face_state_t));
    }
}

void emergence_face_activate(void *context) {
    emergence_face_state_t *state = (emergence_face_state_t *)context;
    state->display_mode = 0;  // Start with word 1
    
    // Get current date for global streak tracking
    watch_date_time date_time = watch_rtc_get_date_time();
    // Calculate day of year (simplified: assumes 30-day months)
    uint16_t day_of_year = (date_time.unit.month - 1) * 30 + date_time.unit.day;
    
    // Update global zone check streak
    if (movement_state.phase.zone_last_check_day != day_of_year) {
        // New day - check if consecutive
        uint16_t prev_day = movement_state.phase.zone_last_check_day;
        movement_state.phase.zone_last_check_day = day_of_year;
        
        // Consecutive check (doesn't handle year rollover perfectly, but good enough)
        if (day_of_year == prev_day + 1 || prev_day == 0) {
            // Consecutive day or first activation
            movement_state.phase.zone_check_streak++;
        } else {
            // Streak broken, reset
            movement_state.phase.zone_check_streak = 1;
        }
    }
    
    // Get current EM metric for word level
    metrics_snapshot_t metrics = {0};
    metrics_get(NULL, &metrics);
    word_level_t level = zone_words_get_level(metrics.em);
    
    // Generate random seed from current time
    uint32_t seed = (date_time.reg << 16) | (date_time.unit.hour << 8) | date_time.unit.minute;
    
    // Select words for this activation
    zone_words_select(ZONE_EM, level, seed, 
                     state->selected_words[0], 
                     state->selected_words[1]);
}

bool emergence_face_loop(movement_event_t event, void *context) {
    emergence_face_state_t *state = (emergence_face_state_t *)context;
    
    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            _emergence_face_update_display(state);
            break;
            
        case EVENT_ALARM_BUTTON_UP:
            // Cycle through display modes: word1 → word2 → stats → word1
            state->display_mode = (state->display_mode + 1) % 3;
            _emergence_face_update_display(state);
            break;
        
        case EVENT_MODE_LONG_PRESS:
            // Long-press MODE → exit playlist mode, return to clock
            movement_move_to_face(1);  // Clock face (index 1)
            break;
            
        case EVENT_LIGHT_BUTTON_UP:
            movement_illuminate_led();
            break;
            
        case EVENT_TIMEOUT:
            movement_move_to_face(1);  // Timeout → return to clock
            break;
            
        default:
            return movement_default_loop_handler(event);
    }
    
    return true;
}

void emergence_face_resign(void *context) {
    (void) context;
    // Nothing to clean up
}

#endif // PHASE_ENGINE_ENABLED
