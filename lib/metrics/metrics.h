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

#ifndef METRICS_H_
#define METRICS_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef PHASE_ENGINE_ENABLED

#include "circadian_score.h"
#include "phase_engine.h"

// Forward declaration for sensor state
struct sensor_state_t;

/**
 * Metric Engine: Compute biological state metrics from sensors and sleep data
 * 
 * Six metrics (0-100 each):
 * - SD (Sleep Debt): Rolling 3-night weighted sleep deficit
 * - EM (Emotional): Circadian + lunar + activity variance
 * - WK (Wake Momentum): Ramp from sleep onset to full alertness
 * - Energy: Phase-aligned capacity (derived from phase + SD + activity)
 * - JL (Jet Lag): Timezone shift disruption (Phase 4 - stubbed in Phase 3)
 * - Comfort: Environmental alignment (temp + light vs homebase)
 */

// Metric snapshot - all current values
typedef struct {
    uint8_t sd;       // Sleep Debt (0=rested, 100=exhausted)
    uint8_t em;       // Emotional/Mood (0=low, 100=elevated)
    uint8_t wk;       // Wake Momentum (0=just woke, 100=fully alert)
    uint8_t energy;   // Energy capacity (0=depleted, 100=peak)
    uint8_t comfort;  // Environmental comfort (0=deviation, 100=aligned)
} metrics_snapshot_t;

// Internal metric engine state (~32 bytes)
typedef struct {
    // Sleep Debt state (3 bytes in BKUP)
    uint8_t sd_deficits[3];  // 3-night rolling deficit (0-100 each), packed as deficit/4
    
    // Wake Momentum state (2 bytes in BKUP)
    uint8_t wake_onset_hour;
    uint8_t wake_onset_minute;
    
    // Runtime state (not persisted)
    uint8_t last_update_hour;
    bool initialized;
    
    // BKUP register indices (claimed at init)
    uint8_t bkup_reg_sd;      // BKUP register for SD state (3 bytes)
    uint8_t bkup_reg_wk;      // BKUP register for WK state (2 bytes)
} metrics_engine_t;

/**
 * Initialize metric engine and claim BKUP registers.
 * Call once at startup.
 * 
 * @param engine Engine state (zeroed by caller)
 */
void metrics_init(metrics_engine_t *engine);

/**
 * Update all metrics based on current sensor data and time.
 * 
 * @param engine Engine state
 * @param sensors Sensor state (motion, temp, light data)
 * @param hour Current hour (0-23)
 * @param minute Current minute (0-59)
 * @param day_of_year Current day (1-365)
 * @param phase_score Current phase score from phase_engine (0-100)
 * @param cumulative_activity Cumulative activity since wake (0-65535, for WK bonus)
 * @param sleep_data Circadian sleep history (7-night buffer)
 * @param homebase Homebase entry for current day (seasonal baseline)
 * @param has_accelerometer True if LIS2DW accelerometer is available
 */
void metrics_update(metrics_engine_t *engine,
                    const struct sensor_state_t *sensors,
                    uint8_t hour,
                    uint8_t minute,
                    uint16_t day_of_year,
                    uint8_t phase_score,
                    uint16_t cumulative_activity,
                    const circadian_data_t *sleep_data,
                    const homebase_entry_t *homebase,
                    bool has_accelerometer);

/**
 * Get current metric values.
 * 
 * @param engine Engine state
 * @param out Output snapshot (filled by this function)
 */
void metrics_get(const metrics_engine_t *engine, metrics_snapshot_t *out);

/**
 * Save metric state to BKUP registers.
 * Call before entering low-power mode.
 * 
 * @param engine Engine state
 */
void metrics_save_bkup(const metrics_engine_t *engine);

/**
 * Load metric state from BKUP registers.
 * Call after waking from backup mode.
 * 
 * @param engine Engine state
 */
void metrics_load_bkup(metrics_engine_t *engine);

/**
 * Set wake onset time (for WK metric calculation).
 * Call when user wakes up or at sleep->wake transition.
 * 
 * @param engine Engine state
 * @param hour Wake onset hour (0-23)
 * @param minute Wake onset minute (0-59)
 */
void metrics_set_wake_onset(metrics_engine_t *engine, uint8_t hour, uint8_t minute);

/**
 * Get dominant metric ID for current zone.
 * Determines which metric (SD/EM/WK/Comfort) has highest deviation from neutral (50).
 * Used for Phase 4E telemetry to track which metric is driving zone state.
 * 
 * @param snapshot Current metric values
 * @param zone Current phase zone (0-3: Emergence/Momentum/Active/Descent)
 * @return Dominant metric ID (0=SD, 1=EM, 2=WK, 3=Comfort)
 */
uint8_t metrics_get_dominant(const metrics_snapshot_t *snapshot, uint8_t zone);

#endif // PHASE_ENGINE_ENABLED

#endif // METRICS_H_
