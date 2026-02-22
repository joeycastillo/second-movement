/*
 * MIT License
 *
 * Copyright (c) 2022 Joey Castillo
 * Copyright (c) 2025 Alessandro Genova
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
#define MOVEMENT_REALLY_LONG_PRESS_TICKS 192
#define MOVEMENT_MAX_LONG_PRESS_TICKS 1280 // get a chance to check if a button held down over 10 seconds is a glitch

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
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
#include "adc.h"

#include "movement_config.h"
#include "movement_defaults.h"
#include "sleep_tracker_face.h"
#include "circadian_score.h"

#include "movement_custom_signal_tunes.h"
#include "sleep_data.h"
#include "smart_alarm_face.h"

#ifdef PHASE_ENGINE_ENABLED
// Phase 3B: Playlist controller integration
// Placeholder - will wire to face dispatch in PR 4
// For now, just verify playlist compiles and links
#include "playlist.h"
#endif

#if __EMSCRIPTEN__
#include <emscripten.h>
void _wake_up_simulator(void);
#else
#include "watch_usb_cdc.h"
#endif

volatile movement_state_t movement_state;
void * watch_face_contexts[MOVEMENT_NUM_FACES];
watch_date_time_t scheduled_tasks[MOVEMENT_NUM_FACES];
const int32_t movement_le_inactivity_deadlines[8] = {INT_MAX, 600, 3600, 7200, 21600, 43200, 86400, 604800};
const int16_t movement_timeout_inactivity_deadlines[4] = {60, 120, 300, 1800};

const uint32_t _movement_mode_button_events_mask = 0b11111 << EVENT_MODE_BUTTON_DOWN;
const uint32_t _movement_light_button_events_mask = 0b11111 << EVENT_LIGHT_BUTTON_DOWN;
const uint32_t _movement_alarm_button_events_mask = 0b11111 << EVENT_ALARM_BUTTON_DOWN;
const uint32_t _movement_button_events_mask = _movement_mode_button_events_mask | _movement_light_button_events_mask | _movement_alarm_button_events_mask;

typedef struct {
    movement_event_type_t down_event;
    watch_cb_t cb_longpress;
    movement_timeout_index_t timeout_index;
    volatile bool is_down;
    volatile rtc_counter_t down_timestamp;
#if MOVEMENT_DEBOUNCE_TICKS
    volatile rtc_counter_t up_timestamp;
#endif
} movement_button_t;

/* Pieces of state that can be modified by the various interrupt callbacks.
   The interrupt writes state changes here, and it will be acted upon on the next app_loop invokation.
*/
typedef struct {
    volatile uint32_t pending_events;
    volatile bool turn_led_off;
    volatile bool has_pending_sequence;
    volatile bool enter_sleep_mode;
    volatile bool exit_sleep_mode;
    volatile bool is_sleeping;
    volatile uint8_t subsecond;
    volatile rtc_counter_t minute_counter;
    volatile bool minute_alarm_fired;
    volatile bool is_buzzing;
    volatile uint8_t pending_sequence_priority;
    volatile bool schedule_next_comp;
    volatile bool has_pending_accelerometer;
    volatile bool accelerometer_woke;  // set in ISR; I2C reads deferred to app_loop

    // button tracking for long press
    movement_button_t mode_button;
    movement_button_t light_button;
    movement_button_t alarm_button;

    // button events that will not be passed to the current face loop, but will instead passed directly to the default loop handler.
    volatile uint32_t passthrough_events;
} movement_volatile_state_t;

movement_volatile_state_t movement_volatile_state;

// The last sequence that we have been asked to play while the watch was in deep sleep
static int8_t *_pending_sequence;

// The note sequence of the default alarm
int8_t alarm_tune[] = {
    BUZZER_NOTE_C8, 3,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_C8, 3,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_C8, 3,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_C8, 5,
    BUZZER_NOTE_REST, 38,
    -8, 9,
    0
};

int8_t _movement_dst_offset_cache[NUM_ZONE_NAMES] = {0};
#define TIMEZONE_DOES_NOT_OBSERVE (-127)

// Sleep tracking data (70 bytes)
static sleep_data_t sleep_data;
static bool sleep_data_dirty = false;  // Tracks if we need to save to flash

// Circadian Score Data (receives completed sleep sessions)
static circadian_data_t global_circadian_data = {0};
static bool circadian_data_initialized = false;

/* Sleep tracker state (Cole-Kripke algorithm) */
static sleep_tracker_state_t global_sleep_tracker = {0};
static uint32_t last_sleep_tick = 0;
static uint16_t sleep_minute_counter = 0;

// Active Hours configuration (BKUP[2] storage)
// Format: 17 bits packed (7-bit start, 7-bit end, 1-bit enabled, 17 reserved)
typedef struct {
    uint8_t start;   // 0-95 (15-min increments, 0=00:00, 95=23:45)
    uint8_t end;     // 0-95
    bool enabled;    // True = use Active Hours, False = 24h active
} active_hours_config_t;

static active_hours_config_t get_active_hours(void) {
    uint32_t reg = watch_get_backup_data(2);
    active_hours_config_t config;
    
    // Extract from BKUP[2]
    config.start = (reg & 0x7F);         // Bits 0-6
    config.end = ((reg >> 7) & 0x7F);    // Bits 7-13
    config.enabled = ((reg >> 14) & 0x1); // Bit 14
    
    // Validate or set defaults (04:00-23:00)
    if (config.start > 95 || config.end > 95 || config.start == config.end) {
        config.start = 16;   // 04:00 (4 * 4)
        config.end = 92;     // 23:00 (23 * 4)
        config.enabled = true;
        
        // Save defaults
        uint32_t default_reg = config.start | (config.end << 7) | (config.enabled << 14);
        watch_store_backup_data(default_reg, 2);
    }
    
    return config;
}

void cb_mode_btn_interrupt(void);
void cb_light_btn_interrupt(void);
void cb_alarm_btn_interrupt(void);
void cb_alarm_btn_extwake(void);
void cb_minute_alarm_fired(void);
void cb_tick(void);
void cb_mode_btn_timeout_interrupt(void);
void cb_light_btn_timeout_interrupt(void);
void cb_alarm_btn_timeout_interrupt(void);
void cb_led_timeout_interrupt(void);
void cb_resign_timeout_interrupt(void);
void cb_sleep_timeout_interrupt(void);
void cb_buzzer_start(void);
void cb_buzzer_stop(void);

void cb_accelerometer_event(void);
void cb_accelerometer_wake(void);
static bool is_sleep_window(void);
static bool is_confirmed_asleep(void);

#ifdef PHASE_ENGINE_ENABLED
static uint8_t _movement_get_zone_face_index(phase_zone_t zone);
#endif

// Expose sleep tracker state for smart alarm integration
sleep_tracker_state_t* movement_get_sleep_tracker_state(void) {
    return &global_sleep_tracker;
}

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

static watch_buzzer_volume_t _movement_get_buzzer_volume(movement_buzzer_priority_t priority) {
    switch (priority) {
        case BUZZER_PRIORITY_BUTTON:
            return movement_button_volume();
        case BUZZER_PRIORITY_SIGNAL:
            return movement_signal_volume();
        case BUZZER_PRIORITY_ALARM:
            return movement_alarm_volume();
        default:
            return WATCH_BUZZER_VOLUME_LOUD;
    }
}

static void _movement_set_top_of_minute_alarm() {
    uint32_t counter = watch_rtc_get_counter();
    uint32_t next_minute_counter;
    watch_date_time_t date_time = watch_rtc_get_date_time();
    uint32_t freq = watch_rtc_get_frequency();
    uint32_t half_freq = freq >> 1;
    uint32_t subsecond_mask = freq - 1;
    uint32_t ticks_per_minute = watch_rtc_get_ticks_per_minute();

    // get the counter at the last second tick
    next_minute_counter = counter & (~subsecond_mask);
    // add/subtract half second shift to sync up second tick with the 1Hz interrupt
    next_minute_counter += (counter & subsecond_mask) >= half_freq ? half_freq : -half_freq;
    // counter at the next top of the minute
    next_minute_counter += (60 - date_time.unit.second) * freq;

    // Since the minute alarm is very important, double/triple check to make sure that it will fire.
    // These are theoretical corner cases that probably can't even happen, but since we do a subtraction
    // above I wanna be certain that we don't schedule the next alarm at a counter value just before the
    // current counter, which would result in the alarm firing after more than one year.
    // This should be robust to the counter overflow, and we should ever iterate once at most.
    if (next_minute_counter == counter) {
        next_minute_counter += ticks_per_minute;
    }

    while ((next_minute_counter - counter) > ticks_per_minute) {
        next_minute_counter += ticks_per_minute;
    }

    movement_volatile_state.minute_counter = next_minute_counter;

    watch_rtc_register_comp_callback_no_schedule(cb_minute_alarm_fired, next_minute_counter, MINUTE_TIMEOUT);
    movement_volatile_state.schedule_next_comp = true;
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
    rtc_counter_t counter = watch_rtc_get_counter();
    uint32_t freq = watch_rtc_get_frequency();

    watch_rtc_register_comp_callback_no_schedule(
        cb_resign_timeout_interrupt,
        counter + movement_timeout_inactivity_deadlines[movement_state.settings.bit.to_interval] * freq,
        RESIGN_TIMEOUT
    );

    movement_volatile_state.enter_sleep_mode = false;

    watch_rtc_register_comp_callback_no_schedule(
        cb_sleep_timeout_interrupt,
        counter + movement_le_inactivity_deadlines[movement_state.settings.bit.le_interval] * freq,
        SLEEP_TIMEOUT
    );

    movement_volatile_state.schedule_next_comp = true;
}

static inline void _movement_disable_inactivity_countdown(void) {
    watch_rtc_disable_comp_callback_no_schedule(RESIGN_TIMEOUT);
    watch_rtc_disable_comp_callback_no_schedule(SLEEP_TIMEOUT);
    movement_volatile_state.schedule_next_comp = true;
}

static void _movement_renew_top_of_minute_alarm(void) {
    // Renew the alarm for a minute from the previous one (ensures no drift)
    movement_volatile_state.minute_counter += watch_rtc_get_ticks_per_minute();
    watch_rtc_register_comp_callback_no_schedule(cb_minute_alarm_fired, movement_volatile_state.minute_counter, MINUTE_TIMEOUT);
    movement_volatile_state.schedule_next_comp = true;
}

// Forward declarations -- defined after app_loop (depends on sleep helpers below).
static void _movement_handle_accelerometer_wake(void);
static bool is_sleep_window(void);

static uint32_t _movement_get_accelerometer_events() {
    uint32_t accelerometer_events = 0;

    uint8_t int_src = lis2dw_get_interrupt_source();

    if (int_src & LIS2DW_REG_ALL_INT_SRC_DOUBLE_TAP) {
        accelerometer_events |= 1 << EVENT_DOUBLE_TAP;
        // Wake display on tap
        _movement_reset_inactivity_countdown();
    }

    if (int_src & LIS2DW_REG_ALL_INT_SRC_SINGLE_TAP) {
        accelerometer_events |= 1 << EVENT_SINGLE_TAP;
        // Wake display on tap
        _movement_reset_inactivity_countdown();
    }

    return accelerometer_events;
}

static void _movement_handle_button_presses(uint32_t pending_events) {
    bool any_up = false;
    bool any_down = false;
    bool any_long = false;

    movement_button_t* buttons[3] = {
        &movement_volatile_state.mode_button,
        &movement_volatile_state.light_button,
        &movement_volatile_state.alarm_button
    };

    uint32_t button_events_masks[3] = {
        _movement_mode_button_events_mask,
        _movement_light_button_events_mask,
        _movement_alarm_button_events_mask,
    };

    for (uint8_t i = 0; i < 3; i++) {
        movement_button_t* button = buttons[i];

        // If a button down occurred
        if (pending_events & (1 << button->down_event)) {
            watch_rtc_register_comp_callback_no_schedule(button->cb_longpress, button->down_timestamp + MOVEMENT_LONG_PRESS_TICKS, button->timeout_index);
            any_down = true;
            // this button's events will start getting passed to the face
            movement_volatile_state.passthrough_events &= ~button_events_masks[i];
        }

        // If a long press occurred
        if (pending_events & (1 << (button->down_event + 2))) {
            watch_rtc_register_comp_callback_no_schedule(button->cb_longpress, button->down_timestamp + MOVEMENT_REALLY_LONG_PRESS_TICKS, button->timeout_index);
            any_long = true;
        }

        // If a really long press occurred
        if (pending_events & (1 << (button->down_event + 4))) {
            watch_rtc_register_comp_callback_no_schedule(button->cb_longpress, button->down_timestamp + MOVEMENT_MAX_LONG_PRESS_TICKS, button->timeout_index);
            any_long = true;
        }

        // If a button up or button long up occurred
        if (pending_events & (
            (1 << (button->down_event + 1)) |
            (1 << (button->down_event + 3))
            // (1 << (button->down_event + 5))
        )) {
            // We cancel the timeout if it hasn't fired yet
            watch_rtc_disable_comp_callback_no_schedule(button->timeout_index);
            any_up = true;
        }
    }

    if (any_down) {
        // force alarm off if the user pressed a button.
        watch_buzzer_abort_sequence();

        // Delay auto light off if the user is still interacting with the watch.
        if (movement_state.light_on) {
            movement_illuminate_led();
        }
    }

    if (any_down || any_up || any_long) {
        _movement_reset_inactivity_countdown();
        movement_volatile_state.schedule_next_comp = true;
    }
}

static void _movement_handle_top_of_minute(void) {
    watch_date_time_t date_time = watch_rtc_get_date_time();

    // update the DST offset cache every 30 minutes, since someplace in the world could change.
    if (date_time.unit.minute % 30 == 0) {
        _movement_update_dst_offset_cache();
    }
    
    // Stream 4: Save sleep tracking data to flash periodically
    // Save once per hour during sleep window to batch writes and reduce flash wear
    // Also save at end of sleep window to persist the complete night.
    // End-of-sleep hour is user-configurable via BKUP[2] (active hours start = wake-up time).
    if (movement_state.has_lis2dw) {
        movement_active_hours_t active_hours = movement_get_active_hours();
        uint8_t sleep_end_hour = active_hours.bit.start_quarter_hours / 4;
        bool is_end_of_sleep = (active_hours.bit.enabled &&
                                date_time.unit.hour == sleep_end_hour &&
                                date_time.unit.minute == 0);
        bool is_hourly_save = (date_time.unit.minute == 0);
        
        if (is_end_of_sleep || (is_hourly_save && is_sleep_window())) {
            sleep_tracking_save_to_flash();
        }
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
    
    // Sleep Tracking Session Management
    // Track state transitions to start/end sleep sessions
    static bool was_in_sleep_window = false;
    bool now_in_sleep_window = is_sleep_window();
    
    if (now_in_sleep_window && !was_in_sleep_window) {
        // Entering sleep window (e.g., 23:00) - start new session
        sleep_tracker_start_session(&global_sleep_tracker);
        last_sleep_tick = watch_rtc_get_counter();
        sleep_minute_counter = 0;
    } else if (!now_in_sleep_window && was_in_sleep_window) {
        // Leaving sleep window (e.g., 04:00) - end session
        sleep_tracker_end_session(&global_sleep_tracker);
        
        // Export completed night data to Circadian Score system
        if (global_sleep_tracker.total_sleep_minutes > 0) {
            // Calculate derived metrics
            uint16_t total_minutes = global_sleep_tracker.total_sleep_minutes + 
                                    global_sleep_tracker.total_wake_minutes;
            uint8_t efficiency = (total_minutes > 0) ? 
                (global_sleep_tracker.total_sleep_minutes * 100 / total_minutes) : 0;
            uint8_t light_quality = (total_minutes > 0) ?
                (global_sleep_tracker.total_dark_minutes * 100 / total_minutes) : 0;
            
            circadian_sleep_night_t night = {
                .onset_timestamp = global_sleep_tracker.sleep_onset_time,
                .offset_timestamp = global_sleep_tracker.sleep_offset_time,
                .duration_min = global_sleep_tracker.total_sleep_minutes,
                .efficiency = efficiency,
                .waso_min = global_sleep_tracker.total_wake_minutes,
                .awakenings = global_sleep_tracker.num_awakenings,
                .light_quality = light_quality,
                .valid = true
            };
            
            // Load circadian data if not initialized
            if (!circadian_data_initialized) {
                circadian_data_load_from_flash(&global_circadian_data);
                circadian_data_initialized = true;
            }
            
            // Add night and persist
            circadian_data_add_night(&global_circadian_data, &night);
        }
    }
    
    was_in_sleep_window = now_in_sleep_window;

#ifdef PHASE_ENGINE_ENABLED
    // PR #66: Sample lux every minute (lightweight)
    sensors_sample_lux(&movement_state.sensors);
    
    // Phase 3: Update metrics engine every 15 minutes
    movement_state.metric_tick_count++;
    if (movement_state.metric_tick_count >= 15) {
        movement_state.metric_tick_count = 0;
        
        // PR #65 + #66: Full sensor update (motion + lux + temperature)
        sensors_update(&movement_state.sensors);
        
        // Gather current sensor readings
        uint8_t hour = date_time.unit.hour;
        uint8_t minute = date_time.unit.minute;
        uint16_t day_of_year = date_time.unit.day;  // Simplified; actual day_of_year calculation needed
        
        // Get phase score from phase engine (stubbed for now - requires Phase 1-2 integration)
        uint16_t phase_score = 50;  // Default midpoint; replace with phase_compute() when available
        
        // Update metrics engine (sensors passed directly for cleaner API)
        metrics_update(&movement_state.metrics,
                      &movement_state.sensors,
                      hour, minute, day_of_year,
                      (uint8_t)phase_score,
                      movement_state.cumulative_activity,
                      &global_circadian_data,
                      NULL,  // homebase - requires Phase 2 homebase integration
                      movement_state.has_lis2dw);
        
        // Update playlist controller
        metrics_snapshot_t snapshot;
        metrics_get(&movement_state.metrics, &snapshot);
        
        // Get previous zone before update
        phase_zone_t prev_zone = playlist_get_zone(&movement_state.playlist);
        
        playlist_update(&movement_state.playlist, phase_score, &snapshot);
        
        // Phase 4B: If playlist mode is active and zone changed, switch to zone face
        if (movement_state.playlist_mode_active) {
            phase_zone_t new_zone = playlist_get_zone(&movement_state.playlist);
            
            // Only switch faces on zone changes (debounced by playlist hysteresis)
            if (new_zone != prev_zone) {
                uint8_t zone_face_idx = _movement_get_zone_face_index(new_zone);
                movement_move_to_face(zone_face_idx);
            }
        }
    }
#endif
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
    // Movement requires at least a 1 Hz tick.
    // If we are asked for an invalid frequency, default back to 1 Hz.
    if (freq == 0 || __builtin_popcount(freq) != 1) freq = 1;

    // disable all periodic callbacks
    watch_rtc_disable_matching_periodic_callbacks(0xFF);

    // this left-justifies the period in a 32-bit integer.
    uint32_t tmp = (freq & 0xFF) << 24;
    // now we can count the leading zeroes to get the value we need.
    // 0x01 (1 Hz) will have 7 leading zeros for PER7. 0x80 (128 Hz) will have no leading zeroes for PER0.
    uint8_t per_n = __builtin_clz(tmp);

    movement_state.tick_frequency = freq;
    movement_state.tick_pern = per_n;

    watch_rtc_register_periodic_callback(cb_tick, freq);
}

void movement_illuminate_led(void) {
    if (movement_state.settings.bit.led_duration != 0b111) {
        movement_state.light_on = true;
        watch_set_led_color_rgb(movement_state.settings.bit.led_red_color | movement_state.settings.bit.led_red_color << 4,
                                movement_state.settings.bit.led_green_color | movement_state.settings.bit.led_green_color << 4,
                                movement_state.settings.bit.led_blue_color | movement_state.settings.bit.led_blue_color << 4);
        if (movement_state.settings.bit.led_duration == 0) {
            // Do nothing it'll be turned off on button release
        } else {
            // Set a timeout to turn off the light
            rtc_counter_t counter = watch_rtc_get_counter();
            uint32_t freq = watch_rtc_get_frequency();
            watch_rtc_register_comp_callback_no_schedule(
                cb_led_timeout_interrupt,
                counter + (movement_state.settings.bit.led_duration * 2 - 1) * freq,
                LED_TIMEOUT
            );
            movement_volatile_state.schedule_next_comp = true;
        }
    }
}

void movement_force_led_on(uint8_t red, uint8_t green, uint8_t blue) {
    // this is hacky, we need a way for watch faces to set an arbitrary color and prevent Movement from turning it right back off.
    movement_state.light_on = true;
    watch_set_led_color_rgb(red, green, blue);
    // The led will stay on until movement_force_led_off is called, so disable the led timeout in case we were in the middle of it.
    watch_rtc_disable_comp_callback_no_schedule(LED_TIMEOUT);
    movement_volatile_state.schedule_next_comp = true;
}

void movement_force_led_off(void) {
    movement_state.light_on = false;
    // The led timeout probably already triggered, but still disable just in case we are switching off the light by other means
    watch_rtc_disable_comp_callback_no_schedule(LED_TIMEOUT);
    movement_volatile_state.schedule_next_comp = true;
    watch_set_led_off();
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
        case EVENT_LIGHT_LONG_UP:
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
#ifdef PHASE_ENGINE_ENABLED
        case EVENT_ALARM_LONG_PRESS:
            // Phase 4B: Long-press ALARM from clock → enter phase playlist mode
            if (movement_state.current_face_idx == 1) {  // Clock face (index 1)
                movement_state.playlist_mode_active = true;
                
                // Jump to zone face for current zone
                phase_zone_t zone = playlist_get_zone(&movement_state.playlist);
                uint8_t zone_face_idx = _movement_get_zone_face_index(zone);
                movement_move_to_face(zone_face_idx);
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
            // Phase 4B: If in playlist mode, short-press ALARM cycles through zone metrics
            if (movement_state.playlist_mode_active) {
                playlist_advance(&movement_state.playlist);
                // Stay on the same zone face, metric changes internally via ALARM button
            }
            break;
#endif
        default:
            break;
    }

    return true;
}

#ifdef PHASE_ENGINE_ENABLED
/**
 * Get the watch face index for the current zone.
 * Maps zones to their corresponding zone face indices.
 * 
 * Zone faces are positioned at indices 2-5 (after wyoscan and clock):
 * - Index 2: emergence_face (ZONE_EMERGENCE)
 * - Index 3: momentum_face (ZONE_MOMENTUM)
 * - Index 4: active_face (ZONE_ACTIVE)
 * - Index 5: descent_face (ZONE_DESCENT)
 * 
 * @param zone Current zone (0-3)
 * @return Face index (2-5)
 */
static uint8_t _movement_get_zone_face_index(phase_zone_t zone) {
    // Zone faces start at index 2 (after wyoscan[0] and clock[1])
    return 2 + zone;
}
#endif

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
    movement_volatile_state.enter_sleep_mode = true;
}

void movement_request_wake() {
    movement_volatile_state.exit_sleep_mode = true;
    _movement_reset_inactivity_countdown();
}

void cb_buzzer_start(void) {
    movement_volatile_state.is_buzzing = true;
}

void cb_buzzer_stop(void) {
    movement_volatile_state.is_buzzing = false;
    movement_volatile_state.pending_sequence_priority = 0;
}

void movement_play_note(watch_buzzer_note_t note, uint16_t duration_ms) {
    static int8_t single_note_sequence[3];

    single_note_sequence[0] = note;
    // 64 ticks per second for the tc0
    // Each tick is approximately 15ms
    uint16_t duration = duration_ms / 15;
    if (duration > 127) duration = 127;
    single_note_sequence[1] = (int8_t)duration;
    single_note_sequence[2] = 0;

    movement_play_sequence(single_note_sequence, BUZZER_PRIORITY_BUTTON);
}

void movement_play_signal(void) {
    movement_play_sequence(signal_tune, BUZZER_PRIORITY_SIGNAL);
}

void movement_play_alarm(void) {
    movement_play_sequence(alarm_tune, BUZZER_PRIORITY_ALARM);
}

void movement_play_alarm_beeps(uint8_t rounds, watch_buzzer_note_t alarm_note) {
    // Ugly but necessary to avoid breaking backward compatibility with some faces.
    // Create an alarm tune on the fly with the specified note and repetition.
    static int8_t custom_alarm_tune[19];

    if (rounds == 0) rounds = 1;
    if (rounds > 20) rounds = 20;

    for (uint8_t i = 0; i < 9; i++) {
        uint8_t note_idx = i * 2;
        uint8_t duration_idx = note_idx + 1;

        int8_t note = alarm_tune[note_idx];
        int8_t duration = alarm_tune[duration_idx];

        if (note == BUZZER_NOTE_C8) {
            note = alarm_note;
        } else if (note < 0) {
            duration = rounds;
        }

        custom_alarm_tune[note_idx] = note;
        custom_alarm_tune[duration_idx] = duration;
    }

    custom_alarm_tune[18] = 0;

    movement_play_sequence(custom_alarm_tune, BUZZER_PRIORITY_ALARM);
}

void movement_play_sequence(int8_t *note_sequence, movement_buzzer_priority_t priority) {
    // Priority is used to ensure that lower priority sequences don't cancel higher priority ones
    // Priotity order: alarm(2) > signal(1) > note(0)
    if (priority < movement_volatile_state.pending_sequence_priority) {
        return;
    }

    movement_volatile_state.pending_sequence_priority = priority;

    // The tcc is off during sleep, we can't play immediately.
    // Ask to wake up the watch.
    if (movement_volatile_state.is_sleeping) {
        _pending_sequence = note_sequence;
        movement_volatile_state.has_pending_sequence = true;
        movement_volatile_state.exit_sleep_mode = true;
    } else {
        watch_buzzer_play_sequence_with_volume(note_sequence, NULL, _movement_get_buzzer_volume(priority));
    }
}

uint8_t movement_claim_backup_register(void) {
    // We use backup register 7 in watch_rtc to keep track of the reference time
    if (movement_state.next_available_backup_register >= 7) return 0;
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
    unix_timestamp_t timestamp = watch_rtc_get_unix_time();
    return watch_utility_date_time_from_unix_time(timestamp, offset);
}

watch_date_time_t movement_get_local_date_time(void) {
    static struct {
        unix_timestamp_t timestamp;
        rtc_date_time_t datetime;
    } cached_date_time = {.datetime.reg=0, .timestamp=0};

    unix_timestamp_t timestamp = watch_rtc_get_unix_time();

    if (timestamp != cached_date_time.timestamp) {
        cached_date_time.timestamp = timestamp;
        cached_date_time.datetime = watch_utility_date_time_from_unix_time(timestamp, movement_get_current_timezone_offset());
    }

    return cached_date_time.datetime;
}

uint32_t movement_get_utc_timestamp(void) {
    return watch_rtc_get_unix_time();
}

void movement_set_utc_date_time(watch_date_time_t date_time) {
    movement_set_utc_timestamp(watch_utility_date_time_to_unix_time(date_time, 0));
}

void movement_set_local_date_time(watch_date_time_t date_time) {
    int32_t current_offset = movement_get_current_timezone_offset();
    movement_set_utc_timestamp(watch_utility_date_time_to_unix_time(date_time, current_offset));
}

void movement_set_utc_timestamp(uint32_t timestamp) {
    watch_rtc_set_unix_time(timestamp);

    // If the time was changed, the top of the minute alarm needs to be reset accordingly
    _movement_set_top_of_minute_alarm();

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

watch_buzzer_volume_t movement_signal_volume(void) {
    return movement_state.signal_volume;
}
void movement_set_signal_volume(watch_buzzer_volume_t value) {
    movement_state.signal_volume = value;
}

watch_buzzer_volume_t movement_alarm_volume(void) {
    return movement_state.alarm_volume;
}

void movement_set_alarm_volume(watch_buzzer_volume_t value) {
    movement_state.alarm_volume = value;
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
        lis2dw_configure_tap_duration(2, 2, 2);

        // Configure for low power operation BEFORE ramping to 400 Hz
        // This ensures we stay in LP mode and achieve ~45-90µA instead of ~400µA
        lis2dw_set_mode(LIS2DW_MODE_LOW_POWER);
        lis2dw_set_low_power_mode(LIS2DW_LP_MODE_1);  // 12-bit, lowest power
        lis2dw_set_low_noise_mode(false);  // Low noise increases power consumption
        lis2dw_set_data_rate(LIS2DW_DATA_RATE_HP_400_HZ);  // 400 Hz needed for tap detection
        lis2dw_enable_double_tap();

        // Settling time (1 sample duration, i.e. 1/400Hz)
        delay_ms(3);

        // enable tap detection on INT1/A3.
        lis2dw_configure_int1(LIS2DW_CTRL4_INT1_SINGLE_TAP | LIS2DW_CTRL4_INT1_DOUBLE_TAP);

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
        lis2dw_disable_double_tap();
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

movement_active_hours_t movement_get_active_hours(void) {
    movement_active_hours_t settings;
    settings.reg = watch_get_backup_data(2);
    
    // Check if backup register is uninitialized (all zeros or all ones)
    // Initialize with defaults from movement_defaults.h
    if (settings.reg == 0 || settings.reg == 0xFFFFFFFF) {
        settings.bit.start_quarter_hours = MOVEMENT_DEFAULT_ACTIVE_HOURS_START;
        settings.bit.end_quarter_hours = MOVEMENT_DEFAULT_ACTIVE_HOURS_END;
        settings.bit.enabled = MOVEMENT_DEFAULT_ACTIVE_HOURS_ENABLED;
        settings.bit.reserved = 0;
        movement_set_active_hours(settings);
    }

    // Validate values are in range (0-95)
    if (settings.bit.start_quarter_hours > 95) settings.bit.start_quarter_hours = MOVEMENT_DEFAULT_ACTIVE_HOURS_START;
    if (settings.bit.end_quarter_hours > 95) settings.bit.end_quarter_hours = MOVEMENT_DEFAULT_ACTIVE_HOURS_END;
    
    return settings;
}

void movement_set_active_hours(movement_active_hours_t settings) {
    // Clamp values to valid range
    if (settings.bit.start_quarter_hours > 95) settings.bit.start_quarter_hours = 95;
    if (settings.bit.end_quarter_hours > 95) settings.bit.end_quarter_hours = 95;
    
    watch_store_backup_data(settings.reg, 2);
}

float movement_get_temperature(void) {
    float temperature_c = (float)0xFFFFFFFF;
#if __EMSCRIPTEN__
    temperature_c = EM_ASM_DOUBLE({
        return temp_c || 25.0;
    });
#else

    if (movement_state.has_thermistor) {
        thermistor_driver_enable();
        temperature_c = thermistor_driver_get_temperature();
        thermistor_driver_disable();
    } else if (movement_state.has_lis2dw) {
            int16_t val = lis2dw_get_temperature();
            val = val >> 4;
            temperature_c = 25 + (float)val / 16.0;
    }
#endif

    return temperature_c;
}

void app_init(void) {
    _watch_init();

    filesystem_init();

    // check if we are plugged into USB power.
    HAL_GPIO_VBUS_DET_in();
    HAL_GPIO_VBUS_DET_pulldown();
    delay_ms(100);
    if (HAL_GPIO_VBUS_DET_read()){
        /// if so, enable USB functionality.
        _watch_enable_usb();
    }
    HAL_GPIO_VBUS_DET_off();

    memset((void *)&movement_state, 0, sizeof(movement_state));

    movement_volatile_state.pending_events = 0;
    movement_volatile_state.turn_led_off = false;

    movement_volatile_state.minute_alarm_fired = false;
    movement_volatile_state.minute_counter = 0;

    movement_volatile_state.enter_sleep_mode = false;
    movement_volatile_state.exit_sleep_mode = false;
    movement_volatile_state.has_pending_sequence = false;
    movement_volatile_state.has_pending_accelerometer = false;
    movement_volatile_state.accelerometer_woke = false;
    movement_volatile_state.is_sleeping = false;

    movement_volatile_state.is_buzzing = false;
    movement_volatile_state.pending_sequence_priority = 0;

    movement_volatile_state.mode_button.down_event = EVENT_MODE_BUTTON_DOWN;
    movement_volatile_state.mode_button.is_down = false;
    movement_volatile_state.mode_button.down_timestamp = 0;
    movement_volatile_state.mode_button.timeout_index = MODE_BUTTON_TIMEOUT;
    movement_volatile_state.mode_button.cb_longpress = cb_mode_btn_timeout_interrupt;

    movement_volatile_state.light_button.down_event = EVENT_LIGHT_BUTTON_DOWN;
    movement_volatile_state.light_button.is_down = false;
    movement_volatile_state.light_button.down_timestamp = 0;
    movement_volatile_state.light_button.timeout_index = LIGHT_BUTTON_TIMEOUT;
    movement_volatile_state.light_button.cb_longpress = cb_light_btn_timeout_interrupt;

    movement_volatile_state.alarm_button.down_event = EVENT_ALARM_BUTTON_DOWN;
    movement_volatile_state.alarm_button.is_down = false;
    movement_volatile_state.alarm_button.down_timestamp = 0;
    movement_volatile_state.alarm_button.timeout_index = ALARM_BUTTON_TIMEOUT;
    movement_volatile_state.alarm_button.cb_longpress = cb_alarm_btn_timeout_interrupt;

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

    // Initialize location defaults on first boot (BKUP[1])
    // Only applies if defaults are non-zero and register is uninitialized.
#if (MOVEMENT_DEFAULT_LATITUDE != 0 || MOVEMENT_DEFAULT_LONGITUDE != 0)
    {
        movement_location_t location;
        location.reg = watch_get_backup_data(1);
        if (location.reg == 0) {
            location.bit.latitude = MOVEMENT_DEFAULT_LATITUDE;
            location.bit.longitude = MOVEMENT_DEFAULT_LONGITUDE;
            watch_store_backup_data(location.reg, 1);
        }
    }
#endif

    watch_date_time_t date_time = watch_rtc_get_date_time();
    if (date_time.reg == 0) {
        date_time = watch_get_init_date_time();
        // but convert from local time to UTC
        date_time = watch_utility_date_time_convert_zone(date_time, movement_get_current_timezone_offset(), 0);
        watch_rtc_set_date_time(date_time);
    }

    // register callbacks to be notified when buzzer starts/stops playing.
    // this is so movement can be notified even when triggered by a face bypassing movement
    watch_buzzer_register_global_callbacks(cb_buzzer_start, cb_buzzer_stop);

    // populate the DST offset cache
    _movement_update_dst_offset_cache();

    if (movement_state.accelerometer_motion_threshold == 0) movement_state.accelerometer_motion_threshold = 32;

    movement_state.signal_volume = MOVEMENT_DEFAULT_SIGNAL_VOLUME;
    movement_state.alarm_volume = MOVEMENT_DEFAULT_ALARM_VOLUME;
    movement_state.light_on = false;
    // Reserve BKUP[0-3] for movement core (settings, location, active_hours, reserved)
    movement_state.next_available_backup_register = 4;
    _movement_reset_inactivity_countdown();

    // set up the 1 minute alarm (for background tasks and low power updates)
    _movement_set_top_of_minute_alarm();
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
    }

    // LCD autodetect uses the buttons as a a failsafe, so we should run it before we enable the button interrupts
    watch_enable_display();

    if (!movement_volatile_state.is_sleeping) {
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

            // Wake on motion for hybrid tap-to-wake: motion detection on INT2/A4 wakes the device,
            // then software polls tap registers to determine if it was a tap event.
            watch_register_extwake_callback(HAL_GPIO_A4_pin(), cb_accelerometer_wake, false);

            // later on, we are going to use INT1 for tap detection. We'll set up that interrupt here,
            // but it will only fire once tap recognition is enabled.
            watch_register_interrupt_callback(HAL_GPIO_A3_pin(), cb_accelerometer_event, INTERRUPT_TRIGGER_RISING);

            // Enable the interrupts...
            lis2dw_enable_interrupts();

            // Initialize sleep tracking (Stream 4)
            sleep_tracking_init();

            // Enable tap detection for tap-to-wake functionality
            // This will set the data rate to 400Hz for tap detection
            movement_enable_tap_detection_if_available();

            // At first boot, this next line sets the accelerometer's sampling rate to 0, which is LIS2DW_DATA_RATE_POWERDOWN.
            // This means the interrupts we just configured won't fire.
            // Tap detection will ramp up sensing and make use of the A3 interrupt.
            // If a watch face wants to check in on the A4 interrupt pin for motion status, it can call
            // movement_set_accelerometer_background_rate with another rate like LIS2DW_DATA_RATE_LOWEST or LIS2DW_DATA_RATE_25_HZ.
            // FIX: Don't override the 400Hz set by tap detection! Only override if background rate is HIGHER than 400Hz.
            if (movement_state.accelerometer_background_rate > LIS2DW_DATA_RATE_HP_400_HZ) {
                lis2dw_set_data_rate(movement_state.accelerometer_background_rate);
            }
        }
#endif

#ifdef PHASE_ENGINE_ENABLED
        // Phase 3: Initialize metrics engine and playlist controller
        memset(&movement_state.metrics, 0, sizeof(metrics_engine_t));
        memset(&movement_state.playlist, 0, sizeof(playlist_state_t));
        metrics_init(&movement_state.metrics);
        playlist_init(&movement_state.playlist);
        movement_state.metric_tick_count = 0;
        movement_state.cumulative_activity = 0;
        movement_state.playlist_mode_active = false;  // Disabled by default; enable via watch face
        
        // Phase 4A: Initialize sensor state (PR #65 + #66)
        sensors_init(&movement_state.sensors, movement_state.has_lis2dw);
        if (movement_state.has_lis2dw) {
            sensors_configure_accel(&movement_state.sensors);
        }
#endif

        movement_request_tick_frequency(1);

        for(uint8_t i = 0; i < MOVEMENT_NUM_FACES; i++) {
            watch_faces[i].setup(i, &watch_face_contexts[i]);
        }

        watch_faces[movement_state.current_face_idx].activate(watch_face_contexts[movement_state.current_face_idx]);
        movement_volatile_state.pending_events |=  1 << EVENT_ACTIVATE;
        
        // Initialize sleep tracker with default light modifiers
        static const int16_t default_light_modifiers[4] = {-200, -50, +100, +400};
        memcpy(global_sleep_tracker.light_modifiers, default_light_modifiers, sizeof(default_light_modifiers));
        global_sleep_tracker.tracking_active = false;
        sleep_minute_counter = 0;
        last_sleep_tick = 0;
        
        // Initialize circadian data (or load from flash)
        if (!circadian_data_initialized) {
            circadian_data_load_from_flash(&global_circadian_data);
            
            // Sync Active Hours from BKUP[2]
            active_hours_config_t config = get_active_hours();
            global_circadian_data.active_hours_start_min = (config.start * 15);  // Convert quarters to minutes
            global_circadian_data.active_hours_end_min = (config.end * 15);
            circadian_data_save_to_flash(&global_circadian_data);
            
            circadian_data_initialized = true;
        }
    }
}

#ifndef MOVEMENT_LOW_ENERGY_MODE_FORBIDDEN

static void _sleep_mode_app_loop(void) {
    // as long as we are in low energy mode, we wake up here, update the screen, and go right back to sleep.
    while (movement_volatile_state.is_sleeping) {
        // if we need to wake immediately, do it!
        if (movement_volatile_state.exit_sleep_mode) {
            movement_volatile_state.exit_sleep_mode = false;
            movement_volatile_state.is_sleeping = false;

            return;
        }

        // we also have to handle top-of-the-minute tasks here in the mini-runloop
        if (movement_volatile_state.minute_alarm_fired) {
            movement_volatile_state.minute_alarm_fired = false;
            _movement_renew_top_of_minute_alarm();
            _movement_handle_top_of_minute();
        }

        movement_event_t event;
        event.event_type = EVENT_LOW_ENERGY_UPDATE;
        event.subsecond = 0;
        watch_faces[movement_state.current_face_idx].loop(event, watch_face_contexts[movement_state.current_face_idx]);

        // If any of the previous loops requested to wake up, do it!
        if (movement_volatile_state.exit_sleep_mode) {
            movement_volatile_state.exit_sleep_mode = false;
            movement_volatile_state.is_sleeping = false;

            return;
        }

        // If we have made changes to any of the RTC comp timers, schedule the next one in the queue
        if (movement_volatile_state.schedule_next_comp) {
            movement_volatile_state.schedule_next_comp = false;
            watch_rtc_schedule_next_comp();
        }

        // otherwise enter sleep mode, until either the top of the minute interrupt or extwake wakes us up.
        watch_enter_sleep_mode();
    }
}

#endif

static bool _switch_face(void) {
    const watch_face_t *wf = &watch_faces[movement_state.current_face_idx];

    wf->resign(watch_face_contexts[movement_state.current_face_idx]);
    movement_state.current_face_idx = movement_state.next_face_idx;
    // we have just updated the face idx, so we must recache the watch face pointer.
    wf = &watch_faces[movement_state.current_face_idx];
    watch_clear_display();
    movement_request_tick_frequency(1);

    if (movement_state.settings.bit.button_should_sound) {
        // low note for nonzero case, high note for return to watch_face 0
        movement_play_note(movement_state.next_face_idx ? BUZZER_NOTE_C7 : BUZZER_NOTE_C8, 50);
    }

    wf->activate(watch_face_contexts[movement_state.current_face_idx]);

    movement_event_t event;
    event.subsecond = 0;
    event.event_type = EVENT_ACTIVATE;
    movement_state.watch_face_changed = false;
    bool can_sleep = wf->loop(event, watch_face_contexts[movement_state.current_face_idx]);

    // Button events that follow a down event that happened on the previous face should not be forwarded to the new face
    movement_volatile_state.passthrough_events = _movement_button_events_mask;

    return can_sleep;
}

bool app_loop(void) {
    const watch_face_t *wf = &watch_faces[movement_state.current_face_idx];

    // default to being allowed to sleep by the face.
    bool can_sleep = true;

    // Any events that have been added by the various interrupts in between app_loop invokations
    uint32_t pending_events = movement_volatile_state.pending_events;
    movement_volatile_state.pending_events = 0;

    movement_event_t event;
    event.event_type = EVENT_NONE;
    // Subsecond is determined by the TICK event, if concurrent events have happened,
    // they will all have the same subsecond as they should to keep backward compatibility.
    event.subsecond = movement_volatile_state.subsecond;

    // if the LED should be off, turn it off
    if (movement_volatile_state.turn_led_off) {
        // unless the user is holding down the LIGHT button, in which case, give them more time.
        if (movement_volatile_state.light_button.is_down) {
        } else {
            movement_volatile_state.turn_led_off = false;
            movement_force_led_off();
        }
    }

    if (movement_volatile_state.has_pending_accelerometer) {
        movement_volatile_state.has_pending_accelerometer = false;
        pending_events |= _movement_get_accelerometer_events();
    }

    // Deferred accelerometer wake handler: do I2C reads now that we're outside the ISR.
    if (movement_volatile_state.accelerometer_woke) {
        movement_volatile_state.accelerometer_woke = false;
        _movement_handle_accelerometer_wake();
    }

    // handle any button up/down events that occurred, e.g. schedule longpress timeouts, reset inactivity, etc.
    _movement_handle_button_presses(pending_events);

    // if we have a scheduled background task, handle that here:
    if (
        (pending_events & (1 << EVENT_TICK))
        && event.subsecond == 0
        && movement_state.has_scheduled_background_task
    ) {
        _movement_handle_scheduled_tasks();
    }

    // Pop the EVENT_TIMEOUT out of the pending_events so it can be handled separately
    bool resign_timeout = (pending_events & (1 << EVENT_TIMEOUT)) != 0;
    if (resign_timeout) {
        pending_events &= ~(1 << EVENT_TIMEOUT);
    }

    // Consume all the pending events
    uint32_t passthrough_pending_events = pending_events & movement_volatile_state.passthrough_events;
    pending_events = pending_events & ~movement_volatile_state.passthrough_events;

    movement_event_type_t event_type = 0;
    while (passthrough_pending_events) {
        uint8_t next_event = __builtin_ctz(passthrough_pending_events);
        event.event_type = event_type + next_event;
        can_sleep = movement_default_loop_handler(event) && can_sleep;
        passthrough_pending_events = passthrough_pending_events >> (next_event + 1);
        event_type = event_type + next_event + 1;
    }

    event_type = 0;
    while (pending_events) {
        uint8_t next_event = __builtin_ctz(pending_events);
        event.event_type = event_type + next_event;
        can_sleep = wf->loop(event, watch_face_contexts[movement_state.current_face_idx]) && can_sleep;
        pending_events = pending_events >> (next_event + 1);
        event_type = event_type + next_event + 1;
    }

    // handle top-of-minute tasks, if the alarm handler told us we need to
    if (movement_volatile_state.minute_alarm_fired) {
        movement_volatile_state.minute_alarm_fired = false;
        _movement_renew_top_of_minute_alarm();
        _movement_handle_top_of_minute();
    }

    // Now handle the EVENT_TIMEOUT
    if (resign_timeout && movement_state.current_face_idx != 0) {
        event.event_type = EVENT_TIMEOUT;
        can_sleep = wf->loop(event, watch_face_contexts[movement_state.current_face_idx]) && can_sleep;
    }

    // The watch_face_changed flag might be set again by the face loop, so check it again
    if (movement_state.watch_face_changed) {
        can_sleep = _switch_face() && can_sleep;
    }

#ifndef MOVEMENT_LOW_ENERGY_MODE_FORBIDDEN
    // if we have timed out of our low energy mode countdown, enter low energy mode.
    if (movement_volatile_state.enter_sleep_mode && !movement_volatile_state.is_buzzing) {
        movement_volatile_state.enter_sleep_mode = false;
        movement_volatile_state.is_sleeping = true;

        // No need to fire resign and sleep interrupts while in sleep mode
        _movement_disable_inactivity_countdown();

        watch_register_extwake_callback(HAL_GPIO_BTN_ALARM_pin(), cb_alarm_btn_extwake, true);

        // _sleep_mode_app_loop takes over at this point and loops until exit_sleep_mode is set by the extwake handler,
        // or wake is requested using the movement_request_wake function.
        _sleep_mode_app_loop();
        // as soon as _sleep_mode_app_loop returns, we prepare to reactivate

        // // this is a hack tho: waking from sleep mode, app_setup does get called, but it happens before we have reset our ticks.
        // // need to figure out if there's a better heuristic for determining how we woke up.
        app_setup();

        // If we woke up to play a note sequence, actually play the note sequence we were asked to play while in deep sleep.
        if (movement_volatile_state.has_pending_sequence) {
            movement_volatile_state.has_pending_sequence = false;
            watch_buzzer_play_sequence_with_volume(_pending_sequence, movement_request_sleep, _movement_get_buzzer_volume(movement_volatile_state.pending_sequence_priority));
            // When this sequence is done playing, movement_request_sleep is invoked and the watch will go,
            // back to sleep (unless the user interacts with it in the meantime)
            _pending_sequence = NULL;
        }

        // don't let the watch sleep when exiting deep sleep mode,
        // so that app_loop will run again and process the events that may have fired.
        can_sleep = false;
    }
#endif

    // If we have made changes to any of the RTC comp timers, schedule the next one in the queue
    if (movement_volatile_state.schedule_next_comp) {
        movement_volatile_state.schedule_next_comp = false;
        watch_rtc_schedule_next_comp();
    }

#if __EMSCRIPTEN__
    shell_task();
#else
    // if we are plugged into USB, handle the serial shell
    if (usb_is_enabled()) {
        shell_task();
    }
#endif

    // if we are plugged into USB, we can't sleep because we need to keep the serial shell running.
    if (usb_is_enabled()) {
        yield();
        can_sleep = false;
    }

    return can_sleep;
}

static movement_event_type_t _process_button_event(bool pin_level, movement_button_t* button) {
    movement_event_type_t event_type = EVENT_NONE;

    // This shouldn't happen normally
    if (pin_level == button->is_down) {
        return event_type;
    }

    uint32_t counter = watch_rtc_get_counter();

#if MOVEMENT_DEBOUNCE_TICKS
    if (
        (counter - button->up_timestamp) <= MOVEMENT_DEBOUNCE_TICKS &&
        (counter - button->down_timestamp) <= MOVEMENT_DEBOUNCE_TICKS
    ) {
        return event_type;
    }
#endif

    button->is_down = pin_level;

    if (pin_level) {
        button->down_timestamp = counter;
        event_type = button->down_event;
    } else {
#if MOVEMENT_DEBOUNCE_TICKS
        button->up_timestamp = counter;
#endif
        if ((counter - button->down_timestamp) >= MOVEMENT_REALLY_LONG_PRESS_TICKS) {
            // event_type = button->down_event + 5;
            event_type = button->down_event + 3; // TODO: swith to REALLY_LONG_UP
        } else if ((counter - button->down_timestamp) >= MOVEMENT_LONG_PRESS_TICKS) {
            event_type = button->down_event + 3;
        } else {
            event_type = button->down_event + 1;
        }
    }

    return event_type;
}

void cb_light_btn_interrupt(void) {
    bool pin_level = HAL_GPIO_BTN_LIGHT_read();

    movement_volatile_state.pending_events |= 1 << _process_button_event(pin_level, &movement_volatile_state.light_button);
}

void cb_mode_btn_interrupt(void) {
    bool pin_level = HAL_GPIO_BTN_MODE_read();

    movement_volatile_state.pending_events |= 1 << _process_button_event(pin_level, &movement_volatile_state.mode_button);
}

void cb_alarm_btn_interrupt(void) {
    bool pin_level = HAL_GPIO_BTN_ALARM_read();

    movement_volatile_state.pending_events |= 1 << _process_button_event(pin_level, &movement_volatile_state.alarm_button);
}

static movement_event_type_t _process_button_longpress_timeout(bool pin_level, movement_button_t* button) {
    if (!button->is_down) {
        return EVENT_NONE;
    }

    uint32_t counter = watch_rtc_get_counter();
    bool max_long_press = (counter - button->down_timestamp) >= MOVEMENT_MAX_LONG_PRESS_TICKS;
    bool really_long_press = (counter - button->down_timestamp) >= MOVEMENT_REALLY_LONG_PRESS_TICKS;

    if (pin_level) {
        if (max_long_press) {
            return EVENT_NONE; // no further events left to emit
        } else if (really_long_press) {
            return button->down_event + 4; // event_really_longpress
        } else {
            return button->down_event + 2; // event_longpress
        }
    } else {
    // hypotetical corner case: if the timeout fired but the pin level is actually up, we may have missed/rejected the up event, so fire it here
#if MOVEMENT_DEBOUNCE_TICKS
        // we're in a corner case, we don't know when the up actually happened.
        button->up_timestamp = button->down_timestamp;
#endif
        button->is_down = false;
        if (max_long_press) {
            // return button->down_event + 5; // event_really_long_up
            return button->down_event + 3; // event_long_up TODO: use really_long_up
        } else if (really_long_press) {
            return button->down_event + 3; // event_long_up
        } else {
            return button->down_event + 1; // event_up
        }
    }
}

void cb_light_btn_timeout_interrupt(void) {
    bool pin_level = HAL_GPIO_BTN_LIGHT_read();
    movement_button_t* button = &movement_volatile_state.light_button;

    movement_volatile_state.pending_events |= 1 << _process_button_longpress_timeout(pin_level, button);
}

void cb_mode_btn_timeout_interrupt(void) {
    bool pin_level = HAL_GPIO_BTN_MODE_read();
    movement_button_t* button = &movement_volatile_state.mode_button;

    movement_volatile_state.pending_events |= 1 << _process_button_longpress_timeout(pin_level, button);
}

void cb_alarm_btn_timeout_interrupt(void) {
    bool pin_level = HAL_GPIO_BTN_ALARM_read();
    movement_button_t* button = &movement_volatile_state.alarm_button;

    movement_volatile_state.pending_events |= 1 << _process_button_longpress_timeout(pin_level, button);
}

void cb_led_timeout_interrupt(void) {
    movement_volatile_state.turn_led_off = true;
}

void cb_resign_timeout_interrupt(void) {
    movement_volatile_state.pending_events |= 1 << EVENT_TIMEOUT;
}

void cb_sleep_timeout_interrupt(void) {
    movement_request_sleep();
}

void cb_alarm_btn_extwake(void) {
    // wake up!
    movement_request_wake();
}

void cb_minute_alarm_fired(void) {
    movement_volatile_state.minute_alarm_fired = true;

#if __EMSCRIPTEN__
    _wake_up_simulator();
#endif
}

void cb_tick(void) {
    rtc_counter_t counter = watch_rtc_get_counter();
    uint32_t freq = watch_rtc_get_frequency();
    uint32_t half_freq = freq >> 1;
    uint32_t subsecond_mask = freq - 1;
    movement_volatile_state.pending_events |= 1 << EVENT_TICK;
    movement_volatile_state.subsecond = ((counter + half_freq) & subsecond_mask) >> movement_state.tick_pern;
}

void cb_accelerometer_event(void) {
    movement_volatile_state.has_pending_accelerometer = true;
}

// Active Hours Sleep Mode Support (Stream 1: Core Logic)
// This feature prevents wrist motion from waking the display during sleep hours
// while still allowing tap-to-wake and button wake to function normally.

// Sleep Tracking Implementation (Stream 4)
// Logs orientation changes during sleep in 15-minute bins with minimal power overhead.

// Initialize sleep tracking system. Loads data from flash if available.
void sleep_tracking_init(void) {
    // Try to load existing data from flash
    sleep_tracking_load_from_flash();
    sleep_data_dirty = false;
}

// Load sleep data from flash storage
void sleep_tracking_load_from_flash(void) {
    uint8_t buffer[sizeof(sleep_data_t)];
    
    // Read from flash storage row
    if (watch_storage_read(SLEEP_STORAGE_ROW, 0, buffer, sizeof(sleep_data_t))) {
        memcpy(&sleep_data, buffer, sizeof(sleep_data_t));
        
        // Validate the loaded data
        if (sleep_data.current_index >= SLEEP_NIGHTS_STORED) {
            // Invalid data, initialize fresh
            memset(&sleep_data, 0, sizeof(sleep_data_t));
            sleep_data.current_index = 0;
        }
    } else {
        // No data in flash or read failed, initialize fresh
        memset(&sleep_data, 0, sizeof(sleep_data_t));
        sleep_data.current_index = 0;
    }
}

// Save sleep data to flash storage (batched to reduce write cycles)
void sleep_tracking_save_to_flash(void) {
    if (!sleep_data_dirty) {
        return;  // No changes to save
    }
    
    // Erase the row first (required before write)
    watch_storage_erase(SLEEP_STORAGE_ROW);
    
    // Write the data
    watch_storage_write(SLEEP_STORAGE_ROW, 0, (const uint8_t *)&sleep_data, sizeof(sleep_data_t));
    
    // Wait for write to complete
    watch_storage_sync();
    
    sleep_data_dirty = false;
}

// Get which 15-minute bin we're currently in (0-31, or 255 if not in sleep window)
uint8_t sleep_tracking_get_current_bin(void) {
    watch_date_time_t now = watch_rtc_get_date_time();
    uint8_t hour = now.unit.hour;
    uint8_t minute = now.unit.minute;

    // Sleep end hour is user-configurable: active hours start = wake-up time (BKUP[2])
    movement_active_hours_t active_hours = movement_get_active_hours();
    uint8_t sleep_end_hour = active_hours.bit.start_quarter_hours / 4;

    // Sleep window runs from SLEEP_START_HOUR until the configured wake-up hour
    int bin = -1;
    
    if (hour >= SLEEP_START_HOUR) {
        // Evening portion (e.g. 23:00-23:59) -> bins 0-3
        bin = (hour - SLEEP_START_HOUR) * 4 + (minute / SLEEP_BIN_MINUTES);
    } else if (hour < sleep_end_hour) {
        // Post-midnight portion (e.g. 00:00 to wake hour) -> bins 4+
        bin = ((24 - SLEEP_START_HOUR) + hour) * 4 + (minute / SLEEP_BIN_MINUTES);
    }
    
    if (bin < 0 || bin >= SLEEP_BINS_PER_NIGHT) {
        return 255;  // Not in sleep window
    }
    
    return (uint8_t)bin;
}

// Get orientation bits from a specific bin (0-31)
static uint8_t get_bin_orientation(const sleep_night_t *night, uint8_t bin) {
    if (bin >= SLEEP_BINS_PER_NIGHT) {
        return SLEEP_ORIENTATION_UNKNOWN;
    }
    
    uint8_t byte_index = bin / 4;  // 4 bins per byte
    uint8_t bit_offset = (bin % 4) * 2;  // 2 bits per bin
    
    uint8_t value = (night->night_data[byte_index] >> bit_offset) & 0x03;
    return value;
}

// Set orientation bits for a specific bin (0-31)
static void set_bin_orientation(sleep_night_t *night, uint8_t bin, uint8_t orientation) {
    if (bin >= SLEEP_BINS_PER_NIGHT || orientation > 3) {
        return;
    }
    
    uint8_t byte_index = bin / 4;
    uint8_t bit_offset = (bin % 4) * 2;
    
    // Clear the 2 bits
    night->night_data[byte_index] &= ~(0x03 << bit_offset);
    // Set the new value
    night->night_data[byte_index] |= (orientation << bit_offset);
}

// Encode date as 16-bit value: ((year-2024) << 9) | (month << 5) | day
// dt.unit.year is offset from 2020 (0=2020, 4=2024, 63=2083), NOT an absolute year.
static uint16_t encode_date(watch_date_time_t dt) {
    uint16_t year_offset = (dt.unit.year > 4) ? (dt.unit.year - 4) : 0;
    return (year_offset << 9) | (dt.unit.month << 5) | dt.unit.day;
}

// Check if we need to start a new night (date changed or first entry)
static void check_and_start_new_night(void) {
    watch_date_time_t now = watch_rtc_get_date_time();
    uint16_t today_code = encode_date(now);
    
    sleep_night_t *current_night = &sleep_data.nights[sleep_data.current_index];
    
    // If date code doesn't match, we need to start a new night
    if (current_night->date_code != today_code) {
        // Move to next night in circular buffer
        sleep_data.current_index = (sleep_data.current_index + 1) % SLEEP_NIGHTS_STORED;
        current_night = &sleep_data.nights[sleep_data.current_index];
        
        // Clear the new night's data
        memset(current_night->night_data, 0, SLEEP_BYTES_PER_NIGHT);
        current_night->date_code = today_code;
        
        sleep_data_dirty = true;
    }
}

// Get 6D orientation from accelerometer
static uint8_t get_current_orientation(void) {
#ifdef I2C_SERCOM
    /* Read SIXD_SRC register directly; driver has no lis2dw_get_6d_source() wrapper */
    uint8_t source = watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_SIXD_SRC);
    if (source & LIS2DW_WAKE_UP_SRC_VAL_ZH) {
        return SLEEP_ORIENTATION_FACE_UP;
    } else if (source & LIS2DW_WAKE_UP_SRC_VAL_ZL) {
        return SLEEP_ORIENTATION_FACE_DOWN;
    } else {
        return SLEEP_ORIENTATION_TILTED;
    }
#else
    /* No accelerometer on boards without I2C (e.g. sensorwatch_red) */
    return SLEEP_ORIENTATION_UNKNOWN;
#endif
}

// Log an orientation change during sleep
void sleep_tracking_log_orientation(uint8_t orientation) {
    // Only track if we have an accelerometer
    if (!movement_state.has_lis2dw) {
        return;
    }
    
    // Get current bin
    uint8_t bin = sleep_tracking_get_current_bin();
    if (bin == 255) {
        return;  // Not in sleep window
    }
    
    // Make sure we're tracking the right night
    check_and_start_new_night();
    
    // Update the bin
    sleep_night_t *current_night = &sleep_data.nights[sleep_data.current_index];
    set_bin_orientation(current_night, bin, orientation);
    
    sleep_data_dirty = true;
}

// Get sleep data for a specific night (0 = tonight, 1 = last night, etc.)
bool sleep_tracking_get_night_data(uint8_t days_ago, sleep_night_t *out_night) {
    if (days_ago >= SLEEP_NIGHTS_STORED || out_night == NULL) {
        return false;
    }
    
    // Calculate index in circular buffer
    int index = sleep_data.current_index - days_ago;
    if (index < 0) {
        index += SLEEP_NIGHTS_STORED;
    }
    
    memcpy(out_night, &sleep_data.nights[index], sizeof(sleep_night_t));
    
    // Return true only if this night has data (date_code != 0)
    return (out_night->date_code != 0);
}

// Count orientation changes for a given night (restlessness metric)
uint8_t sleep_tracking_count_orientation_changes(const sleep_night_t *night) {
    uint8_t changes = 0;
    uint8_t prev_orientation = SLEEP_ORIENTATION_UNKNOWN;
    
    for (uint8_t bin = 0; bin < SLEEP_BINS_PER_NIGHT; bin++) {
        uint8_t orientation = get_bin_orientation(night, bin);
        
        if (orientation != SLEEP_ORIENTATION_UNKNOWN && 
            prev_orientation != SLEEP_ORIENTATION_UNKNOWN &&
            orientation != prev_orientation) {
            changes++;
        }
        
        if (orientation != SLEEP_ORIENTATION_UNKNOWN) {
            prev_orientation = orientation;
        }
    }
    
    return changes;
}

// Check if current time is outside active hours window.
// Active hours are configurable via BKUP[2] register.
// Returns true if outside active hours window (i.e., in sleep window).
static bool is_sleep_window(void) {
    // Get active hours settings from BKUP[2]
    movement_active_hours_t settings = movement_get_active_hours();
    
    // If active hours are disabled, never consider it sleep time
    if (!settings.bit.enabled) {
        return false;
    }
    
    // Convert current time to quarter-hours (15-minute increments)
    watch_date_time_t now = watch_rtc_get_date_time();
    uint8_t current_quarter_hours = (now.unit.hour * 4) + (now.unit.minute / 15);
    
    uint8_t start = settings.bit.start_quarter_hours;
    uint8_t end = settings.bit.end_quarter_hours;
    
    // Check if current time is in sleep window (outside active hours)
    if (start < end) {
        // Active hours don't wrap midnight (e.g., 08:00-22:00)
        // Sleep window is before start OR after end
        return (current_quarter_hours < start || current_quarter_hours >= end);
    } else if (start > end) {
        // Active hours wrap midnight (e.g., 22:00-08:00)
        // Sleep window is after end AND before start
        return (current_quarter_hours >= end && current_quarter_hours < start);
    } else {
        // start == end means 24-hour active (no sleep window)
        return false;
    }
}

// Check if wearer is confirmed to be asleep.
// Returns true only if BOTH conditions are met:
// 1. Current time is in sleep window (outside active hours)
// 2. Accelerometer reports stationary/sleep state
// This prevents false positives from just lying still during the day.
static bool is_confirmed_asleep(void) {
    // First check: must be in sleep window
    if (!is_sleep_window()) {
        return false;
    }
    
    // Second check: accelerometer must confirm stationary state
    // Only check if we have an accelerometer
    if (!movement_state.has_lis2dw) {
        // No accelerometer: fall back to time-only check
        return true;
    }
    
    // Read accelerometer sleep state from wakeup source register.
    // INT2 is configured to report sleep state changes.
    // NOTE: LIS2DW_WAKEUP_SRC_SLEEP_STATE is set BEFORE a wakeup event fires,
    // so at the time we read this register the bit is already 0 (cleared by
    // the wake transition).  A value of 0 therefore means the device just woke
    // from sleep — i.e. the wearer was stationary / asleep.
    lis2dw_wakeup_source_t wakeup_src = lis2dw_get_wakeup_source();
    bool is_stationary = (wakeup_src & LIS2DW_WAKEUP_SRC_SLEEP_STATE) == 0;  // inverted: 0 == was sleeping

    return is_stationary;
}

// Smart Alarm Pre-Wake Detection Support (Stream 3)
// These functions enable the smart alarm to wake the user during light sleep
// within a configured time window, improving wake quality.

// Global state for smart alarm tracking
// This tracks motion events during the pre-wake monitoring period
static struct {
    uint8_t motion_event_count;     // Count of motion events in tracking window
    uint8_t orientation_changes;    // Count of orientation changes
    uint32_t last_motion_time;      // Timestamp of last motion event (minutes of day, 0-1439)
    bool tracking_active;           // Whether we're actively monitoring for light sleep
} smart_alarm_tracking = {0, 0, 0, false};

// Get smart alarm configuration from BKUP[3] register
// Returns true if smart alarm is enabled, false otherwise
// Outputs window_start and window_end in 15-minute increments (0-95)
static bool get_smart_alarm_config(uint8_t *window_start, uint8_t *window_end) {
    uint32_t config = watch_get_backup_data(3);

    
    *window_start = (config >> 0) & 0x7F;   // Bits 0-6
    *window_end = (config >> 7) & 0x7F;     // Bits 7-13
    bool enabled = (config >> 14) & 0x01;   // Bit 14
    
    return enabled;
}

// Check if current time is approaching the alarm window (15 minutes before start).
// Returns true to trigger accelerometer ramp-up for light sleep detection.
static bool is_approaching_alarm_window(void) {
    uint8_t window_start, window_end;
    
    // Check if smart alarm is enabled
    if (!get_smart_alarm_config(&window_start, &window_end)) {
        return false;
    }
    
    // Get current time and convert to 15-minute increment index
    watch_date_time_t now = watch_rtc_get_date_time();
    uint8_t current_increment = (now.unit.hour * 4) + (now.unit.minute / 15);
    
    // Calculate 15 minutes before window start
    // Handle wraparound (e.g., window at 00:00 means pre-wake at 23:45)
    uint8_t prewake_increment = (window_start > 0) ? (window_start - 1) : 95;
    
    // We're approaching if we're at the pre-wake time
    // or anywhere between pre-wake and window end
    bool in_prewake_window = false;
    if (prewake_increment < window_end) {
        // Normal case: window doesn't wrap midnight
        in_prewake_window = (current_increment >= prewake_increment && 
                             current_increment <= window_end);
    } else {
        // Window wraps midnight
        in_prewake_window = (current_increment >= prewake_increment || 
                             current_increment <= window_end);
    }
    
    return in_prewake_window;
}

// Check if light sleep is detected based on accelerometer activity.
// Light sleep is characterized by increased motion and orientation changes
// compared to deep sleep baseline.
// Returns true if light sleep indicators are present.
static bool is_light_sleep_detected(void) {
    // Only valid if we have an accelerometer and tracking is active
    if (!movement_state.has_lis2dw || !smart_alarm_tracking.tracking_active) {
        return false;
    }
    
    // Light sleep detection criteria:
    // 1. At least 2 motion events in the last 5 minutes (indicates restlessness)
    // 2. OR at least 1 orientation change (indicates position shift)
    
    // Get current time as minutes-of-day (0-1439)
    watch_date_time_t now = watch_rtc_get_date_time();
    uint32_t current_minutes = now.unit.hour * 60 + now.unit.minute;

    // Check if we have recent motion (within last 5 minutes).
    // Use modular arithmetic to handle midnight rollover (e.g., current=0:05, last=23:55 → 10 min).
    uint32_t minutes_since_motion = ((current_minutes - smart_alarm_tracking.last_motion_time + 1440) % 1440);
    bool recent_motion = (minutes_since_motion < 5);
    
    // Light sleep detected if:
    // - Multiple motion events AND recent activity
    // - OR orientation changes detected
    bool light_sleep = (smart_alarm_tracking.motion_event_count >= 2 && recent_motion) ||
                       (smart_alarm_tracking.orientation_changes >= 1);
    
    return light_sleep;
}

// Reset smart alarm tracking state
// Called when alarm fires or user wakes naturally
static void reset_smart_alarm_tracking(void) {
    smart_alarm_tracking.motion_event_count = 0;
    smart_alarm_tracking.orientation_changes = 0;
    smart_alarm_tracking.last_motion_time = 0;
    smart_alarm_tracking.tracking_active = false;
}

// Update smart alarm tracking with new motion event
// Called from the deferred wake handler (not ISR)
static void update_smart_alarm_tracking(bool is_orientation_change) {
    if (!smart_alarm_tracking.tracking_active) {
        return;
    }
    
    // Increment motion event counter
    smart_alarm_tracking.motion_event_count++;
    
    // Track orientation changes separately
    if (is_orientation_change) {
        smart_alarm_tracking.orientation_changes++;
    }
    
    // Update timestamp as minutes-of-day (matches is_light_sleep_detected rollover math)
    watch_date_time_t now = watch_rtc_get_date_time();
    smart_alarm_tracking.last_motion_time = now.unit.hour * 60 + now.unit.minute;
}

// Deferred handler for accelerometer wake events.
// Must NOT be called from an ISR — it performs blocking I2C reads.
// Called by app_loop on the next tick after accelerometer_woke is set.
static void _movement_handle_accelerometer_wake(void) {
    // Check interrupt source via I2C (safe here — we are in the main loop).
    lis2dw_interrupt_source_t int_src = lis2dw_get_interrupt_source();

    bool is_tap = (int_src & (LIS2DW_INTERRUPT_SRC_DOUBLE_TAP | LIS2DW_INTERRUPT_SRC_SINGLE_TAP)) != 0;
    bool is_orientation_change = (int_src & LIS2DW_INTERRUPT_SRC_6D) != 0;

    if (is_tap) {
        // This was a tap - set pending accelerometer flag for event processing
        movement_volatile_state.has_pending_accelerometer = true;
    }

    // Smart Alarm Pre-Wake Monitoring (Stream 3)
    // If we're approaching the alarm window, activate tracking and monitor for light sleep
    if (is_approaching_alarm_window()) {
        // Activate tracking if not already active
        if (!smart_alarm_tracking.tracking_active) {
            smart_alarm_tracking.tracking_active = true;
        }

        // Update tracking with this motion event
        update_smart_alarm_tracking(is_orientation_change);

        // Check if light sleep is detected
        if (is_light_sleep_detected()) {
            // Trigger alarm immediately — light sleep detected within window.
            // Dispatch EVENT_BACKGROUND_TASK directly to the smart alarm face
            // so its own handler (not the generic minute-alarm path) fires the alarm.
            movement_event_t bg_event = { EVENT_BACKGROUND_TASK, 0 };
            for (uint8_t fi = 0; fi < MOVEMENT_NUM_FACES; fi++) {
                if (watch_faces[fi].loop == smart_alarm_face_loop) {
                    watch_faces[fi].loop(bg_event, watch_face_contexts[fi]);
                    break;
                }
            }
            reset_smart_alarm_tracking();
        }
    } else {
        // Not in alarm window — reset tracking if it was active
        if (smart_alarm_tracking.tracking_active) {
            reset_smart_alarm_tracking();
        }
    }

    // Active Hours Sleep Mode: Suppress motion wake during confirmed sleep.
    // This prevents wrist rolls from waking the display at night while still
    // allowing tap-to-wake (INT1/A3) and button wake to function normally.
    // Motion wake only suppressed when BOTH time window and accelerometer agree.
    // Exception: Don't suppress during smart alarm pre-wake window (let tracking happen).
    if (is_confirmed_asleep() && !is_approaching_alarm_window()) {
        // Stream 4: Log orientation changes during sleep
        uint8_t orientation = get_current_orientation();
        sleep_tracking_log_orientation(orientation);

        // We're in confirmed sleep — only process tap events, ignore motion.
        return;
    }

    // Not in sleep mode (or in alarm pre-wake window): normal behavior — wake on any motion.
    movement_volatile_state.pending_events |= 1 << EVENT_ACCELEROMETER_WAKE;
    _movement_reset_inactivity_countdown();
}

void cb_accelerometer_wake(void) {
    // ISR — keep this minimal.  No I2C reads allowed inside an interrupt handler.
    // Set a flag; the real work is deferred to _movement_handle_accelerometer_wake()
    // which is called from app_loop on the next tick.
    movement_volatile_state.accelerometer_woke = true;
}
