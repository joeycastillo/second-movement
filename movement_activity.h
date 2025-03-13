/*
 * MIT License
 *
 * Copyright (c) 2025 Joey Castillo
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

#pragma once

#ifdef HAS_ACCELEROMETER

#include <stdint.h>

// Log 36 hours of data points. Each data point captures 5 minutes.
#define MOVEMENT_NUM_DATA_POINTS (36 * (60 / 5))

typedef union {
    struct {
        uint32_t stationary_minutes: 3;
        uint32_t orientation_changes: 9;
        uint32_t measured_temperature: 10;
        uint32_t measured_light: 10;
    } bit;
    uint32_t reg;
} movement_activity_data_point;

/// @brief Internal function, called every 5 minutes to log data.
void _movement_log_data(void);

/// @brief Returns a pointer to the data log.
/// @param count is a pointer to a uint32_t. The absolute number of data points logged is returned by reference.
/// You can assume that log[count % MOVEMENT_NUM_DATA_POINTS] is the latest data point, and work backwards from there.
movement_activity_data_point *movement_get_data_log(uint32_t *count);

#endif