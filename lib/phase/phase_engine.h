/*
 * MIT License
 *
 * Copyright (c) 2026 Diego Perez
 *
 * Phase Engine: Context-aware circadian rhythm tracking
 * 
 * This module computes a real-time "phase score" (0-100) representing
 * circadian alignment based on:
 * - Time of day and season (via homebase table)
 * - Current activity level
 * - Environmental inputs (temperature, light)
 * 
 * All computations use integer math for embedded efficiency.
 */

#ifndef PHASE_ENGINE_H_
#define PHASE_ENGINE_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef PHASE_ENGINE_ENABLED

// Anomaly flags (bitmask for active anomalies)
typedef enum {
    ANOMALY_NONE = 0,
    ANOMALY_SD_HIGH = (1 << 0),      // Sleep Debt ≥ 20 above 50
    ANOMALY_SD_LOW = (1 << 1),       // Sleep Debt ≤ 20 below 50
    ANOMALY_EM_HIGH = (1 << 2),      // Emotional ≥ 70 (very high)
    ANOMALY_EM_LOW = (1 << 3),       // Emotional ≤ 30 (very low)
    ANOMALY_ENERGY_HIGH = (1 << 4),  // Energy ≥ 70
    ANOMALY_ENERGY_LOW = (1 << 5),   // Energy ≤ 30
    ANOMALY_COMFORT_HIGH = (1 << 6), // Comfort ≥ 70
    ANOMALY_COMFORT_LOW = (1 << 7)   // Comfort ≤ 30
} anomaly_flags_t;

// Phase engine state (≤64 bytes RAM budget)
typedef struct {
    uint16_t last_phase_score;      // Most recent phase score (0-100)
    uint8_t last_hour;              // Last computed hour (0-23)
    uint16_t last_day_of_year;      // Last computed day (1-365), requires 9 bits
    uint16_t cumulative_phase;      // Rolling 24h sum for trends
    uint8_t phase_history[24];      // Hourly phase scores (circular buffer)
    uint8_t history_index;          // Current position in circular buffer
    uint8_t anomaly_flags;          // Active anomalies (bitmask)
    bool initialized;               // Has engine been initialized?
    uint16_t zone_check_streak;     // Consecutive days any zone face viewed
    uint16_t zone_last_check_day;   // Last day-of-year any zone viewed (1-365)
} phase_state_t;

// Homebase data point (one per day-of-year)
typedef struct {
    uint16_t expected_daylight_min; // Expected daylight duration (minutes)
    int16_t avg_temp_c10;           // Average temperature (celsius * 10)
    uint8_t seasonal_baseline;      // Seasonal energy baseline (0-100)
} homebase_entry_t;

/**
 * Initialize the phase engine state.
 * Call once at startup.
 */
void phase_engine_init(phase_state_t *state);

/**
 * Compute current phase score (0-100).
 * 
 * @param state Engine state (updated in-place)
 * @param hour Current hour (0-23)
 * @param day_of_year Current day (1-365)
 * @param activity_level Recent activity (0-1000, arbitrary units)
 * @param temp_c10 Current temperature (celsius * 10)
 * @param light_lux Current light level (lux)
 * @return Phase score (0-100), higher = better alignment
 */
uint16_t phase_compute(phase_state_t *state,
                       uint8_t hour,
                       uint16_t day_of_year,
                       uint16_t activity_level,
                       int16_t temp_c10,
                       uint16_t light_lux);

/**
 * Get phase trend over last N hours.
 * 
 * @param state Engine state
 * @param hours Number of hours to analyze (1-24)
 * @return Trend: -100 (declining) to +100 (improving)
 */
int16_t phase_get_trend(const phase_state_t *state, uint8_t hours);

/**
 * Get recommended action based on current phase and active hours.
 * 
 * Uses active hours configuration to determine activity periods:
 * - Outside active hours: rest/moderate only
 * - First 25% of active hours: early morning (gradual ramp)
 * - Middle 50% of active hours: peak activity window
 * - Last 25% of active hours: wind-down period
 * 
 * @param phase_score Current phase score (0-100)
 * @param hour Current hour (0-23)
 * @param active_hours_enabled True if active hours feature is enabled
 * @param active_start Active hours start (hour, 0-23)
 * @param active_end Active hours end (hour, 0-23)
 * @return Action code: 0=rest, 1=moderate, 2=active, 3=peak performance
 */
uint8_t phase_get_recommendation(uint16_t phase_score, 
                                 uint8_t hour,
                                 bool active_hours_enabled,
                                 uint8_t active_start,
                                 uint8_t active_end);

/**
 * Detect anomalies in metrics (values deviating ±20 from midpoint).
 * Updates state->anomaly_flags bitmask and triggers soft chime during active hours.
 * 
 * Algorithm:
 * - Threshold: |metric - 50| ≥ 20 (integer math)
 * - Active hours gating: only chimes during configured active hours (6-22 default)
 * - Soft chime: C7 @ 80ms
 * 
 * @param state Engine state (updated in-place with anomaly_flags)
 * @param sd Sleep Debt (-60 to +120, midpoint 30)
 * @param em Emotional (0-100, midpoint 50)
 * @param energy Energy (0-100, midpoint 50)
 * @param comfort Comfort (0-100, midpoint 50)
 * @param hour Current hour (0-23) for active hours gating
 * @param active_hours_enabled True if active hours feature is enabled
 * @param active_start Active hours start (hour, 0-23)
 * @param active_end Active hours end (hour, 0-23)
 */
void phase_detect_anomalies(phase_state_t *state,
                            int16_t sd,
                            uint8_t em,
                            uint8_t energy,
                            uint8_t comfort,
                            uint8_t hour,
                            bool active_hours_enabled,
                            uint8_t active_start,
                            uint8_t active_end);

#endif // PHASE_ENGINE_ENABLED

#endif // PHASE_ENGINE_H_
