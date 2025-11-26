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

#pragma once

#include <stdio.h>
#include <stdbool.h>
#include "watch.h"
#include "utz.h"
#include "lis2dw.h"

/// @brief A struct that allows a watch face to report its state back to Movement.
typedef struct {
    uint8_t wants_background_task: 1;
    uint8_t has_active_alarm: 1;
    uint8_t responds_to_dst_change: 1;
} movement_watch_face_advisory_t;

// Movement Preferences
// These four 32-bit structs store information about the wearer and their preferences. Tentatively, the plan is
// for Movement to use four 32-bit registers for these preferences and to store them in the RTC's backup registers
// 0-3, leaving registers 4-7 available for third party watch faces to use as they see fit.
// * The movement_settings_t struct is provided to all watch faces in the callback functions, and is stored in the
//   RTC's first backup register (BKUP[0]).
// * The movement_location_t and movement_birthdate_t types are defined here, and are tentatively meant to be
//   stored in BKUP[1] and BKUP[2], respectively.
// * The movement_reserved_t type is here as a placeholder, because I sense there's some other generally useful
//   stuff we'll want to make available to all watch faces and stash in the BKUP[3] register.
// This allows these preferences to be stored before entering BACKUP mode and and restored after waking from reset.

typedef enum {
    MOVEMENT_CLOCK_MODE_12H = 0,    /// use 12 hour clock
    MOVEMENT_CLOCK_MODE_24H,        /// use 24 hour clock
    MOVEMENT_CLOCK_MODE_024H,       /// use 24 hour clock with leading zero
    MOVEMENT_NUM_CLOCK_MODES
} movement_clock_mode_t;

/// struct for Movement LED color
typedef struct {
    uint8_t red : 4;
    uint8_t green : 4;
    uint8_t blue : 4;
} movement_color_t;

// movement_settings_t contains global settings that cover watch behavior, including preferences around clock and unit
// display, time zones, buzzer behavior, LED color and low energy mode timeouts.
typedef union {
    struct {
        uint8_t version : 2;                // A version number for the struct. 0-3; let's try not to update too often.
        bool button_should_sound : 1;       // if true, pressing a button emits a sound.
        uint8_t to_interval : 2;            // an inactivity interval for asking the active face to resign.
        uint8_t le_interval : 3;            // 0 to disable low energy mode, or an inactivity interval for going into low energy mode.
        uint8_t led_duration : 3;           // how many seconds to shine the LED for (x2), 0 to shine only while the button is depressed, or all bits set to disable the LED altogether.
        uint8_t led_red_color : 4;          // for general purpose illumination, the red LED value (0-15)
        uint8_t led_green_color : 4;        // for general purpose illumination, the green LED value (0-15)
        uint8_t led_blue_color : 4;         // for general purpose illumination, the green LED value (0-15)
        uint8_t time_zone : 6;              // an integer representing an index in the time zone table.

        // while Movement itself doesn't implement a clock or display units, it may make sense to include some
        // global settings for watch faces to check. The 12/24 hour preference could inform a clock or a
        // time-oriented complication like a sunrise/sunset timer, and a simple locale preference could tell an
        // altimeter to display feet or meters as easily as it tells a thermometer to display degrees in F or C.
        bool clock_mode_24h : 1;            // indicates whether clock should use 12 or 24 hour mode.
        bool use_imperial_units : 1;        // indicates whether to use metric units (the default) or imperial.
        
        bool button_volume : 1;             // 0 for soft beep, 1 for loud beep. If button_should_sound (above) is false, this is ignored.
    } bit;
    uint32_t reg;
} movement_settings_t;

// movement_location_t is for storing the wearer's location. This will be useful for astronomical calculations such as
// sunrise and sunset, or predictions of visible satellite passes.
// If you create a UI for this register or need to access it, look for it in the RTC's BKUP[1] register.
typedef union {
    struct {
        int16_t latitude : 16;   // signed latutide in hundredths of a degree
        int16_t longitude : 16;  // signed longitude in hundredths of a degree
    } bit;
    uint32_t reg;
} movement_location_t;

// movement_reserved_t is a placeholder for future use of the BKUP[3] register.
typedef union {
    struct {
        uint32_t reserved : 32;
    } bit;
    uint32_t reg;
} movement_reserved_t;

typedef enum {
    EVENT_NONE = 0,             // There is no event to report.
    EVENT_ACTIVATE,             // Your watch face is entering the foreground.
    EVENT_TICK,                 // Most common event type. Your watch face is being called from the tick callback.
    EVENT_LOW_ENERGY_UPDATE,    // If the watch is in low energy mode and you are in the foreground, you will get a chance to update the display once per minute.
    EVENT_BACKGROUND_TASK,      // Your watch face is being invoked to perform a background task. Don't update the display here; you may not be in the foreground.
    EVENT_TIMEOUT,              // Your watch face has been inactive for a while. You may want to resign, depending on your watch face's intended use case.

    EVENT_LIGHT_BUTTON_DOWN,    // The light button has been pressed, but not yet released.
    EVENT_LIGHT_BUTTON_UP,      // The light button was pressed for less than half a second, and released.
    EVENT_LIGHT_LONG_PRESS,     // The light button was held for over half a second, but not yet released.
    EVENT_LIGHT_LONG_UP,        // The light button was held for over half a second, and released.
    EVENT_LIGHT_REALLY_LONG_PRESS, // The light button was held for more than 1.5 second, note yet released.
    // EVENT_LIGHT_REALLY_LONG_UP, // The light button was held for more than 1.5 second, and released.
    EVENT_MODE_BUTTON_DOWN,     // The mode button has been pressed, but not yet released.
    EVENT_MODE_BUTTON_UP,       // The mode button was pressed for less than half a second, and released.
    EVENT_MODE_LONG_PRESS,      // The mode button was held for over half a second, but not yet released.
    EVENT_MODE_LONG_UP,         // The mode button was held for over half a second, and released. NOTE: your watch face will resign immediately after receiving this event.
    EVENT_MODE_REALLY_LONG_PRESS, // The mode button was held for more than 1.5 second, note yet released.
    // EVENT_MODE_REALLY_LONG_UP, // The mode button was held for more than 1.5 second, and released.
    EVENT_ALARM_BUTTON_DOWN,    // The alarm button has been pressed, but not yet released.
    EVENT_ALARM_BUTTON_UP,      // The alarm button was pressed for less than half a second, and released.
    EVENT_ALARM_LONG_PRESS,     // The alarm button was held for over half a second, but not yet released.
    EVENT_ALARM_LONG_UP,        // The alarm button was held for over half a second, and released.
    EVENT_ALARM_REALLY_LONG_PRESS, // The alarm button was held for more than 1.5 second, note yet released.
    // EVENT_ALARM_REALLY_LONG_UP, // The alarm button was held for more than 1.5 second, and released.

    EVENT_ACCELEROMETER_WAKE,   // The accelerometer has detected motion and woken up.
    EVENT_SINGLE_TAP,           // Accelerometer detected a single tap. This event is not yet implemented.
    EVENT_DOUBLE_TAP,           // Accelerometer detected a double tap. This event is not yet implemented.
} movement_event_type_t;

// Each different timeout type will use a different index when invoking watch_rtc_register_comp_callback
typedef enum {
    LIGHT_BUTTON_TIMEOUT = 0,   // Light button longpress timeout
    MODE_BUTTON_TIMEOUT,        // Mode button longpress timeout
    ALARM_BUTTON_TIMEOUT,       // Alarm button longpress timeout
    LED_TIMEOUT,                // LED off timeout
    RESIGN_TIMEOUT,             // Resign active face timeout
    SLEEP_TIMEOUT,              // Low-energy begin timeout
    MINUTE_TIMEOUT,             // Top of the Minute timeout
} movement_timeout_index_t;

typedef enum {
    BUZZER_PRIORITY_BUTTON = 0, // Buzzer priority for button beeps (lowest priority).
    BUZZER_PRIORITY_SIGNAL,     // Buzzer priority for hourly chime (medium priority).
    BUZZER_PRIORITY_ALARM,      // Buzzer priority for hourly chime (highest priority).
} movement_buzzer_priority_t;

typedef struct {
    uint8_t event_type;
    uint8_t subsecond;
} movement_event_t;

extern const int16_t movement_timezone_offsets[];

/** @brief Perform setup for your watch face.
  * @details It's tempting to say this is 'one-time' setup, but technically this function is called more than
  *          once. When the watch first boots, this function is called with a NULL context_ptr, indicating
  *          that it is the first run. At this time you should set context_ptr to something non-NULL if you
  *          need to keep track of any state in your watch face. If your watch face requires any other setup,
  *          like configuring a pin mode or a peripheral, you may want to do that here too.
  *          This function will be called again after waking from sleep mode, since sleep mode disables all
  *          of the device's pins and peripherals.
  * @param watch_face_index The index of this watch face in the global array of watch faces; 0 is the first face,
  *                         1 is the second, etc. You may stash this value in your context if you wish to reference
  *                         it later; your watch face's index is set at launch and will not change.
  * @param context_ptr A pointer to a pointer; at first invocation, this value will be NULL, and you can set it
  *                    to any value you like. Subsequent invocations will pass in whatever value you previously
  *                    set. You may want to check if this is NULL and if so, allocate some space to store any
  *                    data required for your watch face.
  *
  */
typedef void (*watch_face_setup)(uint8_t watch_face_index, void ** context_ptr);

/** @brief Prepare to go on-screen.
  * @details This function is called just before your watch enters the foreground. If your watch face has any
  *          segments or text that is always displayed, you may want to set that here. In addition, if your
  *          watch face depends on data from a peripheral (like an I2C sensor), you will likely want to enable
  *          that peripheral here. In addition, if your watch face requires an update frequncy other than 1 Hz,
  *          you may want to request that here using the movement_request_tick_frequency function.
  * @param context A pointer to your watch face's context. @see watch_face_setup.
  *
  */
typedef void (*watch_face_activate)(void *context);

/** @brief Handle events and update the display.
  * @details This function is called in response to an event. You should set up a switch statement that handles,
  *          at the very least, the EVENT_TICK and EVENT_MODE_BUTTON_UP event types. The tick event happens once
  *          per second (or more frequently if you asked for a faster tick with movement_request_tick_frequency).
  *          The mode button up event occurs when the user presses the MODE button. **Your loop function SHOULD
  *          call the movement_move_to_next_face function in response to this event.** If you have a good reason
  *          to override this behavior (e.g. your user interface requires all three buttons), your watch face MUST
  *          call the movement_move_to_next_face function in response to the EVENT_MODE_LONG_PRESS event. If you
  *          fail to do this, the user will become stuck on your watch face.
  * @param event A struct containing information about the event, including its type. @see movement_event_type_t
  *              for a list of all possible event types.
  * @param context A pointer to your application's context. @see watch_face_setup.
  * @return true if your watch face is prepared for the system to enter STANDBY mode; false to keep the system awake.
  *         You should almost always return true.
  *         Note that this return value has no effect if your loop function has called movement_move_to_next_face
  *         or movement_move_to_face; in that case, your watch face will resign immediately, and the next watch
  *         face will make the decision on entering standby mode.
  * @note There are two event types that require some extra thought:
          The EVENT_LOW_ENERGY_UPDATE event type is a special case. If you are in the foreground when the watch
          goes into low energy mode, you will receive this tick once a minute (at the top of the minute) so that
          you can update the screen. Great! But! When you receive this event, all pins and peripherals other than
          the RTC will have been disabled to save energy. If your display is clock or calendar oriented, this is
          fine. But if your display requires polling an I2C sensor or reading a value with the ADC, you won't be
          able to do this. You should either display the name of the watch face in response to the low power tick,
          or ensure that you resign before low power mode triggers, (e.g. by calling movement_move_to_face(0)).
          **Your watch face MUST NOT wake up peripherals in response to a low power tick.** The purpose of this
          mode is to consume as little energy as possible during the (potentially long) intervals when it's
          unlikely the user is wearing or looking at the watch.
          EVENT_BACKGROUND_TASK is also a special case. @see watch_face_advise for details.
  */
typedef bool (*watch_face_loop)(movement_event_t event, void *context);

/** @brief Prepare to go off-screen.
  * @details This function is called before your watch face enters the background. If you requested a tick
  *          frequency other than the standard 1 Hz, **you must call movement_request_tick_frequency(1) here**
  *          to reset to 1 Hz. You should also disable any peripherals you enabled when you entered the foreground.
  * @param context A pointer to your application's context. @see watch_face_setup.
  */
typedef void (*watch_face_resign)(void *context);

/** @brief OPTIONAL. Request an opportunity to run a background task.
  * @details Most apps will not need this function, but if you provide it, Movement will call it once per minute in
  *          both active and low power modes, regardless of whether your app is in the foreground. You can check the
  *          current time to determine whether you require a background task. If you return true here, Movement will
  *          immediately call your loop function with an EVENT_BACKGROUND_TASK event. Note that it will not call your
  *          activate or deactivate functions, since you are not going on screen.
  *
  *          Examples of background tasks:
  *           - Wake and play a sound when an alarm or timer has been triggered.
  *           - Check the state of an RTC interrupt pin or the timestamp of an RTC interrupt event.
  *           - Log a data point from a sensor, and then return to sleep mode.
  *
  *          Guidelines for background tasks:
  *           - Assume all peripherals and pins other than the RTC will be disabled when you get an EVENT_BACKGROUND_TASK.
  *           - Even if your background task involves only the RTC peripheral, try to request background tasks sparingly.
  *           - If your background task involves an external pin or peripheral, request background tasks no more than once per hour.
  *           - If you need to enable a pin or a peripheral to perform your task, return it to its original state afterwards.
  *
  * @param context A pointer to your application's context. @see watch_face_setup.
  * @return true to request a background task; false otherwise.
  */
typedef movement_watch_face_advisory_t (*watch_face_advise)(void *context);

typedef struct {
    watch_face_setup setup;
    watch_face_activate activate;
    watch_face_loop loop;
    watch_face_resign resign;
    watch_face_advise advise;
} watch_face_t;

typedef struct {
    movement_settings_t settings;

    // transient properties
    int16_t current_face_idx;
    int16_t next_face_idx;
    bool watch_face_changed;

    // LED stuff
    bool light_on;

    // background task handling
    bool has_scheduled_background_task;

    // stuff for subsecond tracking
    uint8_t tick_frequency;
    uint8_t tick_pern;

    // backup register stuff
    uint8_t next_available_backup_register;

    // temporary alarm enabled boolean, until we implement this in advisories
    bool alarm_enabled;

    // boolean set if thermistor is detected
    bool has_thermistor;

    // boolean set if accelerometer is detected
    bool has_lis2dw;
    // data rate for background accelerometer sensing
    lis2dw_data_rate_t accelerometer_background_rate;
    // threshold for considering the wearer is in motion
    uint8_t accelerometer_motion_threshold;

    // signal and alarm volumes
    watch_buzzer_volume_t signal_volume;
    watch_buzzer_volume_t alarm_volume;
} movement_state_t;

void movement_move_to_face(uint8_t watch_face_index);
void movement_move_to_next_face(void);

bool movement_default_loop_handler(movement_event_t event);

void movement_illuminate_led(void);
void movement_force_led_on(uint8_t red, uint8_t green, uint8_t blue);
void movement_force_led_off(void);

void movement_request_tick_frequency(uint8_t freq);

// note: watch faces can only schedule a background task when in the foreground, since
// movement will associate the scheduled task with the currently active face.
void movement_schedule_background_task(watch_date_time_t date_time);

// note: watch faces can only cancel their background task when in the foreground, since
// movement will associate the scheduled task with the currently active face.
void movement_cancel_background_task(void);

// these functions should work around the limitation of the above functions, which will be deprecated.
void movement_schedule_background_task_for_face(uint8_t watch_face_index, watch_date_time_t date_time);
void movement_cancel_background_task_for_face(uint8_t watch_face_index);

void movement_request_sleep(void);
void movement_request_wake(void);

void movement_play_note(watch_buzzer_note_t note, uint16_t duration_ms);
void movement_play_signal(void);
void movement_play_alarm(void);
void movement_play_alarm_beeps(uint8_t rounds, watch_buzzer_note_t alarm_note);
void movement_play_sequence(int8_t *note_sequence, movement_buzzer_priority_t priority);

uint8_t movement_claim_backup_register(void);

int32_t movement_get_current_timezone_offset_for_zone(uint8_t zone_index);
int32_t movement_get_current_timezone_offset(void);

int32_t movement_get_timezone_index(void);
void movement_set_timezone_index(uint8_t value);

watch_date_time_t movement_get_utc_date_time(void);
watch_date_time_t movement_get_local_date_time(void);
watch_date_time_t movement_get_date_time_in_zone(uint8_t zone_index);
uint32_t movement_get_utc_timestamp(void);

void movement_set_utc_date_time(watch_date_time_t date_time);
void movement_set_local_date_time(watch_date_time_t date_time);
void movement_set_utc_timestamp(uint32_t timestamp);

bool movement_button_should_sound(void);
void movement_set_button_should_sound(bool value);

watch_buzzer_volume_t movement_button_volume(void);
void movement_set_button_volume(watch_buzzer_volume_t value);

watch_buzzer_volume_t movement_signal_volume(void);
void movement_set_signal_volume(watch_buzzer_volume_t value);

watch_buzzer_volume_t movement_alarm_volume(void);
void movement_set_alarm_volume(watch_buzzer_volume_t value);

movement_clock_mode_t movement_clock_mode_24h(void);
void movement_set_clock_mode_24h(movement_clock_mode_t value);

bool movement_use_imperial_units(void);
void movement_set_use_imperial_units(bool value);

uint8_t movement_get_fast_tick_timeout(void);
void movement_set_fast_tick_timeout(uint8_t value);

uint8_t movement_get_low_energy_timeout(void);
void movement_set_low_energy_timeout(uint8_t value);

movement_color_t movement_backlight_color(void);
void movement_set_backlight_color(movement_color_t color);

uint8_t movement_get_backlight_dwell(void);
void movement_set_backlight_dwell(uint8_t value);

void movement_store_settings(void);

/// TODO: For #SecondMovement: Should we have a counter that watch faces increment when they enable an alarm, and decrement when they disable it?
/// Or should there be a watch face function where watch faces can tell us if they have an alarm enabled?
/// Worth considering a better way to handle this.
bool movement_alarm_enabled(void);
void movement_set_alarm_enabled(bool value);

// if the board has an accelerometer, these functions will enable or disable tap detection.
bool movement_enable_tap_detection_if_available(void);
bool movement_disable_tap_detection_if_available(void);

// gets and sets the accelerometer data rate in the background
lis2dw_data_rate_t movement_get_accelerometer_background_rate(void);
bool movement_set_accelerometer_background_rate(lis2dw_data_rate_t new_rate);

// gets and sets the accelerometer motion threshold
uint8_t movement_get_accelerometer_motion_threshold(void);
bool movement_set_accelerometer_motion_threshold(uint8_t new_threshold);

// If the board has a temperature sensor, this function will give you the temperature in degrees celsius.
// If the board has multiple temperature sensors, it will use the most accurate one available.
// If the board has no temperature sensors, it will return 0xFFFFFFFF.
float movement_get_temperature(void);
