/*
 * MIT License
 *
 * Copyright (c) 2022 Joey Castillo
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

#define MOVEMENT_LONG_PRESS_TICKS 64

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "app.h"
#include "watch.h"
#include "watch_utility.h"
#include "usb.h"
#include "watch_private.h"
#include "movement.h"
#include "filesystem.h"
#include "shell.h"
#include "utz.h"
#include "zones.h"
#include "tc.h"
#include "evsys.h"
#include "delay.h"
#include "thermistor_driver.h"

#include "movement_config.h"

#include "movement_custom_signal_tunes.h"

#if __EMSCRIPTEN__
#include <emscripten.h>
#else
#include "watch_usb_cdc.h"
#endif

movement_state_t movement_state;
void * watch_face_contexts[MOVEMENT_NUM_FACES];
watch_date_time_t scheduled_tasks[MOVEMENT_NUM_FACES];
const int32_t movement_le_inactivity_deadlines[8] = {INT_MAX, 600, 3600, 7200, 21600, 43200, 86400, 604800};
const int16_t movement_timeout_inactivity_deadlines[4] = {60, 120, 300, 1800};
movement_event_t event;

int8_t _movement_dst_offset_cache[NUM_ZONE_NAMES] = {0};
#define TIMEZONE_DOES_NOT_OBSERVE (-127)

void cb_mode_btn_interrupt(void);
void cb_light_btn_interrupt(void);
void cb_alarm_btn_interrupt(void);
void cb_alarm_btn_extwake(void);
void cb_alarm_fired(void);
void cb_fast_tick(void);
void cb_tick(void);

void cb_accelerometer_event(void);
void cb_accelerometer_wake(void);

#if __EMSCRIPTEN__
void yield(void) {
}
#else
void yield(void) {
    tud_task();
    cdc_task();
}
#endif

static udatetime_t _movement_convert_date_time_to_udate(watch_date_time_t date_time) {
    return (udatetime_t) {
        .date.dayofmonth = date_time.unit.day,
        .date.dayofweek = dayofweek(UYEAR_FROM_YEAR(date_time.unit.year + WATCH_RTC_REFERENCE_YEAR), date_time.unit.month, date_time.unit.day),
        .date.month = date_time.unit.month,
        .date.year = UYEAR_FROM_YEAR(date_time.unit.year + WATCH_RTC_REFERENCE_YEAR),
        .time.hour = date_time.unit.hour,
        .time.minute = date_time.unit.minute,
        .time.second = date_time.unit.second
    };
}

static bool _movement_update_dst_offset_cache(void) {
    uzone_t local_zone;
    udatetime_t udate_time;
    bool dst_changed = false;
    watch_date_time_t system_date_time = watch_rtc_get_date_time();

    for (uint8_t i = 0; i < NUM_ZONE_NAMES; i++) {
        unpack_zone(&zone_defns[i], "", &local_zone);
        watch_date_time_t date_time = watch_utility_date_time_convert_zone(system_date_time, 0, local_zone.offset.hours * 3600 + local_zone.offset.minutes * 60);

        if (!!local_zone.rules_len) {
            // if local zone has DST rules, we need to see if DST applies.
            udate_time = _movement_convert_date_time_to_udate(date_time);
            uoffset_t offset;
            get_current_offset(&local_zone, &udate_time, &offset);
            int8_t new_offset = (offset.hours * 60 + offset.minutes) / 15;
            if (_movement_dst_offset_cache[i] != new_offset) {
                _movement_dst_offset_cache[i] = new_offset;
                dst_changed = true;
            }
        } else {
            // otherwise set the cache to a constant value that indicates no DST check needs to be performed.
            _movement_dst_offset_cache[i] = TIMEZONE_DOES_NOT_OBSERVE;
        }
    }

    return dst_changed;
}

static inline void _movement_reset_inactivity_countdown(void) {
    movement_state.le_mode_ticks = movement_le_inactivity_deadlines[movement_state.settings.bit.le_interval];
    movement_state.timeout_ticks = movement_timeout_inactivity_deadlines[movement_state.settings.bit.to_interval];
}

static inline void _movement_enable_fast_tick_if_needed(void) {
    if (!movement_state.fast_tick_enabled) {
        movement_state.fast_ticks = 0;
        watch_rtc_register_periodic_callback(cb_fast_tick, 128);
        movement_state.fast_tick_enabled = true;
    }
}

static inline void _movement_disable_fast_tick_if_possible(void) {
    if ((movement_state.light_ticks == -1) &&
        (movement_state.alarm_ticks == -1) &&
        ((movement_state.light_down_timestamp + movement_state.mode_down_timestamp + movement_state.alarm_down_timestamp) == 0)) {
        movement_state.fast_tick_enabled = false;
        watch_rtc_disable_periodic_callback(128);
    }
}

static void _movement_handle_top_of_minute(void) {
    watch_date_time_t date_time = watch_rtc_get_date_time();

    // update the DST offset cache every 30 minutes, since someplace in the world could change.
    if (date_time.unit.minute % 30 == 0) {
        _movement_update_dst_offset_cache();
    }

    for(uint8_t i = 0; i < MOVEMENT_NUM_FACES; i++) {
        // For each face that offers an advisory...
        if (watch_faces[i].advise != NULL) {
            // ...we ask for one.
            movement_watch_face_advisory_t advisory = watch_faces[i].advise(watch_face_contexts[i]);

            // If it wants a background task...
            if (advisory.wants_background_task) {
                // we give it one. pretty straightforward!
                movement_event_t background_event = { EVENT_BACKGROUND_TASK, 0 };
                watch_faces[i].loop(background_event, watch_face_contexts[i]);
            }

            // TODO: handle other advisory types
        }
    }
    movement_state.woke_from_alarm_handler = false;
}

static void _movement_handle_scheduled_tasks(void) {
    watch_date_time_t date_time = watch_rtc_get_date_time();
    uint8_t num_active_tasks = 0;

    for(uint8_t i = 0; i < MOVEMENT_NUM_FACES; i++) {
        if (scheduled_tasks[i].reg) {
            if (scheduled_tasks[i].reg <= date_time.reg) {
                scheduled_tasks[i].reg = 0;
                movement_event_t background_event = { EVENT_BACKGROUND_TASK, 0 };
                watch_faces[i].loop(background_event, watch_face_contexts[i]);
                // check if loop scheduled a new task
                if (scheduled_tasks[i].reg) {
                    num_active_tasks++;
                }
            } else {
                num_active_tasks++;
            }
        }
    }

    if (num_active_tasks == 0) {
        movement_state.has_scheduled_background_task = false;
    } else {
        _movement_reset_inactivity_countdown();
    }
}

void movement_request_tick_frequency(uint8_t freq) {
    // Movement uses the 128 Hz tick internally
    if (freq == 128) return;

    // Movement requires at least a 1 Hz tick.
    // If we are asked for an invalid frequency, default back to 1 Hz.
    if (freq == 0 || __builtin_popcount(freq) != 1) freq = 1;

    // disable all callbacks except the 128 Hz one
    watch_rtc_disable_matching_periodic_callbacks(0xFE);

    movement_state.subsecond = 0;
    movement_state.tick_frequency = freq;
    watch_rtc_register_periodic_callback(cb_tick, freq);
}

void movement_illuminate_led(void) {
    if (movement_state.settings.bit.led_duration != 0b111) {
        watch_set_led_color_rgb(movement_state.settings.bit.led_red_color | movement_state.settings.bit.led_red_color << 4,
                                movement_state.settings.bit.led_green_color | movement_state.settings.bit.led_green_color << 4,
                                movement_state.settings.bit.led_blue_color | movement_state.settings.bit.led_blue_color << 4);
        if (movement_state.settings.bit.led_duration == 0) {
            movement_state.light_ticks = 1;
        } else {
            movement_state.light_ticks = (movement_state.settings.bit.led_duration * 2 - 1) * 128;
        }
        _movement_enable_fast_tick_if_needed();
    }
}

void movement_force_led_on(uint8_t red, uint8_t green, uint8_t blue) {
    // this is hacky, we need a way for watch faces to set an arbitrary color and prevent Movement from turning it right back off.
    watch_set_led_color_rgb(red, green, blue);
    movement_state.light_ticks = 32767;
}

void movement_force_led_off(void) {
    watch_set_led_off();
    movement_state.light_ticks = -1;
    _movement_disable_fast_tick_if_possible();
}

bool movement_default_loop_handler(movement_event_t event) {
    switch (event.event_type) {
        case EVENT_MODE_BUTTON_UP:
            movement_move_to_next_face();
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            movement_illuminate_led();
            break;
        case EVENT_LIGHT_BUTTON_UP:
            if (movement_state.settings.bit.led_duration == 0) {
                movement_force_led_off();
            }
            break;
        case EVENT_MODE_LONG_PRESS:
            if (MOVEMENT_SECONDARY_FACE_INDEX && movement_state.current_face_idx == 0) {
                movement_move_to_face(MOVEMENT_SECONDARY_FACE_INDEX);
            } else {
                movement_move_to_face(0);
            }
            break;
        default:
            break;
    }

    return true;
}

void movement_move_to_face(uint8_t watch_face_index) {
    movement_state.watch_face_changed = true;
    movement_state.next_face_idx = watch_face_index;
}

void movement_move_to_next_face(void) {
    uint16_t face_max;
    if (MOVEMENT_SECONDARY_FACE_INDEX) {
        face_max = (movement_state.current_face_idx < (int16_t)MOVEMENT_SECONDARY_FACE_INDEX) ? MOVEMENT_SECONDARY_FACE_INDEX : MOVEMENT_NUM_FACES;
    } else {
        face_max = MOVEMENT_NUM_FACES;
    }
    movement_move_to_face((movement_state.current_face_idx + 1) % face_max);
}

void movement_schedule_background_task(watch_date_time_t date_time) {
    movement_schedule_background_task_for_face(movement_state.current_face_idx, date_time);
}

void movement_cancel_background_task(void) {
    movement_cancel_background_task_for_face(movement_state.current_face_idx);
}

void movement_schedule_background_task_for_face(uint8_t watch_face_index, watch_date_time_t date_time) {
    watch_date_time_t now = watch_rtc_get_date_time();
    if (date_time.reg > now.reg) {
        movement_state.has_scheduled_background_task = true;
        scheduled_tasks[watch_face_index].reg = date_time.reg;
    }
}

void movement_cancel_background_task_for_face(uint8_t watch_face_index) {
    scheduled_tasks[watch_face_index].reg = 0;
    bool other_tasks_scheduled = false;
    for(uint8_t i = 0; i < MOVEMENT_NUM_FACES; i++) {
        if (scheduled_tasks[i].reg != 0) {
            other_tasks_scheduled = true;
            break;
        }
    }
    movement_state.has_scheduled_background_task = other_tasks_scheduled;
}

void movement_request_sleep(void) {
    /// FIXME: for #SecondMovement: This was a feature request to allow watch faces to request sleep.
    /// Setting the ticks to 1 means the watch will sleep after the next tick. I'd like to say let's
    /// set it to 0, have the watch face loop return false, and then we'll fall asleep immediately.
    /// But could this lead to a race condition where the callback decrements to -1 before the loop?
    /// This is the safest way but consider more testing here.
    movement_state.le_mode_ticks = 1;
}

void movement_request_wake() {
    movement_state.needs_wake = true;
    _movement_reset_inactivity_countdown();
}

static void end_buzzing() {
    movement_state.is_buzzing = false;
}

static void end_buzzing_and_disable_buzzer(void) {
    end_buzzing();
    watch_disable_buzzer();
}

void movement_play_signal(void) {
    void *maybe_disable_buzzer = end_buzzing_and_disable_buzzer;
    if (watch_is_buzzer_or_led_enabled()) {
        maybe_disable_buzzer = end_buzzing;
    } else {
        watch_enable_buzzer();
    }
    movement_state.is_buzzing = true;
    watch_buzzer_play_sequence(signal_tune, maybe_disable_buzzer);
    if (movement_state.le_mode_ticks == -1) {
        // the watch is asleep. wake it up for "1" round through the main loop.
        // the sleep_mode_app_loop will notice the is_buzzing and note that it
        // only woke up to beep and then it will spinlock until the callback
        // turns off the is_buzzing flag.
        movement_state.needs_wake = true;
        movement_state.le_mode_ticks = 1;
    }
}

void movement_play_alarm(void) {
    movement_play_alarm_beeps(5, BUZZER_NOTE_C8);
}

void movement_play_alarm_beeps(uint8_t rounds, watch_buzzer_note_t alarm_note) {
    if (rounds == 0) rounds = 1;
    if (rounds > 20) rounds = 20;
    movement_request_wake();
    movement_state.alarm_note = alarm_note;
    // our tone is 0.375 seconds of beep and 0.625 of silence, repeated as given.
    movement_state.alarm_ticks = 128 * rounds - 75;
    _movement_enable_fast_tick_if_needed();
}

uint8_t movement_claim_backup_register(void) {
    if (movement_state.next_available_backup_register >= 8) return 0;
    return movement_state.next_available_backup_register++;
}

int32_t movement_get_current_timezone_offset_for_zone(uint8_t zone_index) {
    int8_t cached_dst_offset = _movement_dst_offset_cache[zone_index];

    if (cached_dst_offset == TIMEZONE_DOES_NOT_OBSERVE) {
        // if time zone doesn't observe DST, we can just return the standard time offset from the zone definition.
        return (int32_t)zone_defns[zone_index].offset_inc_minutes * OFFSET_INCREMENT * 60;
    } else {
        // otherwise, we've precalculated the offset for this zone and can return it.
        return (int32_t)cached_dst_offset * OFFSET_INCREMENT * 60;
    }
}

int32_t movement_get_current_timezone_offset(void) {
    return movement_get_current_timezone_offset_for_zone(movement_state.settings.bit.time_zone);
}

int32_t movement_get_timezone_index(void) {
    return movement_state.settings.bit.time_zone;
}

void movement_set_timezone_index(uint8_t value) {
    movement_state.settings.bit.time_zone = value;
}

watch_date_time_t movement_get_utc_date_time(void) {
    return watch_rtc_get_date_time();
}

watch_date_time_t movement_get_date_time_in_zone(uint8_t zone_index) {
    int32_t offset = movement_get_current_timezone_offset_for_zone(zone_index);
    return watch_utility_date_time_convert_zone(watch_rtc_get_date_time(), 0, offset);
}

watch_date_time_t movement_get_local_date_time(void) {
    watch_date_time_t date_time = watch_rtc_get_date_time();
    return watch_utility_date_time_convert_zone(date_time, 0, movement_get_current_timezone_offset());
}

void movement_set_local_date_time(watch_date_time_t date_time) {
    int32_t current_offset = movement_get_current_timezone_offset();
    watch_date_time_t utc_date_time = watch_utility_date_time_convert_zone(date_time, current_offset, 0);
    watch_rtc_set_date_time(utc_date_time);

    // this may seem wasteful, but if the user's local time is in a zone that observes DST,
    // they may have just crossed a DST boundary, which means the next call to this function
    // could require a different offset to force local time back to UTC. Quelle horreur!
    _movement_update_dst_offset_cache();
}

bool movement_button_should_sound(void) {
    return movement_state.settings.bit.button_should_sound;
}

void movement_set_button_should_sound(bool value) {
    movement_state.settings.bit.button_should_sound = value;
}

watch_buzzer_volume_t movement_button_volume(void) {
    return movement_state.settings.bit.button_volume;
}

void movement_set_button_volume(watch_buzzer_volume_t value) {
    movement_state.settings.bit.button_volume = value;
}

movement_clock_mode_t movement_clock_mode_24h(void) {
    return movement_state.settings.bit.clock_mode_24h ? MOVEMENT_CLOCK_MODE_24H : MOVEMENT_CLOCK_MODE_12H;
}

void movement_set_clock_mode_24h(movement_clock_mode_t value) {
    movement_state.settings.bit.clock_mode_24h = (value == MOVEMENT_CLOCK_MODE_24H);
}

bool movement_use_imperial_units(void) {
    return movement_state.settings.bit.use_imperial_units;
}

void movement_set_use_imperial_units(bool value) {
    movement_state.settings.bit.use_imperial_units = value;
}

uint8_t movement_get_fast_tick_timeout(void) {
    return movement_state.settings.bit.to_interval;
}

void movement_set_fast_tick_timeout(uint8_t value) {
    movement_state.settings.bit.to_interval = value;
}

uint8_t movement_get_low_energy_timeout(void) {
    return movement_state.settings.bit.le_interval;
}

void movement_set_low_energy_timeout(uint8_t value) {
    movement_state.settings.bit.le_interval = value;
}

movement_color_t movement_backlight_color(void) {
    return (movement_color_t) {
        .red = movement_state.settings.bit.led_red_color,
        .green = movement_state.settings.bit.led_green_color,
        .blue = movement_state.settings.bit.led_blue_color
    };
}

void movement_set_backlight_color(movement_color_t color) {
    movement_state.settings.bit.led_red_color = color.red;
    movement_state.settings.bit.led_green_color = color.green;
    movement_state.settings.bit.led_blue_color = color.blue;
}

uint8_t movement_get_backlight_dwell(void) {
    return movement_state.settings.bit.led_duration;
}

void movement_set_backlight_dwell(uint8_t value) {
    movement_state.settings.bit.led_duration = value;
}

void movement_store_settings(void) {
    movement_settings_t old_settings;
    filesystem_read_file("settings.u32", (char *)&old_settings, sizeof(movement_settings_t));
    if (movement_state.settings.reg != old_settings.reg) {
        filesystem_write_file("settings.u32", (char *)&movement_state.settings, sizeof(movement_settings_t));
    }
}

bool movement_alarm_enabled(void) {
    return movement_state.alarm_enabled;
}

void movement_set_alarm_enabled(bool value) {
    movement_state.alarm_enabled = value;
}

bool movement_enable_tap_detection_if_available(void) {
    if (movement_state.has_lis2dw) {
        // configure tap duration threshold and enable Z axis
        lis2dw_configure_tap_threshold(0, 0, 12, LIS2DW_REG_TAP_THS_Z_Z_AXIS_ENABLE);
        lis2dw_configure_tap_duration(10, 2, 2);

        // ramp data rate up to 400 Hz and high performance mode
        lis2dw_set_low_noise_mode(true);
        lis2dw_set_data_rate(LIS2DW_DATA_RATE_HP_400_HZ);
        lis2dw_set_mode(LIS2DW_MODE_HIGH_PERFORMANCE);

        // Settling time (1 sample duration, i.e. 1/400Hz)
        delay_ms(3);

        // enable tap detection on INT1/A3.
        lis2dw_configure_int1(LIS2DW_CTRL4_INT1_SINGLE_TAP | LIS2DW_CTRL4_INT1_6D);

        return true;
    }

    return false;
}

bool movement_disable_tap_detection_if_available(void) {
    if (movement_state.has_lis2dw) {
        // Ramp data rate back down to the usual lowest rate to save power.
        lis2dw_set_low_noise_mode(false);
        lis2dw_set_data_rate(movement_state.accelerometer_background_rate);
        lis2dw_set_mode(LIS2DW_MODE_LOW_POWER);
        // ...disable Z axis (not sure if this is needed, does this save power?)...
        lis2dw_configure_tap_threshold(0, 0, 0, 0);

        return true;
    }

    return false;
}

lis2dw_data_rate_t movement_get_accelerometer_background_rate(void) {
    if (movement_state.has_lis2dw) return movement_state.accelerometer_background_rate;
    else return LIS2DW_DATA_RATE_POWERDOWN;
}

bool movement_set_accelerometer_background_rate(lis2dw_data_rate_t new_rate) {
    if (movement_state.has_lis2dw) {
        if (movement_state.accelerometer_background_rate != new_rate) {
            lis2dw_set_data_rate(new_rate);
            movement_state.accelerometer_background_rate = new_rate;

            return true;
        }
    }

    return false;
}

uint8_t movement_get_accelerometer_motion_threshold(void) {
    if (movement_state.has_lis2dw) return movement_state.accelerometer_motion_threshold;
    else return 0;
}

bool movement_set_accelerometer_motion_threshold(uint8_t new_threshold) {
    if (movement_state.has_lis2dw) {
        if (movement_state.accelerometer_motion_threshold != new_threshold) {
            lis2dw_configure_wakeup_threshold(new_threshold);
            movement_state.accelerometer_motion_threshold = new_threshold;

            return true;
        }
    }

    return false;
}

float movement_get_temperature(void) {
    float temperature_c = (float)0xFFFFFFFF;

    if (movement_state.has_thermistor) {
        thermistor_driver_enable();
        temperature_c = thermistor_driver_get_temperature();
        thermistor_driver_disable();
    } else if (movement_state.has_lis2dw) {
            int16_t val = lis2dw_get_temperature();
            val = val >> 4;
            temperature_c = 25 + (float)val / 16.0;
    }

    return temperature_c;
}

void app_init(void) {
    _watch_init();

    filesystem_init();

    // check if we are plugged into USB power.
    HAL_GPIO_VBUS_DET_in();
    HAL_GPIO_VBUS_DET_pulldown();
    if (HAL_GPIO_VBUS_DET_read()){
        /// if so, enable USB functionality.
        _watch_enable_usb();
    }
    HAL_GPIO_VBUS_DET_off();

    memset(&movement_state, 0, sizeof(movement_state));

    movement_state.has_thermistor = thermistor_driver_init();

    bool settings_file_exists = filesystem_file_exists("settings.u32");
    movement_settings_t maybe_settings;
    if (settings_file_exists && maybe_settings.bit.version == 0) {
        filesystem_read_file("settings.u32", (char *) &maybe_settings, sizeof(movement_settings_t));
    }

    if (settings_file_exists && maybe_settings.bit.version == 0) {
        // If settings file exists and has a valid version, restore it!
        movement_state.settings.reg = maybe_settings.reg;
    } else {
        // Otherwise set default values.
        movement_state.settings.bit.version = 0;
        movement_state.settings.bit.clock_mode_24h = MOVEMENT_DEFAULT_24H_MODE;
        movement_state.settings.bit.time_zone = UTZ_UTC;
        movement_state.settings.bit.led_red_color = MOVEMENT_DEFAULT_RED_COLOR;
        movement_state.settings.bit.led_green_color = MOVEMENT_DEFAULT_GREEN_COLOR;
    #if defined(WATCH_BLUE_TCC_CHANNEL) && !defined(WATCH_GREEN_TCC_CHANNEL)
        // If there is a blue LED but no green LED, this is a blue Special Edition board.
        // In the past, the "green color" showed up as the blue color on the blue board.
        if (MOVEMENT_DEFAULT_RED_COLOR == 0 && MOVEMENT_DEFAULT_BLUE_COLOR == 0) {
            // If the red color is 0 and the blue color is 0, we'll fall back to the old
            // behavior, since otherwise there would be no default LED color.
            movement_state.settings.bit.led_blue_color = MOVEMENT_DEFAULT_GREEN_COLOR;
        } else {
            // however if either the red or blue color is nonzero, we'll assume the user
            // has used the new defaults and knows what color they want. this could be red
            // if blue is 0, or a custom color if both are nonzero.
            movement_state.settings.bit.led_blue_color = MOVEMENT_DEFAULT_BLUE_COLOR;
        }
    #else
        movement_state.settings.bit.led_blue_color = MOVEMENT_DEFAULT_BLUE_COLOR;
    #endif
        movement_state.settings.bit.button_should_sound = MOVEMENT_DEFAULT_BUTTON_SOUND;
        movement_state.settings.bit.button_volume = MOVEMENT_DEFAULT_BUTTON_VOLUME;
        movement_state.settings.bit.to_interval = MOVEMENT_DEFAULT_TIMEOUT_INTERVAL;
#ifdef MOVEMENT_LOW_ENERGY_MODE_FORBIDDEN
        movement_state.settings.bit.le_interval = 0;
#else
        movement_state.settings.bit.le_interval = MOVEMENT_DEFAULT_LOW_ENERGY_INTERVAL;
#endif
        movement_state.settings.bit.led_duration = MOVEMENT_DEFAULT_LED_DURATION;

        movement_store_settings();
    }

    watch_date_time_t date_time = watch_rtc_get_date_time();
    if (date_time.reg == 0) {
        date_time = watch_get_init_date_time();
        // but convert from local time to UTC
        date_time = watch_utility_date_time_convert_zone(date_time, movement_get_current_timezone_offset(), 0);
        watch_rtc_set_date_time(date_time);
    }

    // populate the DST offset cache
    _movement_update_dst_offset_cache();

    if (movement_state.accelerometer_motion_threshold == 0) movement_state.accelerometer_motion_threshold = 32;

    movement_state.light_ticks = -1;
    movement_state.alarm_ticks = -1;
    movement_state.next_available_backup_register = 2;
    _movement_reset_inactivity_countdown();
}

void app_wake_from_backup(void) {
}

void app_setup(void) {
    watch_store_backup_data(movement_state.settings.reg, 0);

    static bool is_first_launch = true;

    if (is_first_launch) {
        #ifdef MOVEMENT_CUSTOM_BOOT_COMMANDS
        MOVEMENT_CUSTOM_BOOT_COMMANDS()
        #endif

        for(uint8_t i = 0; i < MOVEMENT_NUM_FACES; i++) {
            watch_face_contexts[i] = NULL;
            scheduled_tasks[i].reg = 0;
            is_first_launch = false;
        }

#if __EMSCRIPTEN__
        int32_t time_zone_offset = EM_ASM_INT({
            return -new Date().getTimezoneOffset();
        });
        for (int i = 0; i < NUM_ZONE_NAMES; i++) {
            if (movement_get_current_timezone_offset_for_zone(i) == time_zone_offset * 60) {
                movement_state.settings.bit.time_zone = i;
                break;
            }
        }
#endif

        // set up the 1 minute alarm (for background tasks and low power updates)
        watch_date_time_t alarm_time;
        alarm_time.reg = 0;
        watch_rtc_register_alarm_callback(cb_alarm_fired, alarm_time, ALARM_MATCH_SS);
    }

    // LCD autodetect uses the buttons as a a failsafe, so we should run it before we enable the button interrupts
    watch_enable_display();

    if (movement_state.le_mode_ticks != -1) {
        watch_disable_extwake_interrupt(HAL_GPIO_BTN_ALARM_pin());

        watch_enable_external_interrupts();
        watch_register_interrupt_callback(HAL_GPIO_BTN_MODE_pin(), cb_mode_btn_interrupt, INTERRUPT_TRIGGER_BOTH);
        watch_register_interrupt_callback(HAL_GPIO_BTN_LIGHT_pin(), cb_light_btn_interrupt, INTERRUPT_TRIGGER_BOTH);
        watch_register_interrupt_callback(HAL_GPIO_BTN_ALARM_pin(), cb_alarm_btn_interrupt, INTERRUPT_TRIGGER_BOTH);

#ifdef I2C_SERCOM
        static bool lis2dw_checked = false;
        if (!lis2dw_checked) {
            watch_enable_i2c();
            if (lis2dw_begin()) {
                movement_state.has_lis2dw = true;
            } else {
                movement_state.has_lis2dw = false;
                watch_disable_i2c();
            }
            lis2dw_checked = true;
        } else if (movement_state.has_lis2dw) {
            watch_enable_i2c();
            lis2dw_begin();
        }

        if (movement_state.has_lis2dw) {
            lis2dw_set_mode(LIS2DW_MODE_LOW_POWER);         // select low power (not high performance) mode
            lis2dw_set_low_power_mode(LIS2DW_LP_MODE_1);    // lowest power mode, 12-bit
            lis2dw_set_low_noise_mode(false);               // low noise mode raises power consumption slightly; we don't need it
            lis2dw_enable_stationary_motion_detection();    // stationary/motion detection mode keeps the data rate at 1.6 Hz even in sleep
            lis2dw_set_range(LIS2DW_RANGE_2_G);             // Application note AN5038 recommends 2g range
            lis2dw_enable_sleep();                          // allow acceleromter to sleep and wake on activity
            lis2dw_configure_wakeup_threshold(movement_state.accelerometer_motion_threshold); // g threshold to wake up: (THS * FS / 64) where FS is "full scale" of ±2g.
            lis2dw_configure_6d_threshold(3);               // 0-3 is 80, 70, 60, or 50 degrees. 50 is least precise, hopefully most sensitive?

            // set up interrupts:
            // INT1 is wired to pin A3. We'll configure the accelerometer to output an interrupt on INT1 when it detects an orientation change.
            /// TODO: We had routed this interrupt to TC2 to count orientation changes, but TC2 consumed too much power.
            /// Orientation changes helped with sleep tracking; would love to bring this back if we can find a low power solution.
            /// For now, commenting these lines out; check commit 27f0c629d865f4bc56bc6e678da1eb8f4b919093 for power-hungry but working code.
            // lis2dw_configure_int1(LIS2DW_CTRL4_INT1_6D);
            // HAL_GPIO_A3_in();

            // next: INT2 is wired to pin A4. We'll configure the accelerometer to output the sleep state on INT2.
            // a falling edge on INT2 indicates the accelerometer has woken up.
            lis2dw_configure_int2(LIS2DW_CTRL5_INT2_SLEEP_STATE | LIS2DW_CTRL5_INT2_SLEEP_CHG);
            HAL_GPIO_A4_in();

            // Wake on motion seemed like a good idea when the threshold was lower, but the UX makes less sense now.
            // Still if you want to wake on motion, you can do it by uncommenting this line:
            // watch_register_extwake_callback(HAL_GPIO_A4_pin(), cb_accelerometer_wake, false);

            // later on, we are going to use INT1 for tap detection. We'll set up that interrupt here,
            // but it will only fire once tap recognition is enabled.
            watch_register_interrupt_callback(HAL_GPIO_A3_pin(), cb_accelerometer_event, INTERRUPT_TRIGGER_RISING);

            // Enable the interrupts...
            lis2dw_enable_interrupts();

            // At first boot, this next line sets the accelerometer's sampling rate to 0, which is LIS2DW_DATA_RATE_POWERDOWN.
            // This means the interrupts we just configured won't fire.
            // Tap detection will ramp up sesing and make use of the A3 interrupt.
            // If a watch face wants to check in on the A4 interrupt pin for motion status, it can call
            // movement_set_accelerometer_background_rate with another rate like LIS2DW_DATA_RATE_LOWEST or LIS2DW_DATA_RATE_25_HZ.
            lis2dw_set_data_rate(movement_state.accelerometer_background_rate);
        }
#endif

        watch_enable_buzzer();
        watch_enable_leds();

        movement_request_tick_frequency(1);

        for(uint8_t i = 0; i < MOVEMENT_NUM_FACES; i++) {
            watch_faces[i].setup(i, &watch_face_contexts[i]);
        }

        watch_faces[movement_state.current_face_idx].activate(watch_face_contexts[movement_state.current_face_idx]);
        event.subsecond = 0;
        event.event_type = EVENT_ACTIVATE;
    }
}

#ifndef MOVEMENT_LOW_ENERGY_MODE_FORBIDDEN

static void _sleep_mode_app_loop(void) {
    movement_state.needs_wake = false;
    // as long as le_mode_ticks is -1 (i.e. we are in low energy mode), we wake up here, update the screen, and go right back to sleep.
    while (movement_state.le_mode_ticks == -1) {
        // we also have to handle top-of-the-minute tasks here in the mini-runloop
        if (movement_state.woke_from_alarm_handler) _movement_handle_top_of_minute();

        event.event_type = EVENT_LOW_ENERGY_UPDATE;
        watch_faces[movement_state.current_face_idx].loop(event, watch_face_contexts[movement_state.current_face_idx]);

        // if we need to wake immediately, do it!
        if (movement_state.needs_wake) return;
        // otherwise enter sleep mode, and when the extwake handler is called, it will reset le_mode_ticks and force us out at the next loop.
        else watch_enter_sleep_mode();
    }
}

#endif

bool app_loop(void) {
    const watch_face_t *wf = &watch_faces[movement_state.current_face_idx];
    bool woke_up_for_buzzer = false;

    if (movement_state.watch_face_changed) {
        if (movement_state.settings.bit.button_should_sound) {
            // low note for nonzero case, high note for return to watch_face 0
            watch_buzzer_play_note_with_volume(movement_state.next_face_idx ? BUZZER_NOTE_C7 : BUZZER_NOTE_C8, 50, movement_state.settings.bit.button_volume);
        }
        wf->resign(watch_face_contexts[movement_state.current_face_idx]);
        movement_state.current_face_idx = movement_state.next_face_idx;
        // we have just updated the face idx, so we must recache the watch face pointer.
        wf = &watch_faces[movement_state.current_face_idx];
        watch_clear_display();
        movement_request_tick_frequency(1);
        wf->activate(watch_face_contexts[movement_state.current_face_idx]);
        event.subsecond = 0;
        event.event_type = EVENT_ACTIVATE;
        movement_state.watch_face_changed = false;
    }

    // if the LED should be off, turn it off
    if (movement_state.light_ticks == 0) {
        // unless the user is holding down the LIGHT button, in which case, give them more time.
        if (HAL_GPIO_BTN_LIGHT_read()) {
            movement_state.light_ticks = 1;
        } else {
            movement_force_led_off();
        }
    }

    // handle top-of-minute tasks, if the alarm handler told us we need to
    if (movement_state.woke_from_alarm_handler) _movement_handle_top_of_minute();

    // if we have a scheduled background task, handle that here:
    if (event.event_type == EVENT_TICK && movement_state.has_scheduled_background_task) _movement_handle_scheduled_tasks();

#ifndef MOVEMENT_LOW_ENERGY_MODE_FORBIDDEN
    // if we have timed out of our low energy mode countdown, enter low energy mode.
    if (movement_state.le_mode_ticks == 0) {
        movement_state.le_mode_ticks = -1;
        watch_register_extwake_callback(HAL_GPIO_BTN_ALARM_pin(), cb_alarm_btn_extwake, true);
        event.event_type = EVENT_NONE;
        event.subsecond = 0;

        // _sleep_mode_app_loop takes over at this point and loops until le_mode_ticks is reset by the extwake handler,
        // or wake is requested using the movement_request_wake function.
        _sleep_mode_app_loop();
        // as soon as _sleep_mode_app_loop returns, we prepare to reactivate
        // ourselves, but first, we check to see if we woke up for the buzzer:
        if (movement_state.is_buzzing) {
            woke_up_for_buzzer = true;
        }
        event.event_type = EVENT_ACTIVATE;
        // this is a hack tho: waking from sleep mode, app_setup does get called, but it happens before we have reset our ticks.
        // need to figure out if there's a better heuristic for determining how we woke up.
        app_setup();
    }
#endif

    // default to being allowed to sleep by the face.
    bool can_sleep = true;

    if (event.event_type) {
        event.subsecond = movement_state.subsecond;
        // the first trip through the loop overrides the can_sleep state
        can_sleep = wf->loop(event, watch_face_contexts[movement_state.current_face_idx]);

        // Keep light on if user is still interacting with the watch.
        if (movement_state.light_ticks > 0) {
            switch (event.event_type) {
                case EVENT_LIGHT_BUTTON_DOWN:
                case EVENT_MODE_BUTTON_DOWN:
                case EVENT_ALARM_BUTTON_DOWN:
                    movement_illuminate_led();
            }
        }

        event.event_type = EVENT_NONE;
    }

    // if we have timed out of our timeout countdown, give the app a hint that they can resign.
    if (movement_state.timeout_ticks == 0 && movement_state.current_face_idx != 0) {
        movement_state.timeout_ticks = -1;
        event.event_type = EVENT_TIMEOUT;
        event.subsecond = movement_state.subsecond;
        // if we run through the loop again to time out, we need to reconsider whether or not we can sleep.
        // if the first trip said true, but this trip said false, we need the false to override, thus
        // we will be using boolean AND:
        //
        // first trip  | can sleep | cannot sleep | can sleep    | cannot sleep
        // second trip | can sleep | cannot sleep | cannot sleep | can sleep
        //          && | can sleep | cannot sleep | cannot sleep | cannot sleep
        bool can_sleep2 = wf->loop(event, watch_face_contexts[movement_state.current_face_idx]);
        can_sleep = can_sleep && can_sleep2;
        event.event_type = EVENT_NONE;
    }

    // Now that we've handled all display update tasks, handle the alarm.
    if (movement_state.alarm_ticks >= 0) {
        uint8_t buzzer_phase = (movement_state.alarm_ticks + 80) % 128;
        if(buzzer_phase == 127) {
            // failsafe: buzzer could have been disabled in the meantime
            if (!watch_is_buzzer_or_led_enabled()) watch_enable_buzzer();
            // play 4 beeps plus pause
            for(uint8_t i = 0; i < 4; i++) {
                // TODO: This method of playing the buzzer blocks the UI while it's beeping.
                // It might be better to time it with the fast tick.
                watch_buzzer_play_note(movement_state.alarm_note, (i != 3) ? 50 : 75);
                if (i != 3) watch_buzzer_play_note(BUZZER_NOTE_REST, 50);
            }
        }
        if (movement_state.alarm_ticks == 0) {
            movement_state.alarm_ticks = -1;
            _movement_disable_fast_tick_if_possible();
        }
    }

    // if we are plugged into USB, handle the serial shell
    if (usb_is_enabled()) {
        shell_task();
    }

    event.subsecond = 0;

    // if the watch face changed, we can't sleep because we need to update the display.
    if (movement_state.watch_face_changed) can_sleep = false;

    // if we woke up for the buzzer, stay awake until it's finished.
    if (woke_up_for_buzzer) {
        while(watch_is_buzzer_or_led_enabled());
    }

    // if the LED is on, we need to stay awake to keep the TCC running.
    if (movement_state.light_ticks != -1) can_sleep = false;

    // if we are plugged into USB, we can't sleep because we need to keep the serial shell running.
    if (usb_is_enabled()) {
        yield();
        can_sleep = false;
    }

    return can_sleep;
}

static movement_event_type_t _figure_out_button_event(bool pin_level, movement_event_type_t button_down_event_type, uint16_t *down_timestamp) {
    // force alarm off if the user pressed a button.
    if (movement_state.alarm_ticks) movement_state.alarm_ticks = 0;

    if (pin_level) {
        // handle rising edge
        _movement_enable_fast_tick_if_needed();
        *down_timestamp = movement_state.fast_ticks + 1;
        return button_down_event_type;
    } else {
        // this line is hack but it handles the situation where the light button was held for more than 20 seconds.
        // fast tick is disabled by then, and the LED would get stuck on since there's no one left decrementing light_ticks.
        if (movement_state.light_ticks == 1) movement_state.light_ticks = 0;
        // now that that's out of the way, handle falling edge
        uint16_t diff = movement_state.fast_ticks - *down_timestamp;
        *down_timestamp = 0;
        _movement_disable_fast_tick_if_possible();
        // any press over a half second is considered a long press. Fire the long-up event
        if (diff > MOVEMENT_LONG_PRESS_TICKS) return button_down_event_type + 3;
        else return button_down_event_type + 1;
    }
}

void cb_light_btn_interrupt(void) {
    bool pin_level = HAL_GPIO_BTN_LIGHT_read();
    _movement_reset_inactivity_countdown();
    event.event_type = _figure_out_button_event(pin_level, EVENT_LIGHT_BUTTON_DOWN, &movement_state.light_down_timestamp);
}

void cb_mode_btn_interrupt(void) {
    bool pin_level = HAL_GPIO_BTN_MODE_read();
    _movement_reset_inactivity_countdown();
    event.event_type = _figure_out_button_event(pin_level, EVENT_MODE_BUTTON_DOWN, &movement_state.mode_down_timestamp);
}

void cb_alarm_btn_interrupt(void) {
    bool pin_level = HAL_GPIO_BTN_ALARM_read();
    _movement_reset_inactivity_countdown();
    event.event_type = _figure_out_button_event(pin_level, EVENT_ALARM_BUTTON_DOWN, &movement_state.alarm_down_timestamp);
}

void cb_alarm_btn_extwake(void) {
    // wake up!
    _movement_reset_inactivity_countdown();
}

void cb_alarm_fired(void) {
    movement_state.woke_from_alarm_handler = true;
}

void cb_fast_tick(void) {
    movement_state.fast_ticks++;
    if (movement_state.light_ticks > 0) movement_state.light_ticks--;
    if (movement_state.alarm_ticks > 0) movement_state.alarm_ticks--;
    // check timestamps and auto-fire the long-press events
    // Notice: is it possible that two or more buttons have an identical timestamp? In this case
    // only one of these buttons would receive the long press event. Don't bother for now...
    if (movement_state.light_down_timestamp > 0)
        if (movement_state.fast_ticks - movement_state.light_down_timestamp == MOVEMENT_LONG_PRESS_TICKS + 1)
            event.event_type = EVENT_LIGHT_LONG_PRESS;
    if (movement_state.mode_down_timestamp > 0)
        if (movement_state.fast_ticks - movement_state.mode_down_timestamp == MOVEMENT_LONG_PRESS_TICKS + 1)
            event.event_type = EVENT_MODE_LONG_PRESS;
    if (movement_state.alarm_down_timestamp > 0)
        if (movement_state.fast_ticks - movement_state.alarm_down_timestamp == MOVEMENT_LONG_PRESS_TICKS + 1)
            event.event_type = EVENT_ALARM_LONG_PRESS;
    // this is just a fail-safe; fast tick should be disabled as soon as the button is up, the LED times out, and/or the alarm finishes.
    // but if for whatever reason it isn't, this forces the fast tick off after 20 seconds.
    if (movement_state.fast_ticks >= 128 * 20) {
        watch_rtc_disable_periodic_callback(128);
        movement_state.fast_tick_enabled = false;
    }
}

void cb_tick(void) {
    event.event_type = EVENT_TICK;
    watch_date_time_t date_time = watch_rtc_get_date_time();
    if (date_time.unit.second != movement_state.last_second) {
        // TODO: can we consolidate these two ticks?
        if (movement_state.le_mode_ticks > 0) movement_state.le_mode_ticks--;
        if (movement_state.timeout_ticks > 0) movement_state.timeout_ticks--;

        movement_state.last_second = date_time.unit.second;
        movement_state.subsecond = 0;
    } else {
        movement_state.subsecond++;
    }
}

void cb_accelerometer_event(void) {
    uint8_t int_src = lis2dw_get_interrupt_source();

    if (int_src & LIS2DW_REG_ALL_INT_SRC_DOUBLE_TAP) {
        event.event_type = EVENT_DOUBLE_TAP;
        printf("Double tap!\n");
    }
    if (int_src & LIS2DW_REG_ALL_INT_SRC_SINGLE_TAP) {
        event.event_type = EVENT_SINGLE_TAP;
        printf("Single tap!\n");
    }
}

void cb_accelerometer_wake(void) {
    event.event_type = EVENT_ACCELEROMETER_WAKE;
    // also: wake up!
    _movement_reset_inactivity_countdown();
}
