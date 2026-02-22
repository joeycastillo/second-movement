/*
 * MIT License
 *
 * Copyright (c) 2026 Diego Perez
 */

#include <stdlib.h>
#include <string.h>
#include "active_face.h"

#ifdef PHASE_ENGINE_ENABLED
#include "../../lib/metrics/metrics.h"

// Forward declaration - metrics_get will be called from the metrics module
extern void metrics_get(const metrics_engine_t *engine, metrics_snapshot_t *out);

static void _active_face_update_display(active_face_state_t *state) {
    char buf[11] = {0};
    
    // Get current metrics (engine param unused in current implementation)
    metrics_snapshot_t metrics = {0};  // Zero-initialize
    metrics_get(NULL, &metrics);
    
    // Zone indicator in top-left
    watch_display_text(WATCH_POSITION_TOP_LEFT, "AC");
    
    // Display metric based on current view
    switch (state->view_index) {
        case 0:  // Energy (primary)
            snprintf(buf, sizeof(buf), "EN %3d", metrics.energy);
            break;
        case 1:  // Emotional
            snprintf(buf, sizeof(buf), "EM %3d", metrics.em);
            break;
        case 2:  // Sleep Debt
            snprintf(buf, sizeof(buf), "SD %+3d", metrics.sd);
            break;
        default:
            state->view_index = 0;
            snprintf(buf, sizeof(buf), "EN %3d", metrics.energy);
            break;
    }
    
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

void active_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(active_face_state_t));
        memset(*context_ptr, 0, sizeof(active_face_state_t));
    }
}

void active_face_activate(void *context) {
    active_face_state_t *state = (active_face_state_t *)context;
    state->view_index = 0;  // Always start with primary metric (Energy)
}

bool active_face_loop(movement_event_t event, void *context) {
    active_face_state_t *state = (active_face_state_t *)context;
    
    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            _active_face_update_display(state);
            break;
            
        case EVENT_ALARM_BUTTON_UP:
            // Cycle through metric views
            state->view_index = (state->view_index + 1) % 3;
            _active_face_update_display(state);
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

void active_face_resign(void *context) {
    (void) context;
    // Nothing to clean up
}

#endif // PHASE_ENGINE_ENABLED
