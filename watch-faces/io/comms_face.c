/*
 * MIT License
 *
 * Copyright (c) 2026 Diego Perez
 *
 * Unified Communications Face - Phase 2a Implementation (TX + RX Foundation)
 * TX: Uses FESK library by Eirik S. Morland (PR #139 to second-movement)
 * RX: Optical data reception via ambient light sensor (Manchester encoding)
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "comms_face.h"
#include "circadian_score.h"
#include "movement.h"

// Access to global movement state
extern volatile movement_state_t movement_state;

#ifdef HAS_IR_SENSOR
#include "adc.h"
#endif

#ifdef PHASE_ENGINE_ENABLED
#include "../../lib/phase/phase_engine.h"
#include "../../lib/metrics/metrics.h"
#include "../../lib/phase/sleep_data.h"
#endif

#ifdef PHASE_ENGINE_ENABLED
// Phase Engine hourly sample (6 bytes)
typedef struct {
    uint8_t hour : 5;          // 0-23
    uint8_t zone : 2;          // 0-3 (Emergence/Momentum/Active/Descent)
    uint8_t reserved : 1;      // Future use
    
    uint8_t phase_score;       // 0-100
    uint8_t sd;                // Sleep Debt (offset by 60: 0-180 → -60 to +120)
    uint8_t em;                // Emotional 0-100
    uint8_t wk;                // Wake Momentum 0-100
    uint8_t comfort;           // Comfort 0-100
} phase_hourly_sample_t;

// Phase Engine export data (prepended to circadian export)
typedef struct {
    uint8_t version;           // Protocol version (0x01)
    uint8_t sample_count;      // Number of samples (0-24)
    phase_hourly_sample_t samples[24];  // Hourly samples
} phase_export_t;  // Total: 2 + (6 × 24) = 146 bytes
#endif

// Hex encoding lookup
static const char hex_chars[] = "0123456789ABCDEF";

/* Static TX buffers: allocated once in BSS, not per-session on heap.
 * Phase 4E: 512 bytes binary (146 phase + 112 circadian + 254 sleep/telemetry)
 * Phase Engine (pre-4E): 256 bytes binary (146 phase + 112 circadian)
 * Circadian only: 112 bytes binary
 * Hex buffer: 2x binary + 1 for NUL
 * Safe on embedded: zeroed at startup, reused each transmission. */
#ifdef PHASE_ENGINE_ENABLED
static uint8_t _export_buffer[512];  // Phase 4E: increased for sleep tracking + telemetry
static char    _hex_buffer[1025];    // 512 * 2 + 1 = 1025
#else
static uint8_t _export_buffer[112];
static char    _hex_buffer[225];
#endif

static void _hex_encode(const uint8_t *data, size_t len, char *out) {
    for (size_t i = 0; i < len; i++) {
        out[i * 2] = hex_chars[(data[i] >> 4) & 0xF];
        out[i * 2 + 1] = hex_chars[data[i] & 0xF];
    }
    out[len * 2] = '\0';
}

#ifdef PHASE_ENGINE_ENABLED

// Phase 4E: Export sleep tracking + telemetry data
static uint16_t _export_sleep_telemetry(uint8_t *buffer, uint16_t max_size, sleep_telemetry_state_t *sleep_state) {
    if (!sleep_state || max_size < 254) {
        return 0;  // Need at least 254 bytes
    }
    
    uint16_t offset = 0;
    
    // Header (2 bytes)
    buffer[offset++] = 0x4E;  // Magic: 'N' for Phase 4E
    buffer[offset++] = 0x01;  // Version 1
    
    // Sleep state percentages (4 bytes)
    buffer[offset++] = sleep_state->sleep_states.deep_pct;
    buffer[offset++] = sleep_state->sleep_states.light_pct;
    buffer[offset++] = sleep_state->sleep_states.restless_pct;
    buffer[offset++] = sleep_state->sleep_states.wake_pct;
    
    // Wake events (20 bytes)
    buffer[offset++] = sleep_state->wake_events.wake_count;
    buffer[offset++] = (sleep_state->wake_events.total_wake_min >> 8) & 0xFF;
    buffer[offset++] = sleep_state->wake_events.total_wake_min & 0xFF;
    buffer[offset++] = sleep_state->wake_events.longest_wake_min;
    memcpy(buffer + offset, sleep_state->wake_events.wake_times, MAX_WAKE_EVENTS);
    offset += MAX_WAKE_EVENTS;
    
    // Restlessness index (8 bytes)
    buffer[offset++] = sleep_state->restlessness.today;
    memcpy(buffer + offset, sleep_state->restlessness.history, 7);
    offset += 7;
    
    // Hourly telemetry (24 × 8 = 192 bytes)
    for (uint8_t i = 0; i < 24; i++) {
        hourly_telemetry_t *sample = &sleep_state->telemetry.hours[i];
        buffer[offset++] = sample->flags;
        buffer[offset++] = sample->light_exposure_min;
        buffer[offset++] = sample->movement_count;
        buffer[offset++] = sample->battery_voltage;
        buffer[offset++] = sample->confidence_sd;
        buffer[offset++] = sample->confidence_em;
        buffer[offset++] = sample->confidence_wk;
        buffer[offset++] = sample->confidence_comfort;
    }
    
    // Sleep state thresholds (3 bytes)
    buffer[offset++] = sleep_state->thresholds.light_threshold;
    buffer[offset++] = sleep_state->thresholds.restless_threshold;
    buffer[offset++] = sleep_state->thresholds.wake_threshold;
    
    // Padding to 254 bytes (offset should be 230, need 24 more)
    while (offset < 254) {
        buffer[offset++] = 0x00;
    }
    
    return offset;
}

static uint16_t _export_phase_data(uint8_t *buffer, uint16_t max_size) {
    if (max_size < sizeof(phase_export_t)) {
        return 0;  // Buffer too small
    }
    
    phase_export_t *export = (phase_export_t *)buffer;
    export->version = 0x01;
    export->sample_count = 0;
    
    // Get phase state (from movement.c global or BKUP registers)
    // TODO: Access phase_state_t and metrics history
    // For now, export current snapshot repeated (dogfooding MVP)
    
    // Get current hour (unused in MVP but reserved for future hourly buffering)
    // rtc_date_time_t now = watch_rtc_get_date_time();
    // uint8_t current_hour = now.unit.hour;
    
    // Get current metrics
    metrics_snapshot_t metrics;
    metrics_get(NULL, &metrics);
    
    // Get current phase/zone (TODO: access playlist state)
    uint8_t current_zone = 0;  // Default to Emergence
    uint8_t phase_score = 50;  // Default mid-range
    
    // Fill hourly samples (for MVP, just current state)
    // TODO: In production, read from 24-hour circular buffer
    for (uint8_t i = 0; i < 24; i++) {
        export->samples[i].hour = i;
        export->samples[i].zone = current_zone;
        export->samples[i].reserved = 0;
        export->samples[i].phase_score = phase_score;
        export->samples[i].sd = (uint8_t)(metrics.sd + 60);  // Offset -60..+120 → 0..180
        export->samples[i].em = metrics.em;
        export->samples[i].wk = metrics.wk;
        export->samples[i].comfort = metrics.comfort;
        export->sample_count++;
    }
    
    return sizeof(phase_export_t);
}
#endif

static void _on_transmission_end(void *user_data) {
    comms_face_state_t *state = (comms_face_state_t *)user_data;
    state->mode = COMMS_MODE_TX_DONE;
    state->transmission_active = false;
    watch_clear_indicator(WATCH_INDICATOR_BELL);
}

static void _on_transmission_start(void *user_data) {
    comms_face_state_t *state = (comms_face_state_t *)user_data;
    state->tx_elapsed_seconds = 0;
    watch_set_indicator(WATCH_INDICATOR_BELL);
}

static void _on_error(fesk_result_t error, void *user_data) {
    (void)error;
    comms_face_state_t *state = (comms_face_state_t *)user_data;
    state->mode = COMMS_MODE_IDLE;
    state->transmission_active = false;
    state->tx_elapsed_seconds = 0;
    watch_clear_indicator(WATCH_INDICATOR_BELL);
}

static void _start_transmission(comms_face_state_t *state) {
    uint16_t offset = 0;
    
#ifdef PHASE_ENGINE_ENABLED
    // Export Phase Engine data first
    offset = _export_phase_data(_export_buffer, sizeof(_export_buffer));
    if (offset == 0) {
        // Export failed, fall back to circadian only
        offset = 0;
    }
    
    // Phase 4E: Export sleep tracking + telemetry data
    uint16_t sleep_size = _export_sleep_telemetry(_export_buffer + offset, 
                                                   sizeof(_export_buffer) - offset,
                                                   (sleep_telemetry_state_t*)&movement_state.sleep_telemetry);
    offset += sleep_size;
#endif
    
    // Load circadian data for export
    circadian_data_t circadian_data;
    circadian_data_load_from_flash(&circadian_data);
    
    uint16_t circadian_size = circadian_data_export_binary(&circadian_data, 
                                                            _export_buffer + offset, 
                                                            sizeof(_export_buffer) - offset);
    
    state->export_size = offset + circadian_size;
    
    if (state->export_size == 0) {
        // No data or buffer too small
        watch_display_text(WATCH_POSITION_BOTTOM, "NO DAT");
        return;
    }
    
    // Hex-encode binary data
    _hex_encode(_export_buffer, state->export_size, _hex_buffer);
    
    // Configure FESK session
    fesk_session_config_t config = fesk_session_config_defaults();
    config.static_message = _hex_buffer;
    config.mode = FESK_MODE_4FSK;
    config.enable_countdown = false;  // No countdown, start immediately
    config.show_bell_indicator = false;  // We manage it manually
    config.on_transmission_start = _on_transmission_start;
    config.on_transmission_end = _on_transmission_end;
    config.on_error = _on_error;
    config.user_data = state;
    
    fesk_session_init(&state->fesk_session, &config);
    
    if (fesk_session_start(&state->fesk_session)) {
        state->mode = COMMS_MODE_TX_ACTIVE;
        state->transmission_active = true;
    } else {
        watch_display_text(WATCH_POSITION_BOTTOM, "BUSY  ");
        fesk_session_dispose(&state->fesk_session);
    }
}

static void _stop_transmission(comms_face_state_t *state) {
    fesk_session_cancel(&state->fesk_session);
    fesk_session_dispose(&state->fesk_session);
    state->mode = COMMS_MODE_IDLE;
    state->transmission_active = false;
    state->tx_elapsed_seconds = 0;
    watch_clear_indicator(WATCH_INDICATOR_BELL);
}

// RX functions (implemented in comms_rx.c)
#ifdef HAS_IR_SENSOR
extern void optical_rx_start(comms_face_state_t *state);
extern void optical_rx_stop(comms_face_state_t *state);
extern void optical_rx_poll(comms_face_state_t *state);
#endif

static void _update_display(comms_face_state_t *state) {
    char buf[16];
    
    switch (state->mode) {
        case COMMS_MODE_IDLE: {
            // Top-left: TX or RX
            if (state->active_mode == COMMS_TX) {
                watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "TX", "TX");
            } else {
                watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "RX", "RX");
            }
            // Bottom: show selected data sub-mode
            switch (state->data_type) {
                case COMMS_DATA_ALL:      watch_display_text(WATCH_POSITION_BOTTOM, " ALL  "); break;
                case COMMS_DATA_SLEEP:    watch_display_text(WATCH_POSITION_BOTTOM, " SLP  "); break;
                case COMMS_DATA_LIGHT:    watch_display_text(WATCH_POSITION_BOTTOM, " LIT  "); break;
                case COMMS_DATA_ACTIVITY: watch_display_text(WATCH_POSITION_BOTTOM, " ACT  "); break;
                case COMMS_DATA_CONTROL:  watch_display_text(WATCH_POSITION_BOTTOM, " CTL  "); break;
                default:                  watch_display_text(WATCH_POSITION_BOTTOM, " ALL  "); break;
            }
            break;
        }
        case COMMS_MODE_TX_ACTIVE: {
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "TX", "Trans");

            // Calculate remaining time
            // FESK 4-FSK rate: 26 bps (bits per second) = 3.25 bytes/second
            // Hex encoding: 287 bytes -> 574 hex chars
            // Total bits: 574 * 8 = 4592 bits
            // Total time: 4592 / 26 ~= 177 seconds (~3 minutes)
            uint16_t hex_bytes = state->export_size * 2;
            uint16_t total_bits = hex_bytes * 6;
            uint16_t total_seconds = (total_bits + 25) / 26;
            uint16_t elapsed = state->tx_elapsed_seconds;
            int16_t remaining = (int16_t)total_seconds - (int16_t)elapsed;

            if (remaining < 0) remaining = 0;

            if (remaining < 100) {
                snprintf(buf, sizeof(buf), " %2ds  ", remaining);
            } else {
                snprintf(buf, sizeof(buf), "%3ds  ", remaining);
            }
            watch_display_text(WATCH_POSITION_BOTTOM, buf);
            break;
        }
        case COMMS_MODE_TX_DONE:
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "TX", "Trans");
            watch_display_text(WATCH_POSITION_BOTTOM, " END  ");
            break;
        case COMMS_MODE_RX_ACTIVE:
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "RX", "Recv");
            if (state->rx_state.synced) {
                // Receiving data - show elapsed time
                snprintf(buf, sizeof(buf), " %2ds  ", state->rx_seconds_elapsed);
                watch_display_text(WATCH_POSITION_BOTTOM, buf);
            } else {
                // Looking for sync
                watch_display_text(WATCH_POSITION_BOTTOM, " SYNC ");
            }
            break;
        case COMMS_MODE_RX_DONE:
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "RX", "Recv");
            watch_display_text(WATCH_POSITION_BOTTOM, "  OK  ");
            break;
        case COMMS_MODE_RX_ERROR:
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "RX", "Recv");
            switch (state->rx_error_code) {
                case RX_ERROR_SYNC_TIMEOUT:    watch_display_text(WATCH_POSITION_BOTTOM, "ER SYN"); break;
                case RX_ERROR_CRC_FAIL:        watch_display_text(WATCH_POSITION_BOTTOM, "ER CRC"); break;
                case RX_ERROR_PACKET_TIMEOUT:  watch_display_text(WATCH_POSITION_BOTTOM, "ER TMO"); break;
                case RX_ERROR_BIT_TIMEOUT:     watch_display_text(WATCH_POSITION_BOTTOM, "ER BIT"); break;
                case RX_ERROR_BUFFER_OVERFLOW: watch_display_text(WATCH_POSITION_BOTTOM, "ER BUF"); break;
                case RX_ERROR_INVALID_LENGTH:  watch_display_text(WATCH_POSITION_BOTTOM, "ER LEN"); break;
                case RX_ERROR_INVALID_TYPE:    watch_display_text(WATCH_POSITION_BOTTOM, "ER TYP"); break;
                default:                       watch_display_text(WATCH_POSITION_BOTTOM, " ERR  "); break;
            }
            break;
    }
}

void comms_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(comms_face_state_t));
        if (*context_ptr == NULL) return;
        memset(*context_ptr, 0, sizeof(comms_face_state_t));
    }
}

void comms_face_activate(void *context) {
    comms_face_state_t *state = (comms_face_state_t *)context;
    state->mode = COMMS_MODE_IDLE;
    state->transmission_active = false;
    state->tx_elapsed_seconds = 0;
    state->active_mode = COMMS_RX;  // Default to RX mode
    state->data_type = COMMS_DATA_ALL; // Default: all data types
    state->light_sensor_active = false;
    _update_display(state);
}

bool comms_face_loop(movement_event_t event, void *context) {
    comms_face_state_t *state = (comms_face_state_t *)context;
    
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _update_display(state);
            break;
        case EVENT_TICK:
            if (state->mode == COMMS_MODE_TX_ACTIVE) {
                state->tx_elapsed_seconds++;
            }
            // Poll RX if active
            #ifdef HAS_IR_SENSOR
            if (state->mode == COMMS_MODE_RX_ACTIVE) {
                optical_rx_poll(state);

                // Track elapsed time (increment every 64 ticks = 1 second @ 64 Hz)
                if (state->rx_state.synced) {
                    state->rx_tick_counter++;
                    if (state->rx_tick_counter >= 64) {
                        // Prevent overflow (uint16_t supports up to 18 hours)
                        if (state->rx_seconds_elapsed < UINT16_MAX) {
                            state->rx_seconds_elapsed++;
                        }
                        state->rx_tick_counter = 0;
                    }
                }
            }
            #endif
            _update_display(state);
            break;
        case EVENT_ALARM_BUTTON_UP:
            if (state->mode == COMMS_MODE_IDLE) {
                // Start TX or RX depending on mode
                if (state->active_mode == COMMS_TX) {
                    _start_transmission(state);
                } else {
                    #ifdef HAS_IR_SENSOR
                    optical_rx_start(state);
                    #endif
                }
                _update_display(state);
            } else if (state->mode == COMMS_MODE_TX_ACTIVE) {
                _stop_transmission(state);
                _update_display(state);
            } else if (state->mode == COMMS_MODE_RX_ACTIVE) {
                #ifdef HAS_IR_SENSOR
                optical_rx_stop(state);
                #endif
                _update_display(state);
            } else if (state->mode == COMMS_MODE_TX_DONE || 
                       state->mode == COMMS_MODE_RX_DONE || 
                       state->mode == COMMS_MODE_RX_ERROR) {
                state->mode = COMMS_MODE_IDLE;
                state->tx_elapsed_seconds = 0;
                _update_display(state);
            }
            break;
        case EVENT_LIGHT_BUTTON_UP:
            // Light short press: cycle RX ↔ TX (only when idle)
            // In non-idle TX mode, illuminate LED; in RX mode never use LED (interferes with sensor)
            if (state->mode == COMMS_MODE_IDLE) {
                state->active_mode = (state->active_mode == COMMS_TX) ? COMMS_RX : COMMS_TX;
                _update_display(state);
            } else if (state->active_mode == COMMS_TX) {
                movement_illuminate_led();
            }
            break;
        case EVENT_LIGHT_LONG_PRESS:
            // Light long press: cycle data sub-mode (all → sleep → light → activity → control → all)
            // Only when idle; resets on each TX/RX toggle
            if (state->mode == COMMS_MODE_IDLE) {
                if (state->data_type == COMMS_DATA_ALL) {
                    state->data_type = COMMS_DATA_SLEEP;
                } else if ((uint8_t)state->data_type < COMMS_DATA_TYPE_COUNT - 1) {
                    state->data_type = (comms_data_type_t)((uint8_t)state->data_type + 1);
                } else {
                    state->data_type = COMMS_DATA_ALL;
                }
                _update_display(state);
            }
            break;
        case EVENT_TIMEOUT:
            if (state->mode != COMMS_MODE_TX_ACTIVE && state->mode != COMMS_MODE_RX_ACTIVE) {
                movement_move_to_face(0);
            }
            break;
        default:
            return movement_default_loop_handler(event);
    }
    
    return true;
}

void comms_face_resign(void *context) {
    comms_face_state_t *state = (comms_face_state_t *)context;
    if (state->transmission_active) {
        _stop_transmission(state);
    }
    #ifdef HAS_IR_SENSOR
    if (state->light_sensor_active) {
        optical_rx_stop(state);
    }
    #endif
}
