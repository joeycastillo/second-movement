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
#include "filesystem.h"
#include "watch_utility.h"

#ifdef HAS_ACCELEROMETER

// RAM to stash the data points.
movement_activity_data_point movement_activity_log[MOVEMENT_NUM_DATA_POINTS] = {0};
// the absolute number of data points logged
uint32_t data_points = 0;

// variables to stash the voltage and low energy stats at midnight, to be logged at noon the next day.
static uint16_t midnight_voltage = 0;
static uint8_t midnight_le_ratio = 0;

// hacky: we're just tapping into Movement's global state for activity detection.
// do we need better API for this? i'm less bothered now that it's all in Movement.
extern uint8_t active_minutes;
extern uint16_t low_energy_minutes;

typedef union {
    struct {
        uint64_t magic1 : 8;
        uint64_t magic2 : 8;
        uint64_t magic3 : 8;
        uint64_t magic4 : 8;
        uint64_t version : 8;
        uint64_t flags : 8;
        uint64_t year : 7;
        uint64_t month : 4;
        uint64_t day : 5;
    } bit;
    uint64_t reg;
} _movement_activity_header_t;

/// @brief Internal function, writes a new data point to the file system.
void _movement_store_daily_info(void);
void _movement_store_daily_info(void) {
    // Don't attempt to log data until 36 hours have passed.
    if (data_points < MOVEMENT_NUM_DATA_POINTS) return;

    // if the log does not exist, create it and add a header.
    if (!filesystem_file_exists("movement.log")) {
        // fetch the local date/time
        watch_date_time_t datetime = movement_get_local_date_time();
        uint32_t epoch_time = watch_utility_date_time_to_unix_time(datetime, 0);
        // but rewind by a day since the data point represents yesterday's activity
        datetime = watch_utility_date_time_from_unix_time(epoch_time - 86400, 0);

        // struct with header data, accessible as a union of a struct and a uint32
        _movement_activity_header_t header = {
            .bit = {
                .magic1 = 'M',
                .magic2 = 'V',
                .magic3 = 'M',
                .magic4 = 'T',
                .version = 0,
                .flags = 0,
                .year = datetime.unit.year + RTC_REFERENCE_YEAR - 2000,
                .month = datetime.unit.month,
                .day = datetime.unit.day
           }
        };

        filesystem_write_file("movement.log", (char *)&header, sizeof(header));
    }

    // we have 36 hours of data. let's extract some features from it!
    // we should be logging data at noon local time, which should place the head of the 36 hour log at the interval ending 12:05 AM the previous day
    int8_t sleep_hour = -1;
    int8_t sleep_minute = -1;
    int8_t wake_hour = -1;
    int8_t wake_minute = -1;
    int16_t sleep_duration = -1;
    int16_t maximum_temperature = -1;
    int16_t worn_minutes = -1;
    int16_t active_minutes = -1;

    bool is_likely_worn = false;

    bool previous_interval_was_exercise = false;

    int candidate_sleep_index = -1;
    int candidate_wake_index = -1;
    int intervals_not_matching_expectation = 0;

    for (int i = 0; i < MOVEMENT_NUM_DATA_POINTS; ++i) {
        movement_activity_data_point data = movement_activity_log[i];

        // we are likely on-wrist if any movement was detected, or if the temperature is above 28 degrees Celsius (82.4°F)
        is_likely_worn = data.bit.orientation_changes || data.bit.active_minutes || (data.bit.measured_temperature > (28 + 30));

        // this data point represents yesterday's daytime stats
        if (i < 288) {
            // total up yesterday's exercise minutes. You get credit for exercise if:
            //  - an interval had at least three active minutes, OR
            //  - an interval had 1-2 active minutes, and the next interval will have at least three active minutes, OR
            //  - an interval had 1-2 active minutes, and the previous interval qualified under one of the previous two rules.
            switch (data.bit.active_minutes) {
                case 0:
                {
                    // if this interval has zero active minutes, we can feel certain that the wearer is not exercising in this interval.
                    previous_interval_was_exercise = false;
                    // there's also no active minutes to add.
                }
                break;
                case 1:
                case 2:
                {
                    // if we have one or two active minutes, that could be the beginning / end of an exercise session, or it could be random (we caught their arm in motion).
                    // we will count it as exercise if:
                    //  - the previous interval was part of an exercise session, or
                    //  - the next interval will be detected as exercise
                    if (previous_interval_was_exercise || movement_activity_log[i + 1].bit.active_minutes > 2) {
                        // It's a hit! These active minutes represent exercise.
                        active_minutes += data.bit.active_minutes;
                        // This interval will become the previous interval on next iteration; flag it as such.
                        previous_interval_was_exercise = true;
                    } else {
                        // this is not a workout: either the previous interval wasn't part of an exercise session, or the next one won't qualify.
                        // mark this interval as such, and do not add the active minutes to the total.
                        previous_interval_was_exercise = false;
                    }
                }
                break;
                case 3:
                case 4:
                case 5:
                {
                    // if we have three or more active minutes, we are going to assume it's not random and there's some exercise going on.
                    active_minutes += data.bit.active_minutes;
                    previous_interval_was_exercise = true;
                }
                break;
            }

            // if temperature indicates watch being worn, add 5 worn minutes to total
            if (is_likely_worn) worn_minutes += 5;

            // if temperature is above previous maximum temperature, update maximum
            if (data.bit.measured_temperature > maximum_temperature) maximum_temperature = data.bit.measured_temperature;
        }
        if (i > 216) {
            // after 6:00 PM, start looking for sleep time
            if (!is_likely_worn) continue;

            if (candidate_sleep_index == -1) {
                // we don't yet have a likely bedtime
                if (data.bit.orientation_changes < 10) {
                    // but if the orientation changes fell below 10, we may be asleep
                    candidate_sleep_index = i;
                }
            } else if (sleep_hour == -1) {
                // we have a candidate bedtime, but haven't determined time of sleep. look at the next 30 minutes to see if we can establish it.
                if (data.bit.orientation_changes >= 10) intervals_not_matching_expectation++;

                // regardless of whether we got to 30 minutes, if the wearer was moving in two intervals after the candidate sleep time, they didn't go to sleep.
                if (intervals_not_matching_expectation > 2) {
                    candidate_sleep_index = -1;
                    intervals_not_matching_expectation = 0;
                    continue;
                }

                // if we made it six entries past the candidate sleep time without getting kicked out by an expectation mismatch, we probably fell asleep.
                if (i == candidate_sleep_index + 5) {
                    sleep_hour = (candidate_sleep_index + 1) / 12;
                    sleep_minute = ((candidate_sleep_index + 1) % 12);
                    sleep_duration = (6 - intervals_not_matching_expectation) * 5;
                    // expectations are changing: in the next stanza, we're expecting the wearer to wake up.
                    intervals_not_matching_expectation = 0;
                }
            }
            if (candidate_sleep_index != -1 && candidate_wake_index == -1) {
                // we are asleep, and we don't yet have a likely wake time.
                if (data.bit.orientation_changes < 10) {
                    sleep_duration += 5;
                } else {
                    // but if the orientation changes went above 10, we might!
                    candidate_wake_index = i;
                }
            } else if (wake_hour == -1) {
                // we have a candidate wake time, but haven't determined if the wearer is awake. look at the next 30 minutes to see if we can establish that.
                if (data.bit.orientation_changes < 10) intervals_not_matching_expectation++;

                // regardless of whether we got to 30 minutes, if the wearer was dead to the world in two intervals after the candidate wake time, they fell back asleep.
                if (intervals_not_matching_expectation > 2) {
                    candidate_wake_index = -1;
                    intervals_not_matching_expectation = 0;
                    continue;
                }

                // if we made it six entries past the candidate wake time without getting kicked out by an expectation mismatch, we probably woke up.
                if (i == candidate_wake_index + 5) {
                    wake_hour = (candidate_wake_index + 1) / 12;
                    wake_minute = ((candidate_wake_index + 1) % 12);
                    break; // nothing more to do here! we'll crunch the rest of today's data tomorrow.
                }
            }
        }
    }

    movement_data_log_entry_t entry = {
        .bit.sleep_time.hour = sleep_hour,
        .bit.sleep_time.minute = sleep_minute,
        .bit.wake_time.hour = wake_hour,
        .bit.wake_time.minute = wake_minute,
        .bit.sleep_duration = sleep_duration,
        .bit.worn_unworn_ratio = worn_minutes / 96,
        .bit.maximum_temperature = maximum_temperature,
        .bit.active_minutes = active_minutes,
        .bit.battery_voltage = midnight_voltage,
        .bit.le_wake_ratio = midnight_le_ratio
    };

    filesystem_append_file("movement.log", (char*)&entry, sizeof(entry));
}

void _movement_log_data(void) {
    size_t pos = data_points % MOVEMENT_NUM_DATA_POINTS;
    movement_activity_data_point data_point = {0};

    // Movement tracks active minutes when deciding whether to sleep.
    data_point.bit.active_minutes = active_minutes;

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

    // midnight stuff:
    watch_date_time_t datetime = movement_get_local_date_time();
    if (datetime.unit.hour == 0 && datetime.unit.minute == 0) {
        watch_enable_adc();
        midnight_voltage = watch_get_vcc_voltage();
        watch_disable_adc();
        if (low_energy_minutes > 1440) low_energy_minutes = 1440;
        midnight_le_ratio = low_energy_minutes / 96;
        low_energy_minutes = 0;
    }

    // noon stuff:
    if (datetime.unit.hour == 12 && datetime.unit.minute == 0) {
        _movement_store_daily_info();
    }
}

movement_activity_data_point *movement_get_data_log(uint32_t *count) {
    *count = data_points;
    return movement_activity_log;
}

#endif
