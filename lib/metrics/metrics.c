/*
 * MIT License
 *
 * Copyright (c) 2026 Diego Perez
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifdef PHASE_ENGINE_ENABLED

#include "metrics.h"
#include "metric_sd.h"
#include "metric_em.h"
#include "metric_wk.h"
#include "metric_energy.h"
#include "metric_comfort.h"
#include "circadian_score.h"
#include "phase_engine.h"
#include "sensors.h"
#include "watch.h"

// Safe absolute value wrapper to handle INT16_MIN edge case
static inline int16_t safe_abs16(int16_t x) {
    return (x == INT16_MIN) ? INT16_MAX : ((x < 0) ? -x : x);
}

// Forward declaration from movement (defined in movement.c)
extern uint8_t movement_claim_backup_register(void);

// Current metric values (computed on each update)
static metrics_snapshot_t _current_metrics;

void metrics_init(metrics_engine_t *engine) {
    if (!engine) return;
    
    // Claim BKUP registers for persistent storage
    // Need 2 registers total: one for SD (3 bytes), one for WK (2 bytes)
    engine->bkup_reg_sd = movement_claim_backup_register();
    engine->bkup_reg_wk = movement_claim_backup_register();
    
    // Initialize state
    engine->sd_deficits[0] = 0;
    engine->sd_deficits[1] = 0;
    engine->sd_deficits[2] = 0;
    engine->wake_onset_hour = 0;
    engine->wake_onset_minute = 0;
    engine->last_update_hour = 0;
    engine->initialized = true;
    
    // Initialize current metrics to neutral
    _current_metrics.sd = 50;
    _current_metrics.em = 50;
    _current_metrics.wk = 50;
    _current_metrics.energy = 50;
    _current_metrics.comfort = 50;
    
    // Try to load from BKUP if registers were claimed successfully
    if (engine->bkup_reg_sd != 0 && engine->bkup_reg_wk != 0) {
        metrics_load_bkup(engine);
    }
}

void metrics_update(metrics_engine_t *engine,
                    const struct sensor_state_t *sensors,
                    uint8_t hour,
                    uint8_t minute,
                    uint16_t day_of_year,
                    uint8_t phase_score,
                    uint16_t cumulative_activity,
                    const circadian_data_t *sleep_data,
                    const homebase_entry_t *homebase,
                    bool has_accelerometer) {
    
    if (!engine || !engine->initialized) return;
    
    // Extract sensor values (with fallback defaults if sensors is NULL)
    int16_t temp_c10 = sensors ? (int16_t)sensors_get_temperature_c10(sensors) : 200;
    uint16_t light_lux = sensors ? sensors_get_lux_avg(sensors) : 0;
    uint16_t activity_variance = sensors ? sensors_get_motion_variance(sensors) : 50;
    uint16_t activity_level = sensors ? sensors_get_motion_intensity(sensors) : 0;
    
    // Update cadence tracking
    engine->last_update_hour = hour;
    
    // Get current date for lunar calculation
    watch_date_time_t now = watch_rtc_get_date_time();
    uint16_t year = 2020 + now.unit.year;  // Convert from offset to absolute year
    uint8_t month = now.unit.month;
    uint8_t day = now.unit.day;
    
    // --- Sleep Debt (SD) ---
    // Compute from sleep history and store deficits
    _current_metrics.sd = metric_sd_compute(sleep_data, engine->sd_deficits);
    
    // --- Comfort ---
    // Phase 4D: Now includes lunar component (20% weight)
    _current_metrics.comfort = metric_comfort_compute(temp_c10, light_lux, hour, homebase,
                                                     year, month, day);
    
    // --- Emotional (EM) ---
    // Compute from circadian cycle, lunar cycle, and activity variance
    _current_metrics.em = metric_em_compute(hour, day_of_year, activity_variance);
    
    // --- Wake Momentum (WK) ---
    // Calculate minutes awake from wake onset time
    // Use total minutes from midnight to handle wraparound correctly
    uint16_t wake_minutes = engine->wake_onset_hour * 60 + engine->wake_onset_minute;
    uint16_t current_minutes = hour * 60 + minute;
    
    // Handle midnight wraparound
    if (current_minutes < wake_minutes) {
        current_minutes += 1440;  // Add 24 hours in minutes
    }
    
    uint16_t minutes_awake = current_minutes - wake_minutes;
    _current_metrics.wk = metric_wk_compute(minutes_awake, cumulative_activity, has_accelerometer);
    
    // --- Energy ---
    // Derived from phase score, sleep debt, and activity/circadian bonus
    _current_metrics.energy = metric_energy_compute(phase_score, _current_metrics.sd, 
                                                     activity_level, hour, has_accelerometer);
    
    // Auto-save to BKUP on hourly boundary
    // (Only save SD and WK state, other metrics are derived)
    if (engine->bkup_reg_sd != 0 && engine->bkup_reg_wk != 0) {
        metrics_save_bkup(engine);
    }
}

void metrics_get(const metrics_engine_t *engine, metrics_snapshot_t *out) {
    if (!out) return;
    
    // Copy current metrics to output
    *out = _current_metrics;
}

void metrics_save_bkup(const metrics_engine_t *engine) {
    if (!engine || engine->bkup_reg_sd == 0 || engine->bkup_reg_wk == 0) return;
    
    // Pack SD state into BKUP[bkup_reg_sd] (3 bytes)
    // Format: [deficit[0], deficit[1], deficit[2], unused]
    uint32_t sd_data = 0;
    sd_data |= (uint32_t)engine->sd_deficits[0];
    sd_data |= ((uint32_t)engine->sd_deficits[1]) << 8;
    sd_data |= ((uint32_t)engine->sd_deficits[2]) << 16;
    watch_store_backup_data(sd_data, engine->bkup_reg_sd);
    
    // Pack WK state into BKUP[bkup_reg_wk] (2 bytes)
    // Format: [wake_onset_hour, wake_onset_minute, unused, unused]
    uint32_t wk_data = 0;
    wk_data |= (uint32_t)engine->wake_onset_hour;
    wk_data |= ((uint32_t)engine->wake_onset_minute) << 8;
    watch_store_backup_data(wk_data, engine->bkup_reg_wk);
}

void metrics_load_bkup(metrics_engine_t *engine) {
    if (!engine || engine->bkup_reg_sd == 0 || engine->bkup_reg_wk == 0) return;
    
    // Load SD state from BKUP
    uint32_t sd_data = watch_get_backup_data(engine->bkup_reg_sd);
    engine->sd_deficits[0] = (uint8_t)(sd_data & 0xFF);
    engine->sd_deficits[1] = (uint8_t)((sd_data >> 8) & 0xFF);
    engine->sd_deficits[2] = (uint8_t)((sd_data >> 16) & 0xFF);
    
    // Clamp SD deficits to valid range [0-100]
    if (engine->sd_deficits[0] > 100) engine->sd_deficits[0] = 0;
    if (engine->sd_deficits[1] > 100) engine->sd_deficits[1] = 0;
    if (engine->sd_deficits[2] > 100) engine->sd_deficits[2] = 0;
    
    // Load WK state from BKUP
    uint32_t wk_data = watch_get_backup_data(engine->bkup_reg_wk);
    engine->wake_onset_hour = (uint8_t)(wk_data & 0xFF);
    engine->wake_onset_minute = (uint8_t)((wk_data >> 8) & 0xFF);
    
    // Validate and clamp loaded time values
    if (engine->wake_onset_hour >= 24) engine->wake_onset_hour = 0;
    if (engine->wake_onset_minute >= 60) engine->wake_onset_minute = 0;
}

void metrics_set_wake_onset(metrics_engine_t *engine, uint8_t hour, uint8_t minute) {
    if (!engine) return;
    
    // Validate and clamp inputs
    if (hour >= 24) hour = 0;
    if (minute >= 60) minute = 0;
    
    // Update wake onset time
    engine->wake_onset_hour = hour;
    engine->wake_onset_minute = minute;
    
    // Save to BKUP immediately (important for WK metric persistence)
    if (engine->bkup_reg_wk != 0) {
        uint32_t wk_data = 0;
        wk_data |= (uint32_t)engine->wake_onset_hour;
        wk_data |= ((uint32_t)engine->wake_onset_minute) << 8;
        watch_store_backup_data(wk_data, engine->bkup_reg_wk);
    }
}

uint8_t metrics_get_dominant(const metrics_snapshot_t *snapshot, uint8_t zone) {
    if (!snapshot) return 0;  // Default to SD
    
    // Calculate absolute deviation from neutral (50) for each metric
    // Higher deviation = more "active" / driving the current state
    uint8_t sd_dev = (snapshot->sd >= 50) ? (snapshot->sd - 50) : (50 - snapshot->sd);
    uint8_t em_dev = (snapshot->em >= 50) ? (snapshot->em - 50) : (50 - snapshot->em);
    uint8_t wk_dev = (snapshot->wk >= 50) ? (snapshot->wk - 50) : (50 - snapshot->wk);
    uint8_t comfort_dev = (snapshot->comfort >= 50) ? (snapshot->comfort - 50) : (50 - snapshot->comfort);
    
    // Find metric with highest deviation
    uint8_t max_dev = sd_dev;
    uint8_t dominant = 0;  // DOMINANT_SD
    
    if (em_dev > max_dev) {
        max_dev = em_dev;
        dominant = 1;  // DOMINANT_EM
    }
    
    if (wk_dev > max_dev) {
        max_dev = wk_dev;
        dominant = 2;  // DOMINANT_WK
    }
    
    if (comfort_dev > max_dev) {
        dominant = 3;  // DOMINANT_COMFORT
    }
    
    return dominant;
}

#endif // PHASE_ENGINE_ENABLED
