/*
 * MIT License
 *
 * Copyright (c) 2026 Diego Perez
 */

#include <stdlib.h>
#include <string.h>
#include "descent_face.h"

#ifdef PHASE_ENGINE_ENABLED
#include "../../lib/metrics/metrics.h"

// Forward declaration - metrics_get will be called from the metrics module
extern void metrics_get(const metrics_engine_t *engine, metrics_snapshot_t *out);

static void _descent_face_update_display(descent_face_state_t *state) {
    char buf[11] = {0};
    
    // Get current metrics (engine param unused in current implementation)
    metrics_snapshot_t metrics = {0};  // Zero-initialize
    metrics_get(NULL, &metrics);
    
    // Zone indicator in top-left
    watch_display_text(WATCH_POSITION_TOP_LEFT, "DE");
    
    // Display metric based on current view (with trend)
    uint8_t current_value = 0;
    int8_t trend = 0;
    
    switch (state->view_index) {
        case 0:  // Comfort (primary)
            current_value = metrics.comfort;
            trend = (int8_t)(current_value - state->prev_other[0]);
            snprintf(buf, sizeof(buf), "CF%d %+d", metrics.comfort, trend);
            state->prev_other[0] = current_value;
            break;
        case 1:  // Emotional
            current_value = metrics.em;
            trend = (int8_t)(current_value - state->prev_other[1]);
            snprintf(buf, sizeof(buf), "EM%d %+d", metrics.em, trend);
            state->prev_other[1] = current_value;
            break;
        case 2:  // Sleep Debt - use signed field
            current_value = (uint8_t)metrics.sd;
            trend = (int8_t)(metrics.sd - state->prev_sd);
            snprintf(buf, sizeof(buf), "SD%d %+d", metrics.sd, trend);
            state->prev_sd = metrics.sd;
            break;
        default:
            state->view_index = 0;
            current_value = metrics.comfort;
            trend = (int8_t)(current_value - state->prev_other[0]);
            snprintf(buf, sizeof(buf), "CF%d %+d", metrics.comfort, trend);
            state->prev_other[0] = current_value;
            break;
    }
    
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

void descent_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(descent_face_state_t));
        memset(*context_ptr, 0, sizeof(descent_face_state_t));
    }
}

void descent_face_activate(void *context) {
    descent_face_state_t *state = (descent_face_state_t *)context;
    state->view_index = 0;  // Always start with primary metric (Comfort)
}

bool descent_face_loop(movement_event_t event, void *context) {
    descent_face_state_t *state = (descent_face_state_t *)context;
    
    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            _descent_face_update_display(state);
            break;
            
        case EVENT_ALARM_BUTTON_UP:
            // Cycle through metric views
            state->view_index = (state->view_index + 1) % 3;
            _descent_face_update_display(state);
            break;
        
        case EVENT_MODE_LONG_PRESS:
            // Long-press MODE → exit playlist mode, return to clock
            movement_move_to_face(1);  // Clock face (index 1)
            break;
            
        case EVENT_LIGHT_BUTTON_UP:
            movement_illuminate_led();
            break;
            
        case EVENT_TIMEOUT:
            movement_move_to_face(1);  // Return to clock
            break;
            
        default:
            return movement_default_loop_handler(event);
    }
    
    return true;
}

void descent_face_resign(void *context) {
    (void) context;
    // Nothing to clean up
}

#endif // PHASE_ENGINE_ENABLED
