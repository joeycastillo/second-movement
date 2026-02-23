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

#include "metric_comfort.h"
#include "phase_engine.h"

// Conway's lunar approximation (±1 day accuracy)
// Returns moon age in days (0-29)
static uint8_t lunar_age_conway(uint16_t year, uint8_t month, uint8_t day) {
    // Conway's formula for moon age
    // r = year % 100
    // r = r % 19
    // if (month < 3) { month += 12; year -= 1; }
    // age = ((r * 11) % 30 + month * 2 + day) % 30
    
    uint16_t r = year % 100;
    r = r % 19;
    
    // Adjust for Jan/Feb (treat as months 13/14 of previous year)
    uint8_t adj_month = month;
    if (month < 3) {
        adj_month += 12;
        // Don't modify year for calculation - only affects r
    }
    
    // Conway's magic formula
    uint8_t age = ((r * 11) % 30 + adj_month * 2 + day) % 30;
    
    return age;
}

// Map lunar phase (0-29 days) to comfort modifier (0-100)
// Full moon (day 15) = 100 (peak comfort)
// New moon (day 0/29) = 0 (lowest comfort)
// Linear interpolation between
static uint8_t lunar_comfort_score(uint8_t lunar_age) {
    // Map 0-29 to comfort score
    // Full moon at day 15 → 100
    // New moon at day 0/29 → 0
    
    if (lunar_age <= 15) {
        // Waxing: 0→15 maps to 0→100
        return (lunar_age * 100) / 15;
    } else {
        // Waning: 16→29 maps to 93→0
        uint8_t days_past_full = lunar_age - 15;  // 1-14
        return 100 - ((days_past_full * 100) / 15);
    }
}

// Safe absolute value wrapper to handle INT16_MIN edge case
static inline int16_t safe_abs16(int16_t x) {
    return (x == INT16_MIN) ? INT16_MAX : ((x < 0) ? -x : x);
}

// Helper: min function
static inline uint16_t min16(uint16_t a, uint16_t b) {
    return (a < b) ? a : b;
}

uint8_t metric_comfort_compute(int16_t temp_c10,
                                uint16_t light_lux,
                                uint8_t hour,
                                const homebase_entry_t *baseline,
                                uint16_t year,
                                uint8_t month,
                                uint8_t day) {
    if (!baseline) {
        // No baseline data - return neutral score
        return 50;
    }
    
    // Temp comfort (48%): deviation from seasonal baseline
    // Use int32_t to prevent overflow in subtraction
    int32_t temp_diff = (int32_t)temp_c10 - (int32_t)baseline->avg_temp_c10;
    int32_t temp_dev = (temp_diff < 0) ? -temp_diff : temp_diff;
    
    // Penalty: divide by 3 to convert temp deviation (in 0.1°C) to comfort penalty
    // Cap at 30°C deviation (300 units)
    uint8_t temp_comfort;
    if (temp_dev > 300) {
        temp_comfort = 0;
    } else {
        temp_comfort = 100 - (uint8_t)(temp_dev / 3);
    }
    
    // Light comfort (32%): expected vs actual for hour
    // Use homebase daylight data to determine sunrise/sunset
    // Approximate sunrise: 12:00 - (daylight_min / 120) hours
    // Approximate sunset: 12:00 + (daylight_min / 120) hours
    uint16_t daylight_min = baseline->expected_daylight_min;
    uint8_t sunrise_hour = 12 - (daylight_min / 120);
    uint8_t sunset_hour = 12 + (daylight_min / 120);
    
    uint8_t light_comfort;
    if (hour >= sunrise_hour && hour <= sunset_hour) {
        // Daytime: expect bright light (>= 200 lux)
        if (light_lux >= 200) {
            light_comfort = 100;
        } else {
            // Penalty for dim daytime: scale 0-200 lux to 0-100 comfort
            light_comfort = (uint8_t)((light_lux * 100) / 200);
        }
    } else {
        // Nighttime: expect dark (<= 50 lux)
        if (light_lux <= 50) {
            light_comfort = 100;
        } else {
            // Penalty for bright nighttime: scale above 50 lux
            // Example: 150 lux at night → 100 lux over threshold → 50 point penalty → 50 comfort
            // Cast to uint32_t to prevent overflow when light_lux is near UINT16_MAX
            uint16_t penalty = min16(100, ((uint32_t)(light_lux - 50)) / 2);
            light_comfort = 100 - (uint8_t)penalty;
        }
    }
    
    // Lunar comfort (20%): Conway approximation
    uint8_t lunar_age = lunar_age_conway(year, month, day);
    uint8_t lunar_comfort = lunar_comfort_score(lunar_age);
    
    // Blend: 48% temp + 32% light + 20% lunar
    // Scale to avoid overflow: use (a*48 + b*32 + c*20) / 100
    uint32_t comfort = ((uint32_t)temp_comfort * 48 + 
                       (uint32_t)light_comfort * 32 + 
                       (uint32_t)lunar_comfort * 20) / 100;
    
    return (uint8_t)comfort;
}

#endif // PHASE_ENGINE_ENABLED
