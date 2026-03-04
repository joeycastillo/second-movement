/*
 * MIT License
 *
 * Copyright (c) 2026 Diego Perez
 */

#include <stdlib.h>
#include <string.h>
#include "circadian_score_face.h"
#include "circadian_score.h"
#include "movement.h"

#ifdef PHASE_ENGINE_ENABLED
extern volatile movement_state_t movement_state;
#endif

static circadian_data_t global_circadian_data = {0};
static bool data_loaded = false;

static void _circadian_score_face_update_display(circadian_score_face_state_t *state) {
    char buf[11] = {0};
    
    // Load data from flash if not yet loaded
    if (!data_loaded) {
        circadian_data_load_from_flash(&global_circadian_data);
        data_loaded = true;
    }
    
    if (state->historical_night == 0) {
        // Show aggregate 7-day scores
        circadian_score_components_t components;
        circadian_score_calculate_components(&global_circadian_data, &components);
        
        switch (state->mode) {
            case CSFACE_MODE_CS:
                snprintf(buf, sizeof(buf), "CS  %2d", components.overall_score);
                break;
            case CSFACE_MODE_TI:
                snprintf(buf, sizeof(buf), "TI  %2d", components.timing_score);
                break;
            case CSFACE_MODE_DU:
                snprintf(buf, sizeof(buf), "DU  %2d", components.duration_score);
                break;
            case CSFACE_MODE_EF:
                snprintf(buf, sizeof(buf), "EF  %2d", components.efficiency_score);
                break;
            case CSFACE_MODE_AH:
                snprintf(buf, sizeof(buf), "AH  %2d", components.compliance_score);
                break;
            case CSFACE_MODE_LI:
                snprintf(buf, sizeof(buf), "LI  %2d", components.light_score);
                break;
#ifdef PHASE_ENGINE_ENABLED
            case CSFACE_MODE_RI:
                // Phase 4E: Display restlessness index (0-100, 0=perfect, 100=terrible)
                snprintf(buf, sizeof(buf), "RI  %2d", movement_state.sleep_telemetry.restlessness.today);
                break;
#endif
            default:
                break;
        }
    } else {
        // Show individual night (1-7 = most recent to oldest)
        uint8_t night_idx = (global_circadian_data.write_index + 7 - state->historical_night) % 7;
        const circadian_sleep_night_t *night = &global_circadian_data.nights[night_idx];
        
        if (!night->valid) {
            snprintf(buf, sizeof(buf), "-%d  --", state->historical_night);
        } else {
            switch (state->mode) {
                case CSFACE_MODE_CS: {
                    uint8_t score = circadian_score_calculate_sleep_score(night);
                    snprintf(buf, sizeof(buf), "-%d  %2d", state->historical_night, score);
                    break;
                }
                case CSFACE_MODE_DU: {
                    uint8_t hours = night->duration_min / 60;
                    uint8_t mins = night->duration_min % 60;
                    snprintf(buf, sizeof(buf), "-%dh%2d%02d", state->historical_night, hours, mins);
                    break;
                }
                case CSFACE_MODE_EF:
                    snprintf(buf, sizeof(buf), "-%d  %2d", state->historical_night, night->efficiency);
                    break;
                case CSFACE_MODE_AH:
                case CSFACE_MODE_TI:
                    // Not meaningful for single night
                    snprintf(buf, sizeof(buf), "-%d  --", state->historical_night);
                    break;
                case CSFACE_MODE_LI:
                    snprintf(buf, sizeof(buf), "-%d  %2d", state->historical_night, night->light_quality);
                    break;
#ifdef PHASE_ENGINE_ENABLED
                case CSFACE_MODE_RI: {
                    // Show historical restlessness index
                    uint8_t hist_idx = state->historical_night - 1;  // 1-7 maps to 0-6
                    if (hist_idx < 7) {
                        uint8_t ri = movement_state.sleep_telemetry.restlessness.history[hist_idx];
                        snprintf(buf, sizeof(buf), "-%d  %2d", state->historical_night, ri);
                    } else {
                        snprintf(buf, sizeof(buf), "-%d  --", state->historical_night);
                    }
                    break;
                }
#endif
                default:
                    break;
            }
        }
    }
    
    watch_display_text(WATCH_POSITION_FULL, buf);
}

void circadian_score_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(circadian_score_face_state_t));
        memset(*context_ptr, 0, sizeof(circadian_score_face_state_t));
    }
}

void circadian_score_face_activate(void *context) {
    circadian_score_face_state_t *state = (circadian_score_face_state_t *)context;
    state->mode = CSFACE_MODE_CS;  // Always start with overall score
}

bool circadian_score_face_loop(movement_event_t event, void *context) {
    circadian_score_face_state_t *state = (circadian_score_face_state_t *)context;
    
    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            _circadian_score_face_update_display(state);
            break;
        case EVENT_ALARM_BUTTON_UP:
            // Cycle through modes
            state->mode = (state->mode + 1) % CSFACE_MODE_COUNT;
            _circadian_score_face_update_display(state);
            break;
        case EVENT_LIGHT_BUTTON_UP:
            // Navigate historical nights (0 = aggregate, 1-7 = individual)
            state->historical_night = (state->historical_night + 1) % 8;
            _circadian_score_face_update_display(state);
            break;
        case EVENT_LIGHT_LONG_PRESS:
            // Quick return to aggregate view
            state->historical_night = 0;
            _circadian_score_face_update_display(state);
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);  // Return to watch face
            break;
        default:
            return movement_default_loop_handler(event);
    }
    
    return true;
}

void circadian_score_face_resign(void *context) {
    (void) context;
    // Nothing to clean up
}
