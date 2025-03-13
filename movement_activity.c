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

#include "movement.h"
#include "movement_activity.h"
#include "tc.h"
#include "thermistor_driver.h"

#ifdef HAS_ACCELEROMETER

// RAM to stash the data points.
movement_activity_data_point movement_activity_log[MOVEMENT_NUM_DATA_POINTS] = {0};
// the absolute number of data points logged
uint32_t data_points = 0;

// hacky: we're just tapping into Movement's global state for stationary detection.
// do we need better API for this? i'm less bothered now that it's all in Movement.
extern uint8_t stationary_minutes;

void _movement_log_data(void) {
    size_t pos = data_points % MOVEMENT_NUM_DATA_POINTS;
    movement_activity_data_point data_point = {0};

    // Movement tracks stationary minutes when deciding whether to sleep.
    data_point.bit.stationary_minutes = stationary_minutes;

    // orientation changes are counted in TC2. stash them in the data point...
    data_point.bit.orientation_changes = tc_count16_get_count(2);
    // ...and then reset the number of orientation changes.
    tc_count16_set_count(2, 0);

    // log the temperature
    thermistor_driver_enable();
    float temperature_c = thermistor_driver_get_temperature();
    thermistor_driver_disable();
    // offset the temperature by 30, so -30°C is 0, and 72.3°C is 102.3
    temperature_c = temperature_c + 30;
    if (temperature_c < 0) temperature_c = 0;
    if (temperature_c > 102.3) temperature_c = 102.3;
    // now we have can fit temperature into a 10-bit value
    data_point.bit.measured_temperature = temperature_c * 10;

    /// TODO: log light level

    // log the data point
    movement_activity_log[pos].reg = data_point.reg;
    data_points++;
}

movement_activity_data_point *movement_get_data_log(uint32_t *count) {
    *count = data_points;
    return movement_activity_log;
}

#endif
