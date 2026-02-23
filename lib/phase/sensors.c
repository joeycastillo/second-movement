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

#include "sensors.h"
#include "lis2dw.h"
#include "watch.h"
#include "movement.h"
#include <string.h>
#include <stdlib.h>

static uint16_t _compute_variance(const uint16_t *buffer, uint8_t count);
static uint16_t _compute_intensity(uint16_t current_mag, uint16_t prev_smoothed);
static uint16_t _abs16(int16_t val);

void sensors_init(struct sensor_state_t *state, bool has_accel) {
    memset(state, 0, sizeof(struct sensor_state_t));
    state->has_accelerometer = has_accel;
    
    // Note: Thermistor initialization is handled by Movement
    
    state->initialized = true;
}

void sensors_configure_accel(struct sensor_state_t *state) {
    if (!state || !state->initialized || !state->has_accelerometer) {
        return;
    }
    
    lis2dw_set_mode(LIS2DW_MODE_LOW_POWER);
    lis2dw_set_low_power_mode(LIS2DW_LP_MODE_1);
    lis2dw_set_data_rate(LIS2DW_DATA_RATE_LOWEST);
    lis2dw_set_low_noise_mode(false);
    lis2dw_set_range(LIS2DW_RANGE_2_G);
    lis2dw_configure_wakeup_threshold(1);
    lis2dw_enable_sleep();
    lis2dw_enable_stationary_motion_detection();
    lis2dw_configure_int1(LIS2DW_CTRL4_INT1_WU);
}

void sensors_update(struct sensor_state_t *state) {
    if (!state || !state->initialized) {
        return;
    }
    
    // PR #65: Update motion tracking
    if (!state->has_accelerometer) {
        state->motion_active = false;
        state->motion_variance = 0;
        state->motion_intensity = 0;
        state->motion_magnitude = 0;
    } else {
        uint8_t wake_src = lis2dw_get_wakeup_source();
        bool is_awake = (wake_src & LIS2DW_WAKEUP_SRC_WAKEUP) != 0;
        bool is_sleeping = (wake_src & LIS2DW_WAKEUP_SRC_SLEEP_STATE) != 0;
        
        if (is_awake) {
            state->motion_active = true;
            state->inactivity_minutes = 0;
            
            // Phase 4E: Count movement interrupt for sleep tracking
            if (state->epoch_movement_count < 255) {
                state->epoch_movement_count++;
            }
            if (state->hourly_movement_count < 255) {
                state->hourly_movement_count++;
            }
        } else if (is_sleeping) {
            state->inactivity_minutes += 15;
            if (state->inactivity_minutes >= SENSOR_INACTIVITY_MIN) {
                state->motion_active = false;
            }
        }
        
        lis2dw_reading_t raw = lis2dw_get_raw_reading();
        uint16_t mag = _abs16(raw.x) + _abs16(raw.y) + _abs16(raw.z);
        
        state->motion_buffer[state->motion_buf_idx] = mag;
        state->motion_buf_idx = (state->motion_buf_idx + 1) % SENSOR_MOTION_BUFFER_SIZE;
        if (state->motion_buf_count < SENSOR_MOTION_BUFFER_SIZE) {
            state->motion_buf_count++;
        }
        
        state->motion_magnitude = mag;
        state->motion_variance = _compute_variance(state->motion_buffer, state->motion_buf_count);
        state->motion_intensity = _compute_intensity(mag, state->motion_intensity);
    }
    
    // PR #66: Update lux + temperature
    sensors_sample_lux(state);
    sensors_sample_temperature(state);
}

uint16_t sensors_get_motion_variance(const struct sensor_state_t *state) {
    return state ? state->motion_variance : 0;
}

uint16_t sensors_get_motion_intensity(const struct sensor_state_t *state) {
    return state ? state->motion_intensity : 0;
}

bool sensors_is_motion_active(const struct sensor_state_t *state) {
    return state ? state->motion_active : false;
}

static uint16_t _compute_variance(const uint16_t *buffer, uint8_t count) {
    if (count < 2) return 0;
    
    uint32_t sum = 0;
    for (uint8_t i = 0; i < count; i++) {
        sum += buffer[i];
    }
    uint16_t mean = (uint16_t)(sum / count);
    
    uint32_t sq_sum = 0;
    for (uint8_t i = 0; i < count; i++) {
        int32_t diff = (int32_t)buffer[i] - (int32_t)mean;
        sq_sum += (uint32_t)(diff * diff);
    }
    
    uint32_t var = sq_sum / count;
    return (var > UINT16_MAX) ? UINT16_MAX : (uint16_t)var;
}

static uint16_t _compute_intensity(uint16_t current_mag, uint16_t prev_smoothed) {
    uint16_t scaled_current = current_mag / 32;
    if (scaled_current > 1000) scaled_current = 1000;
    
    uint32_t smoothed = ((uint32_t)prev_smoothed * 3 + scaled_current) / 4;
    return (uint16_t)smoothed;
}

static uint16_t _abs16(int16_t val) {
    return (val < 0) ? (uint16_t)(-val) : (uint16_t)val;
}

// ============================================================================
// PR #66: Lux + Temperature Integration
// ============================================================================

void sensors_sample_lux(struct sensor_state_t *state) {
    if (!state || !state->initialized) {
        return;
    }
    
#if HAS_LIGHT_SENSOR
    // Pro board: sample ADC, update rolling average
    watch_enable_adc();
    
    // Read ambient light from A2 pin (light sensor on Pro board)
    uint16_t raw = watch_get_analog_pin_level(HAL_GPIO_A2_pin());
    
    watch_disable_adc();
    
    // Convert raw ADC to approximate lux
    // Raw 0-65535 → roughly 0-10000 lux (calibration TBD during dogfooding)
    // Simple linear mapping: lux = raw / 6 (gives ~0-10922 range)
    uint16_t lux = raw / 6;
    
    // Add to circular buffer
    state->lux_buffer[state->lux_buf_idx] = lux;
    state->lux_buf_idx = (state->lux_buf_idx + 1) % SENSOR_LUX_BUFFER_SIZE;
    if (state->lux_buf_count < SENSOR_LUX_BUFFER_SIZE) {
        state->lux_buf_count++;
    }
    
    // Compute 5-min rolling average
    uint32_t sum = 0;
    for (uint8_t i = 0; i < state->lux_buf_count; i++) {
        sum += state->lux_buffer[i];
    }
    
    // Guard against division by zero
    if (state->lux_buf_count == 0) {
        state->lux_avg = 0;
        return;
    }
    
    state->lux_avg = (uint16_t)(sum / state->lux_buf_count);
#else
    // Non-Pro boards: no light sensor
    state->lux_avg = 0;
#endif
}

void sensors_sample_temperature(struct sensor_state_t *state) {
    if (!state || !state->initialized) {
        return;
    }
    
    // Use existing Movement API (handles thermistor + LIS2DW fallback)
    // movement_get_temperature() checks movement_state.has_thermistor and
    // falls back to LIS2DW12 internal temp sensor if no thermistor available
    float temp_c = movement_get_temperature();
    
    // Convert to 0.1°C units (e.g., 20.5°C → 205)
    if (temp_c == (float)0xFFFFFFFF) {
        // No temperature sensor available, use reasonable fallback
        state->temperature_c10 = 200;  // 20.0°C (room temperature)
    } else {
        // Convert float to 0.1°C units with rounding
        state->temperature_c10 = (uint16_t)((temp_c * 10.0f) + 0.5f);
    }
}

uint16_t sensors_get_lux_avg(const struct sensor_state_t *state) {
    return state ? state->lux_avg : 0;
}

uint16_t sensors_get_temperature_c10(const struct sensor_state_t *state) {
    return state ? state->temperature_c10 : 0;
}

// ============================================================================
// Phase 4E: Sleep Tracking Helpers
// ============================================================================

void sensors_tick_epoch(struct sensor_state_t *state) {
    if (!state || !state->initialized) {
        return;
    }
    
    state->epoch_seconds++;
    
    // Check if light is currently detected (lux > threshold)
    // Threshold: 10 lux (dim light detection)
    if (state->lux_avg > 10 && state->hourly_light_minutes < 60) {
        state->hourly_light_minutes++;
    }
}

uint8_t sensors_get_epoch_movement_count(const struct sensor_state_t *state) {
    return state ? state->epoch_movement_count : 0;
}

void sensors_reset_hourly_counters(struct sensor_state_t *state) {
    if (!state || !state->initialized) {
        return;
    }
    
    state->hourly_light_minutes = 0;
    state->hourly_movement_count = 0;
}

uint8_t sensors_get_hourly_light_minutes(const struct sensor_state_t *state) {
    return state ? state->hourly_light_minutes : 0;
}

uint8_t sensors_get_hourly_movement_count(const struct sensor_state_t *state) {
    return state ? state->hourly_movement_count : 0;
}

#endif // PHASE_ENGINE_ENABLED
