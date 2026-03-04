/*
 * MIT License
 *
 * Copyright (c) 2026 Diego Perez
 */

#include <stdlib.h>
#include <string.h>
#include "minimal_phase_face.h"

#ifdef PHASE_ENGINE_ENABLED
#include "../../lib/phase/phase_engine.h"
#endif

static void _minimal_phase_face_update_display(minimal_phase_state_t *state) {
    char buf[11];
    memset(buf, 0, sizeof(buf));
    
#ifdef PHASE_ENGINE_ENABLED
    watch_date_time now = watch_rtc_get_date_time();
    uint8_t hour = now.unit.hour;
    uint16_t day_of_year = movement_get_day_of_year(now);
    
    // Get sensor inputs - use defaults if sensors unavailable
    // TODO: Integrate with actual sensors when available
    uint16_t activity_level = 500;    // Moderate activity default
    int16_t temp_c10 = 200;           // 20.0°C default
    uint16_t light_lux = 100;         // Dim indoor light default
    
    // Compute current phase score
    uint16_t phase_score = phase_compute(&state->phase,
                                         hour,
                                         day_of_year,
                                         activity_level,
                                         temp_c10,
                                         light_lux);
    
    switch (state->mode) {
        case PHASE_MODE_SCORE:
            // Display current phase score
            sprintf(buf, "PH  %3d   ", (int)phase_score);
            break;
            
        case PHASE_MODE_TREND:
        {
            // Display 6-hour trend
            int16_t trend = phase_get_trend(&state->phase, 6);
            char sign = (trend >= 0) ? '+' : '-';
            sprintf(buf, "TR %c%02d   ", sign, abs(trend));
            break;
        }
        
        case PHASE_MODE_RECOMMENDATION:
        {
            // Get active hours configuration
            movement_active_hours_t active_hours = movement_get_active_hours();
            uint8_t active_start = active_hours.bit.start_quarter_hours / 4;
            uint8_t active_end = active_hours.bit.end_quarter_hours / 4;
            
            // Display recommendation
            uint8_t rec = phase_get_recommendation(phase_score, hour,
                                                   active_hours.bit.enabled,
                                                   active_start, active_end);
            const char *rec_str[] = {"RST", "MOD", "ACT", "PEK"};
            sprintf(buf, "RC %s   ", rec_str[rec]);
            break;
        }
        
        default:
            sprintf(buf, "PH  --    ");
            break;
    }
#else
    // Phase engine not enabled - show error
    sprintf(buf, "PH  --    ");
#endif
    
    watch_display_string(buf, 0);
}

void minimal_phase_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void **context_ptr) {
    (void) settings;
    (void) watch_face_index;
    
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(minimal_phase_state_t));
        minimal_phase_state_t *state = (minimal_phase_state_t *)*context_ptr;
        memset(state, 0, sizeof(minimal_phase_state_t));
        
#ifdef PHASE_ENGINE_ENABLED
        phase_engine_init(&state->phase);
#endif
        state->mode = PHASE_MODE_SCORE;
        state->last_minute = 0xFF; // Force initial update
    }
}

void minimal_phase_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    minimal_phase_state_t *state = (minimal_phase_state_t *)context;
    
    // Force display update on activation
    state->last_minute = 0xFF;
    _minimal_phase_face_update_display(state);
}

bool minimal_phase_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    (void) settings;
    minimal_phase_state_t *state = (minimal_phase_state_t *)context;
    
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _minimal_phase_face_update_display(state);
            break;
            
        case EVENT_TICK:
        {
            watch_date_time now = watch_rtc_get_date_time();
            
            // Update display once per minute
            if (now.unit.minute != state->last_minute) {
                state->last_minute = now.unit.minute;
                _minimal_phase_face_update_display(state);
            }
            break;
        }
        
        case EVENT_ALARM_BUTTON_UP:
            // Cycle through display modes
            state->mode = (state->mode + 1) % PHASE_MODE_LAST;
            _minimal_phase_face_update_display(state);
            break;
            
        case EVENT_MODE_BUTTON_UP:
            // Exit face
            movement_move_to_next_face();
            return false;
            
        case EVENT_LIGHT_BUTTON_DOWN:
            // Illuminate display
            movement_illuminate_led();
            break;
            
        case EVENT_TIMEOUT:
            // Return to clock face after timeout
            movement_move_to_face(0);
            break;
            
        default:
            return movement_default_loop_handler(event, settings);
    }
    
    return true;
}

void minimal_phase_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
    // Nothing to clean up
}
