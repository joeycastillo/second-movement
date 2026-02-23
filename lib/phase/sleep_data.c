/*
 * MIT License
 *
 * Copyright (c) 2026 Diego Perez
 *
 * Phase 4E: Sleep Tracking Improvements + Telemetry Expansion
 * Implementation
 */

#ifdef PHASE_ENGINE_ENABLED

#include "sleep_data.h"
#include <string.h>

// Helper macros
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ABS(x) ((x) < 0 ? -(x) : (x))

// Default thresholds (from Cole-Kripke distribution)
#define DEFAULT_LIGHT_THRESHOLD 2
#define DEFAULT_RESTLESS_THRESHOLD 6
#define DEFAULT_WAKE_THRESHOLD 16

void sleep_data_init(sleep_telemetry_state_t *state) {
    memset(state, 0, sizeof(sleep_telemetry_state_t));
    
    // Set default thresholds
    state->thresholds.light_threshold = DEFAULT_LIGHT_THRESHOLD;
    state->thresholds.restless_threshold = DEFAULT_RESTLESS_THRESHOLD;
    state->thresholds.wake_threshold = DEFAULT_WAKE_THRESHOLD;
    
    state->initialized = true;
}

sleep_state_t sleep_data_get_state_at_epoch(const sleep_telemetry_state_t *state, uint16_t epoch_idx) {
    if (epoch_idx >= 960) return WAKE;  // Out of bounds, default to wake
    
    uint16_t byte_idx = epoch_idx / 4;
    uint8_t bit_offset = (epoch_idx % 4) * 2;
    
    return (sleep_state_t)((state->sleep_states.state_buffer[byte_idx] >> bit_offset) & 0x03);
}

void sleep_data_set_state_at_epoch(sleep_telemetry_state_t *state, uint16_t epoch_idx, sleep_state_t sleep_state) {
    if (epoch_idx >= 960) return;  // Out of bounds
    
    uint16_t byte_idx = epoch_idx / 4;
    uint8_t bit_offset = (epoch_idx % 4) * 2;
    
    // Clear the 2 bits
    state->sleep_states.state_buffer[byte_idx] &= ~(0x03 << bit_offset);
    
    // Set the new state
    state->sleep_states.state_buffer[byte_idx] |= ((uint8_t)sleep_state << bit_offset);
}

void sleep_data_record_epoch(sleep_telemetry_state_t *state, uint8_t movement_count) {
    if (!state || !state->initialized) return;
    
    // Determine sleep state based on movement count and thresholds
    sleep_state_t sleep_state;
    if (movement_count < state->thresholds.light_threshold) {
        sleep_state = DEEP_SLEEP;
    } else if (movement_count < state->thresholds.restless_threshold) {
        sleep_state = LIGHT_SLEEP;
    } else if (movement_count < state->thresholds.wake_threshold) {
        sleep_state = RESTLESS;
    } else {
        sleep_state = WAKE;
    }
    
    // Record state at current epoch
    uint16_t epoch_idx = state->sleep_states.total_epochs;
    if (epoch_idx < 960) {  // Only record if within 8-hour window
        sleep_data_set_state_at_epoch(state, epoch_idx, sleep_state);
        state->sleep_states.total_epochs++;
    }
}

void sleep_data_detect_wake_events(sleep_telemetry_state_t *state) {
    if (!state || !state->initialized) return;
    
    // Reset wake event data
    state->wake_events.wake_count = 0;
    state->wake_events.total_wake_min = 0;
    state->wake_events.longest_wake_min = 0;
    memset(state->wake_events.wake_times, 0, MAX_WAKE_EVENTS);
    
    uint16_t consecutive_wake = 0;
    uint16_t current_wake_start = 0;
    bool in_wake_event = false;
    
    // Skip first 20 minutes (40 epochs) - normal sleep latency
    uint16_t start_epoch = 40;
    
    for (uint16_t i = start_epoch; i < state->sleep_states.total_epochs; i++) {
        sleep_state_t s = sleep_data_get_state_at_epoch(state, i);
        
        if (s == WAKE) {
            if (!in_wake_event) {
                current_wake_start = i;
                consecutive_wake = 1;
                in_wake_event = true;
            } else {
                consecutive_wake++;
            }
        } else {
            // Not in wake state
            if (in_wake_event && consecutive_wake >= WAKE_MIN_EPOCHS) {
                // This was a valid wake event (sustained for >= 5 min)
                uint16_t wake_duration_epochs = consecutive_wake;
                uint16_t wake_duration_min = (wake_duration_epochs + 1) / 2;  // 30s epochs -> minutes
                
                // Saturating add to prevent overflow
                uint32_t new_total = (uint32_t)state->wake_events.total_wake_min + wake_duration_min;
                state->wake_events.total_wake_min = (new_total > UINT16_MAX) ? UINT16_MAX : (uint16_t)new_total;
                
                if (wake_duration_min > state->wake_events.longest_wake_min) {
                    state->wake_events.longest_wake_min = MIN(wake_duration_min, 255);
                }
                
                if (state->wake_events.wake_count < MAX_WAKE_EVENTS) {
                    // Record wake time as offset from sleep onset in 15-min blocks
                    uint16_t offset_min = current_wake_start / 2;  // epochs -> minutes
                    state->wake_events.wake_times[state->wake_events.wake_count] = MIN(offset_min / 15, 255);
                    state->wake_events.wake_count++;
                }
                // If already at max, don't increment (prevents overflow and wraparound)
            }
            
            consecutive_wake = 0;
            in_wake_event = false;
        }
    }
    
    // Check if we ended in a wake event
    if (in_wake_event && consecutive_wake >= WAKE_MIN_EPOCHS) {
        uint16_t wake_duration_epochs = consecutive_wake;
        uint16_t wake_duration_min = (wake_duration_epochs + 1) / 2;
        
        // Saturating add to prevent overflow
        uint32_t new_total = (uint32_t)state->wake_events.total_wake_min + wake_duration_min;
        state->wake_events.total_wake_min = (new_total > UINT16_MAX) ? UINT16_MAX : (uint16_t)new_total;
        
        if (wake_duration_min > state->wake_events.longest_wake_min) {
            state->wake_events.longest_wake_min = MIN(wake_duration_min, 255);
        }
        
        if (state->wake_events.wake_count < MAX_WAKE_EVENTS) {
            uint16_t offset_min = current_wake_start / 2;
            state->wake_events.wake_times[state->wake_events.wake_count] = MIN(offset_min / 15, 255);
            state->wake_events.wake_count++;
        }
        // If already at max, don't increment (prevents overflow and wraparound)
    }
}

uint8_t sleep_data_calc_restlessness(const sleep_telemetry_state_t *state) {
    if (!state || !state->initialized || state->sleep_states.total_epochs == 0) {
        return 0;
    }
    
    // Calculate percentages first
    uint16_t deep_count = 0;
    uint16_t light_count = 0;
    uint16_t restless_count = 0;
    uint16_t wake_count = 0;
    
    for (uint16_t i = 0; i < state->sleep_states.total_epochs; i++) {
        sleep_state_t s = sleep_data_get_state_at_epoch(state, i);
        switch (s) {
            case DEEP_SLEEP:
                deep_count++;
                break;
            case LIGHT_SLEEP:
                light_count++;
                break;
            case RESTLESS:
                restless_count++;
                break;
            case WAKE:
                wake_count++;
                break;
        }
    }
    
    // Calculate percentages (avoid division by zero and integer overflow)
    uint8_t restless_pct = (uint8_t)(((uint32_t)restless_count * 100) / state->sleep_states.total_epochs);
    uint8_t wake_pct = (uint8_t)(((uint32_t)wake_count * 100) / state->sleep_states.total_epochs);
    
    // Movement component (0-50): % of epochs in RESTLESS or WAKE state
    uint16_t move_score = (restless_pct + wake_pct) / 2;
    if (move_score > 50) move_score = 50;
    
    // Wake component (0-30): scaled by wake count
    uint16_t wake_score = state->wake_events.wake_count * 5;
    if (wake_score > 30) wake_score = 30;
    
    // Duration component (0-20): longest wake bout contribution
    uint16_t dur_score = state->wake_events.longest_wake_min;
    if (dur_score > 20) dur_score = 20;
    
    return (uint8_t)(move_score + wake_score + dur_score);
}

void sleep_data_accumulate_telemetry(sleep_telemetry_state_t *state,
                                    uint8_t hour,
                                    uint8_t current_zone,
                                    uint8_t prev_zone,
                                    dominant_metric_t dominant_metric,
                                    bool anomaly_fired,
                                    uint8_t light_minutes,
                                    uint8_t motion_interrupts,
                                    uint16_t battery_mv,
                                    uint8_t sd_now,
                                    uint8_t sd_prev,
                                    uint8_t em_now,
                                    uint8_t em_prev,
                                    uint8_t wk_now,
                                    uint8_t wk_prev,
                                    uint8_t comfort_now,
                                    uint8_t comfort_prev) {
    if (!state || !state->initialized || hour >= 24) return;
    
    hourly_telemetry_t *sample = &state->telemetry.hours[hour];
    
    // Build flags byte
    sample->flags = 0;
    
    // Zone transition bit
    if (current_zone != prev_zone) {
        sample->flags |= 0x01;
    }
    
    // Dominant metric (bits 1-2)
    sample->flags |= ((uint8_t)dominant_metric & 0x03) << 1;
    
    // Anomaly flag (bit 3)
    if (anomaly_fired) {
        sample->flags |= 0x08;
    }
    
    // Direct readings
    sample->light_exposure_min = MIN(light_minutes, 60);
    sample->movement_count = MIN(motion_interrupts, 255);
    
    // Battery voltage: (V - 2.0) / 2.2 × 255
    // battery_mv is in millivolts, so (battery_mv - 2000) * 255 / 2200
    if (battery_mv < 2000) battery_mv = 2000;
    if (battery_mv > 4200) battery_mv = 4200;
    sample->battery_voltage = (uint8_t)(((battery_mv - 2000) * 255UL) / 2200);
    
    // Confidence scores: 100 - abs(delta), clamped
    // High stability (small delta) = high confidence
    int16_t sd_delta = ABS((int16_t)sd_now - (int16_t)sd_prev);
    int16_t em_delta = ABS((int16_t)em_now - (int16_t)em_prev);
    int16_t wk_delta = ABS((int16_t)wk_now - (int16_t)wk_prev);
    int16_t comfort_delta = ABS((int16_t)comfort_now - (int16_t)comfort_prev);
    
    sample->confidence_sd = 100 - MIN(sd_delta, 100);
    sample->confidence_em = 100 - MIN(em_delta, 100);
    sample->confidence_wk = 100 - MIN(wk_delta, 100);
    sample->confidence_comfort = 100 - MIN(comfort_delta, 100);
    
    state->telemetry.current_hour = hour;
}

void sleep_data_reset_daily_telemetry(sleep_telemetry_state_t *state) {
    if (!state || !state->initialized) return;
    
    memset(&state->telemetry, 0, sizeof(daily_telemetry_t));
}

// ============================================================================
// Phase 4F: WK Baseline Implementation
// ============================================================================

void wk_baseline_update_daily(wk_baseline_t *wk, uint16_t daily_movement) {
    if (!wk) return;
    
    // Store today's total in circular buffer
    wk->daily_totals[wk->day_index] = daily_movement;
    
    // Advance circular buffer index
    wk->day_index = (wk->day_index + 1) % 7;
    
    // Recompute baseline: average movements per hour over valid days
    uint32_t sum = 0;
    uint8_t valid_days = 0;
    
    for (uint8_t i = 0; i < 7; i++) {
        if (wk->daily_totals[i] > 0) {
            sum += wk->daily_totals[i];
            valid_days++;
        }
    }
    
    if (valid_days > 0) {
        // Baseline = average hourly movement (assume 16 active hours per day)
        // Integer math: sum / (valid_days * 16)
        uint32_t total_hours = (uint32_t)valid_days * 16;
        wk->baseline_per_hour = (uint8_t)(sum / total_hours);
    } else {
        // No history: use sensible default (30 movements/hour)
        wk->baseline_per_hour = 30;
    }
}

uint8_t wk_baseline_get_per_hour(const wk_baseline_t *wk) {
    return wk ? wk->baseline_per_hour : 30;  // Default if NULL
}

uint8_t wk_baseline_get_active_threshold(const wk_baseline_t *wk) {
    uint8_t baseline = wk_baseline_get_per_hour(wk);
    // Active = 1.5× baseline (integer: baseline + baseline/2)
    return baseline + (baseline >> 1);
}

uint8_t wk_baseline_get_very_active_threshold(const wk_baseline_t *wk) {
    uint8_t baseline = wk_baseline_get_per_hour(wk);
    // Very active = 2.5× baseline (integer: baseline*2 + baseline/2)
    return (baseline << 1) + (baseline >> 1);
}

#endif // PHASE_ENGINE_ENABLED
