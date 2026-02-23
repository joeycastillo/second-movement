/*
 * MIT License
 *
 * Copyright (c) 2026 Diego Perez
 *
 * Phase Engine implementation
 * 
 * Real circadian rhythm computation using integer math.
 * Tracks alignment with natural cycles based on time, season,
 * and environmental inputs.
 */

#include "phase_engine.h"

#ifdef PHASE_ENGINE_ENABLED

#include "homebase.h"
#include "watch.h"
#include <string.h>

/*
 * Integer cosine lookup table (24 entries, one per hour)
 * Values scaled to ±1000 to preserve precision
 * cos(2π * hour / 24) * 1000
 * Peak at hour 14 (2 PM), trough at hour 2 (2 AM)
 */
static const int16_t cosine_lut_24[24] = {
    866,   // 00:00
    707,   // 01:00
    500,   // 02:00
    259,   // 03:00
    0,     // 04:00
    -259,  // 05:00
    -500,  // 06:00
    -707,  // 07:00
    -866,  // 08:00
    -966,  // 09:00
    -1000, // 10:00
    -966,  // 11:00
    -866,  // 12:00
    -707,  // 13:00
    -500,  // 14:00
    -259,  // 15:00
    0,     // 16:00
    259,   // 17:00
    500,   // 18:00
    707,   // 19:00
    866,   // 20:00
    966,   // 21:00
    1000,  // 22:00
    966    // 23:00
};

void phase_engine_init(phase_state_t *state) {
    // Clear all state
    memset(state, 0, sizeof(phase_state_t));
    state->initialized = true;
}

uint16_t phase_compute(phase_state_t *state,
                       uint8_t hour,
                       uint16_t day_of_year,
                       uint16_t activity_level,
                       int16_t temp_c10,
                       uint16_t light_lux) {
    
    if (!state->initialized) {
        phase_engine_init(state);
    }
    
    // Input validation (from PR #56)
    if (hour > 23 || day_of_year < 1 || day_of_year > 366) {
        return 0;  // Invalid input
    }
    
    if (activity_level > 1000) {
        return 0;  // Invalid input
    }
    
    // Get seasonal baseline
    const homebase_entry_t* baseline = homebase_get_entry(day_of_year);
    if (baseline == NULL) {
        return 0;  // Error - no baseline data
    }
    
    // Calculate circadian curve (expected activity level at this hour)
    // Peak at 14:00 (afternoon), trough at 02:00 (night)
    int16_t circadian_curve = cosine_lut_24[hour];
    
    // Expected activity: baseline * circadian_curve
    // baseline.seasonal_baseline is 0-100
    // circadian_curve is -1000 to +1000
    // Result scaled to 0-100 range
    int16_t expected_activity = ((int32_t)baseline->seasonal_baseline * 
                                 (1000 + circadian_curve)) / 2000;
    
    // Activity deviation (scaled to 0-100 range)
    // activity_level is 0-1000, scale to 0-100
    int16_t actual_activity = activity_level / 10;
    int16_t activity_dev = (actual_activity > expected_activity) 
                          ? (actual_activity - expected_activity)
                          : (expected_activity - actual_activity);
    
    // Temperature deviation (scaled to 0-30 range)
    // Both are in celsius * 10
    int16_t temp_dev = (temp_c10 > baseline->avg_temp_c10)
                      ? (temp_c10 - baseline->avg_temp_c10)
                      : (baseline->avg_temp_c10 - temp_c10);
    // Scale to penalty (max penalty ~30 for 30°C deviation)
    temp_dev = (temp_dev > 300) ? 30 : (temp_dev / 10);
    
    // Light deviation (simplified scoring)
    // Use homebase daylight data to determine day/night
    // Approximate sunrise: 12:00 - (daylight_min / 120) hours
    // Approximate sunset: 12:00 + (daylight_min / 120) hours
    uint16_t daylight_min = baseline->expected_daylight_min;
    uint8_t sunrise_hour = 12 - (daylight_min / 120);
    uint8_t sunset_hour = 12 + (daylight_min / 120);
    
    uint16_t expected_light = (hour >= sunrise_hour && hour < sunset_hour) ? 500 : 50;
    int16_t light_dev = (light_lux > expected_light)
                       ? (light_lux - expected_light)
                       : (expected_light - light_lux);
    // Scale to penalty (max ~20)
    light_dev = (light_dev > 1000) ? 20 : (light_dev / 50);
    
    // Compute final phase score
    // Start at 100, subtract deviations
    int16_t score = 100 - (activity_dev / 2) - temp_dev - light_dev;
    
    // Clamp to valid range
    if (score < 0) score = 0;
    if (score > 100) score = 100;
    
    state->last_phase_score = (uint16_t)score;
    state->last_hour = hour;
    state->last_day_of_year = day_of_year;
    
    // Update circular buffer
    uint8_t old_score = state->phase_history[state->history_index];
    state->phase_history[state->history_index] = (uint8_t)score;
    
    // Update cumulative sum with proper saturating arithmetic
    // Saturating subtract
    if (state->cumulative_phase >= old_score) {
        state->cumulative_phase -= old_score;
    } else {
        state->cumulative_phase = 0;
    }
    
    // Saturating add
    uint32_t temp = (uint32_t)state->cumulative_phase + score;
    if (temp > UINT16_MAX) {
        state->cumulative_phase = UINT16_MAX;
    } else {
        state->cumulative_phase = (uint16_t)temp;
    }
    
    // THEN increment index
    state->history_index = (state->history_index + 1) % 24;
    
    return (uint16_t)score;
}

int16_t phase_get_trend(const phase_state_t *state, uint8_t hours) {
    if (hours == 0 || hours > 24) {
        return 0;
    }
    
    // Calculate average of first half vs second half
    uint8_t half = hours / 2;
    if (half == 0) half = 1;
    
    // Sum recent half (more recent)
    int32_t recent_sum = 0;
    uint8_t recent_count = 0;
    
    // Sum older half (earlier)
    int32_t older_sum = 0;
    uint8_t older_count = 0;
    
    // Work backwards from current position
    int8_t idx = state->history_index - 1;
    if (idx < 0) idx = 23;
    
    for (uint8_t i = 0; i < hours; i++) {
        if (i < half) {
            recent_sum += state->phase_history[idx];
            recent_count++;
        } else {
            older_sum += state->phase_history[idx];
            older_count++;
        }
        
        idx--;
        if (idx < 0) idx = 23;
    }
    
    if (recent_count == 0 || older_count == 0) {
        return 0;
    }
    
    // Calculate averages
    int16_t recent_avg = recent_sum / recent_count;
    int16_t older_avg = older_sum / older_count;
    
    // Trend is difference (scaled to -100 to +100)
    int16_t trend = recent_avg - older_avg;
    
    // Amplify small trends for better resolution
    trend = trend * 2;
    
    // Clamp to valid range
    if (trend < -100) trend = -100;
    if (trend > 100) trend = 100;
    
    return trend;
}

uint8_t phase_get_recommendation(uint16_t phase_score, 
                                 uint8_t hour,
                                 bool active_hours_enabled,
                                 uint8_t active_start,
                                 uint8_t active_end) {
    // Recommendation codes:
    // 0 = rest
    // 1 = moderate activity
    // 2 = active
    // 3 = peak performance
    
    // Check if we're within active hours
    bool in_active_hours;
    if (!active_hours_enabled) {
        in_active_hours = true;  // If disabled, always active
    } else if (active_start < active_end) {
        in_active_hours = (hour >= active_start && hour < active_end);
    } else if (active_start > active_end) {
        // Wraparound case (e.g., 22:00-06:00)
        in_active_hours = (hour >= active_start || hour < active_end);
    } else {
        // start == end: 24-hour active
        in_active_hours = true;
    }
    
    // Outside active hours: prefer rest/moderate only
    if (!in_active_hours) {
        if (phase_score < 30) return 0;  // Rest
        if (phase_score < 70) return 1;  // Moderate
        return 1;  // Even high scores -> moderate when resting
    }
    
    // Within active hours: calculate activity periods
    // Calculate active duration (handling wraparound)
    uint8_t active_duration;
    if (active_start < active_end) {
        active_duration = active_end - active_start;
    } else {
        active_duration = (24 - active_start) + active_end;
    }
    
    // Calculate position within active hours (0 to active_duration-1)
    uint8_t hours_into_active;
    if (active_start < active_end) {
        hours_into_active = hour - active_start;
    } else {
        // Wraparound case
        if (hour >= active_start) {
            hours_into_active = hour - active_start;
        } else {
            hours_into_active = (24 - active_start) + hour;
        }
    }
    
    // Define activity periods:
    // First 25% = early morning (gradual ramp)
    // Middle 50% = peak activity window
    // Last 25% = wind-down period
    uint8_t early_morning_end = active_duration / 4;  // First 25%
    uint8_t wind_down_start = (active_duration * 3) / 4;  // Last 25%
    
    if (hours_into_active < early_morning_end) {
        // Early active hours: gradual ramp
        if (phase_score < 30) return 0;
        if (phase_score < 50) return 1;
        if (phase_score < 70) return 2;
        return 3;
    } else if (hours_into_active >= wind_down_start) {
        // Wind-down period: moderate activity preferred
        if (phase_score < 30) return 0;
        if (phase_score < 50) return 1;
        if (phase_score < 70) return 2;
        return 2;  // Cap at active, not peak
    } else {
        // Peak activity window: full range available
        if (phase_score < 30) return 0;
        if (phase_score < 50) return 1;
        if (phase_score < 70) return 2;
        return 3;
    }
}

// Helper: Check if current hour is within active hours
static bool is_active_hours(uint8_t hour, 
                           bool enabled, 
                           uint8_t start, 
                           uint8_t end) {
    if (!enabled) {
        return true;  // If disabled, always active
    }
    
    // Handle wraparound case (e.g., 22:00 - 06:00)
    if (start < end) {
        return (hour >= start && hour < end);
    } else if (start > end) {
        return (hour >= start || hour < end);
    } else {
        // start == end: 24-hour active
        return true;
    }
}

void phase_detect_anomalies(phase_state_t *state,
                            int16_t sd,
                            uint8_t em,
                            uint8_t energy,
                            uint8_t comfort,
                            uint8_t hour,
                            bool active_hours_enabled,
                            uint8_t active_start,
                            uint8_t active_end) {
    if (!state || !state->initialized) {
        return;
    }
    
    // Check if we're in active hours
    bool in_active_hours = is_active_hours(hour, active_hours_enabled, 
                                          active_start, active_end);
    
    // Store previous anomaly flags to detect new anomalies
    uint8_t prev_flags = state->anomaly_flags;
    uint8_t new_flags = ANOMALY_NONE;
    
    // Detect Sleep Debt anomalies (midpoint 30, threshold ±20)
    // SD range: -60 to +120, midpoint = 30
    if (sd >= 50) {  // 30 + 20
        new_flags |= ANOMALY_SD_HIGH;
    } else if (sd <= 10) {  // 30 - 20
        new_flags |= ANOMALY_SD_LOW;
    }
    
    // Detect Emotional anomalies (midpoint 50, threshold ±20)
    if (em >= 70) {  // 50 + 20
        new_flags |= ANOMALY_EM_HIGH;
    } else if (em <= 30) {  // 50 - 20
        new_flags |= ANOMALY_EM_LOW;
    }
    
    // Detect Energy anomalies (midpoint 50, threshold ±20)
    if (energy >= 70) {
        new_flags |= ANOMALY_ENERGY_HIGH;
    } else if (energy <= 30) {
        new_flags |= ANOMALY_ENERGY_LOW;
    }
    
    // Detect Comfort anomalies (midpoint 50, threshold ±20)
    if (comfort >= 70) {
        new_flags |= ANOMALY_COMFORT_HIGH;
    } else if (comfort <= 30) {
        new_flags |= ANOMALY_COMFORT_LOW;
    }
    
    // Update state
    state->anomaly_flags = new_flags;
    
    // Trigger soft chime if new anomalies detected during active hours
    // Only chime if there are new flags that weren't present before
    uint8_t newly_detected = new_flags & ~prev_flags;
    
    if (in_active_hours && newly_detected != ANOMALY_NONE) {
        // Soft chime: C7 @ 80ms
        watch_buzzer_play_note(BUZZER_NOTE_C7, 80);
    }
}

#endif // PHASE_ENGINE_ENABLED
