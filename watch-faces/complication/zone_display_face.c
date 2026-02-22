/*
 * MIT License
 *
 * Copyright (c) 2026 Diego Perez
 *
 * Zone Display Face - Implementation
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "zone_display_face.h"

#ifdef PHASE_ENGINE_ENABLED
#include "../../lib/metrics/metrics.h"
#include "../../lib/phase/playlist.h"

// Access movement state to call playlist functions
extern volatile movement_state_t movement_state;

// Metric abbreviations (5 chars max to fit LCD)
static const char* metric_abbrevs[5] = {
    "SD",      // Sleep Debt
    "EM",      // Emotional
    "WK",      // Work
    "Enrgy",   // Energy
    "Comft"    // Comfort
};

static void _zone_display_face_update_display(zone_display_face_state_t *state) {
    (void)state;  // Unused - no persistent state
    char buf[11] = {0};
    
    // Query playlist to determine which metric to display
    uint8_t metric_idx = playlist_get_current_face(&movement_state.playlist);
    
    // Get current metrics snapshot
    metrics_snapshot_t metrics = {0};
    metrics_get(&movement_state.metrics, &metrics);
    
    // Get metric value
    uint8_t value = 0;
    
    switch (metric_idx) {
        case 0:  // Sleep Debt
            value = metrics.sd;
            break;
        case 1:  // Emotional
            value = metrics.em;
            break;
        case 2:  // Work
            value = metrics.wk;
            break;
        case 3:  // Energy
            value = metrics.energy;
            break;
        case 4:  // Comfort
            value = metrics.comfort;
            break;
        default:
            metric_idx = 0;
            value = metrics.sd;
            break;
    }
    
    // Display format: metric abbreviation + value (simplified for now)
    // Future: Add trend calculation and display
    snprintf(buf, sizeof(buf), "%-5s %3d", metric_abbrevs[metric_idx], (int)value);
    watch_display_text(WATCH_POSITION_TOP, buf);
    
    // Clear bottom line for now (trend display can be added in future)
    watch_display_text(WATCH_POSITION_BOTTOM, "        ");
}

void zone_display_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(zone_display_face_state_t));
        memset(*context_ptr, 0, sizeof(zone_display_face_state_t));
    }
}

void zone_display_face_activate(void *context) {
    zone_display_face_state_t *state = (zone_display_face_state_t *)context;
    (void)state;  // No initialization needed
}

bool zone_display_face_loop(movement_event_t event, void *context) {
    zone_display_face_state_t *state = (zone_display_face_state_t *)context;
    
    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            _zone_display_face_update_display(state);
            break;
            
        case EVENT_ALARM_BUTTON_UP:
            // ALARM: Cycle through zone playlist
            playlist_advance(&movement_state.playlist);
            _zone_display_face_update_display(state);
            break;
        
        case EVENT_MODE_LONG_PRESS:
            // Long-press MODE → exit playlist mode, return to clock
            movement_state.playlist_mode_active = false;
            movement_move_to_face(1);  // Clock face (index 1)
            break;
            
        case EVENT_LIGHT_BUTTON_UP:
            movement_illuminate_led();
            break;
            
        case EVENT_TIMEOUT:
            // Timeout → return to clock (but stay in playlist mode)
            movement_move_to_face(1);
            break;
            
        default:
            return movement_default_loop_handler(event);
    }
    
    return true;
}

void zone_display_face_resign(void *context) {
    (void) context;
    // Nothing to clean up
}

#endif // PHASE_ENGINE_ENABLED
