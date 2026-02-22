/*
 * MIT License
 *
 * Copyright (c) 2026 Diego Perez
 */

#include <stdlib.h>
#include <string.h>
#include "momentum_face.h"

#ifdef PHASE_ENGINE_ENABLED
#include "../../lib/metrics/metrics.h"

// Forward declaration - metrics_get will be called from the metrics module
extern void metrics_get(const metrics_engine_t *engine, metrics_snapshot_t *out);

static void _momentum_face_update_display(momentum_face_state_t *state) {
    char buf[11] = {0};
    
    // Get current metrics (engine param unused in current implementation)
    metrics_snapshot_t metrics = {0};  // Zero-initialize
    metrics_get(NULL, &metrics);
    
    // Zone indicator in top-left
    watch_display_text(WATCH_POSITION_TOP_LEFT, "MO");
    
    // Display metric based on current view
    switch (state->view_index) {
        case 0:  // Wake Momentum (primary)
            snprintf(buf, sizeof(buf), "WK %3d", metrics.wk);
            break;
        case 1:  // Sleep Debt
            snprintf(buf, sizeof(buf), "SD %+3d", metrics.sd);
            break;
        case 2:  // Emotional/Engagement
            snprintf(buf, sizeof(buf), "EM %3d", metrics.em);
            break;
        default:
            state->view_index = 0;
            snprintf(buf, sizeof(buf), "WK %3d", metrics.wk);
            break;
    }
    
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

void momentum_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(momentum_face_state_t));
        memset(*context_ptr, 0, sizeof(momentum_face_state_t));
    }
}

void momentum_face_activate(void *context) {
    momentum_face_state_t *state = (momentum_face_state_t *)context;
    state->view_index = 0;  // Always start with primary metric (WK)
}

bool momentum_face_loop(movement_event_t event, void *context) {
    momentum_face_state_t *state = (momentum_face_state_t *)context;
    
    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            _momentum_face_update_display(state);
            break;
            
        case EVENT_ALARM_BUTTON_UP:
            // Cycle through metric views
            state->view_index = (state->view_index + 1) % 3;
            _momentum_face_update_display(state);
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

void momentum_face_resign(void *context) {
    (void) context;
    // Nothing to clean up
}

#endif // PHASE_ENGINE_ENABLED
