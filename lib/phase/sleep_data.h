/*
 * MIT License
 *
 * Copyright (c) 2026 Diego Perez
 *
 * Phase 4E: Sleep Tracking Improvements + Telemetry Expansion
 * 
 * Adds rich sleep quality metrics beyond binary sleep/wake:
 * - 4-level sleep states (deep/light/restless/wake)
 * - Wake event detection and characterization
 * - Restlessness index (0-100 sleep quality score)
 * - Per-hour operational telemetry for Phase Engine tuning
 */

#ifndef SLEEP_DATA_H_
#define SLEEP_DATA_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef PHASE_ENGINE_ENABLED

// ============================================================================
// 1. Movement Frequency Scoring (4-Level Sleep States)
// ============================================================================

typedef enum {
    DEEP_SLEEP = 0,    // 0-1 interrupts/epoch
    LIGHT_SLEEP = 1,   // 2-5 interrupts/epoch
    RESTLESS = 2,      // 6-15 interrupts/epoch
    WAKE = 3           // 16+ interrupts/epoch
} sleep_state_t;

// Per 30-second epoch, 2 bits for sleep state
// 8 hours = 960 epochs = 240 bytes (packed 4 per byte)
typedef struct {
    uint8_t state_buffer[240];   // 4 epochs per byte, 2 bits each
    uint8_t deep_pct;            // % time in deep sleep (0-100)
    uint8_t light_pct;           // % time in light sleep (0-100)
    uint8_t restless_pct;        // % time restless (0-100)
    uint8_t wake_pct;            // % time in wake state (0-100)
    uint16_t total_epochs;       // Total epochs recorded (for percentages)
} sleep_state_data_t;  // 246 bytes

// Thresholds for state classification (tunable via settings)
typedef struct {
    uint8_t light_threshold;     // Default: 2 (interrupts/epoch)
    uint8_t restless_threshold;  // Default: 6
    uint8_t wake_threshold;      // Default: 16
} sleep_state_thresholds_t;

// ============================================================================
// 2. Wake Event Detection
// ============================================================================

#define WAKE_MIN_EPOCHS 10       // 5 minutes (10 × 30s)
#define MAX_WAKE_EVENTS 16       // Maximum wake events to track

typedef struct {
    uint8_t wake_count;          // Number of wake events (0-255)
    uint16_t total_wake_min;     // Total minutes awake during sleep window
    uint8_t longest_wake_min;    // Longest single wake bout (minutes, capped 255)
    uint8_t wake_times[MAX_WAKE_EVENTS];  // Offset from sleep onset in 15-min blocks
} wake_event_data_t;  // 20 bytes

// ============================================================================
// 3. Restlessness Index (0-100 sleep quality score)
// ============================================================================

typedef struct {
    uint8_t today;               // Today's restlessness index (0-100)
    uint8_t history[7];          // 7-day history for trends
    uint8_t history_idx;         // Circular buffer write position
} restlessness_data_t;  // 9 bytes

// ============================================================================
// 4. Per-Hour Telemetry
// ============================================================================

// Dominant metric IDs
typedef enum {
    DOMINANT_SD = 0,
    DOMINANT_EM = 1,
    DOMINANT_WK = 2,
    DOMINANT_COMFORT = 3
} dominant_metric_t;

// Bit-packed flags byte
// Bits [7:4] reserved
// Bit  [3]   anomaly_flag
// Bits [2:1] dominant_metric_id (0=SD, 1=EM, 2=WK, 3=Comfort)
// Bit  [0]   zone_transition

typedef struct {
    uint8_t flags;               // zone_transition:1, dominant_metric:2, anomaly:1, reserved:4
    uint8_t light_exposure_min;  // 0-60 minutes
    uint8_t movement_count;      // LIS2DW12 interrupt count (capped 255)
    uint8_t battery_voltage;     // scaled 2.0-4.2V → 0-255
    uint8_t confidence_sd;       // 0-100
    uint8_t confidence_em;       // 0-100
    uint8_t confidence_wk;       // 0-100
    uint8_t confidence_comfort;  // 0-100
} hourly_telemetry_t;  // 8 bytes

typedef struct {
    hourly_telemetry_t hours[24];  // 192 bytes
    uint8_t current_hour;          // Last updated hour (0-23)
} daily_telemetry_t;  // 193 bytes

// ============================================================================
// Combined Sleep + Telemetry State
// ============================================================================

typedef struct {
    sleep_state_data_t sleep_states;
    wake_event_data_t wake_events;
    restlessness_data_t restlessness;
    daily_telemetry_t telemetry;
    sleep_state_thresholds_t thresholds;
    bool initialized;
} sleep_telemetry_state_t;  // ~471 bytes total

// ============================================================================
// Function Declarations
// ============================================================================

/**
 * Initialize sleep tracking state.
 * Call once at startup.
 */
void sleep_data_init(sleep_telemetry_state_t *state);

/**
 * Record a 30-second epoch of sleep data.
 * 
 * @param state Sleep state
 * @param movement_count Number of movement interrupts in this epoch
 */
void sleep_data_record_epoch(sleep_telemetry_state_t *state, uint8_t movement_count);

/**
 * Calculate restlessness index from sleep state and wake events.
 * 
 * Formula (integer-only):
 *   move_score (0-50) = (restless% + wake%) / 2, capped at 50
 *   wake_score (0-30) = wake_count × 5, capped at 30
 *   dur_score (0-20) = longest_wake_min, capped at 20
 *   restlessness = move_score + wake_score + dur_score
 * 
 * @param state Sleep state
 * @return Restlessness index (0-100, 0=perfect, 100=terrible)
 */
uint8_t sleep_data_calc_restlessness(const sleep_telemetry_state_t *state);

/**
 * Detect wake events from state buffer.
 * Updates wake_events structure in-place.
 * 
 * @param state Sleep state
 */
void sleep_data_detect_wake_events(sleep_telemetry_state_t *state);

/**
 * Accumulate telemetry data for the current hour.
 * Called once per hour from Phase Engine tick.
 * 
 * @param state Sleep state
 * @param hour Current hour (0-23)
 * @param current_zone Current Phase Engine zone
 * @param prev_zone Previous hour's zone
 * @param dominant_metric Dominant metric ID this hour
 * @param anomaly_fired True if anomaly fired this hour
 * @param light_minutes Minutes of light exposure this hour
 * @param motion_interrupts LIS2DW12 interrupts this hour
 * @param battery_mv Battery voltage in millivolts
 * @param sd_now Current Sleep Debt value
 * @param sd_prev Previous hour's Sleep Debt
 * @param em_now Current Emotional value
 * @param em_prev Previous hour's Emotional
 * @param wk_now Current Wake Momentum value
 * @param wk_prev Previous hour's Wake Momentum
 * @param comfort_now Current Comfort value
 * @param comfort_prev Previous hour's Comfort
 */
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
                                    uint8_t comfort_prev);

/**
 * Reset daily telemetry at midnight.
 * 
 * @param state Sleep state
 */
void sleep_data_reset_daily_telemetry(sleep_telemetry_state_t *state);

/**
 * Get sleep state at specific epoch index.
 * 
 * @param state Sleep state
 * @param epoch_idx Epoch index (0-959 for 8 hours)
 * @return Sleep state (0-3)
 */
sleep_state_t sleep_data_get_state_at_epoch(const sleep_telemetry_state_t *state, uint16_t epoch_idx);

/**
 * Set sleep state at specific epoch index.
 * 
 * @param state Sleep state
 * @param epoch_idx Epoch index (0-959 for 8 hours)
 * @param sleep_state State to set (0-3)
 */
void sleep_data_set_state_at_epoch(sleep_telemetry_state_t *state, uint16_t epoch_idx, sleep_state_t sleep_state);

// ============================================================================
// Phase 4F: WK Baseline Tracking (7-day rolling average)
// ============================================================================

/**
 * WK Baseline: Tracks daily movement totals over 7 days to compute
 * a personalized baseline for activity levels.
 */
typedef struct {
    uint16_t daily_totals[7];    // 7-day circular buffer (movements per day)
    uint8_t day_index;           // Current index in circular buffer (0-6)
    uint8_t baseline_per_hour;   // Computed baseline (movements/hour)
} wk_baseline_t;  // 17 bytes

/**
 * Update baseline with today's movement total.
 * Recalculates baseline as average hourly movement over valid days.
 * 
 * @param wk Baseline state
 * @param daily_movement Total movements for the day
 */
void wk_baseline_update_daily(wk_baseline_t *wk, uint16_t daily_movement);

/**
 * Get computed baseline movements per hour.
 * 
 * @param wk Baseline state
 * @return Baseline movements per hour (0-255)
 */
uint8_t wk_baseline_get_per_hour(const wk_baseline_t *wk);

/**
 * Get "active" threshold (baseline × 1.5).
 * 
 * @param wk Baseline state
 * @return Active threshold (0-255)
 */
uint8_t wk_baseline_get_active_threshold(const wk_baseline_t *wk);

/**
 * Get "very active" threshold (baseline × 2.0).
 * 
 * @param wk Baseline state
 * @return Very active threshold (0-255)
 */
uint8_t wk_baseline_get_very_active_threshold(const wk_baseline_t *wk);

#endif // PHASE_ENGINE_ENABLED

#endif // SLEEP_DATA_H_
