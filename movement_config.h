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

#ifndef MOVEMENT_CONFIG_H_
#define MOVEMENT_CONFIG_H_

/* ===== EXPERIMENTAL FEATURES ===== */

/* Phase Engine: Context-aware circadian rhythm tracking
 * 
 * Enables the Phase Watch system - real-time circadian alignment scoring
 * based on time of day, season, activity, temperature, and light exposure.
 * 
 * Flash impact: ~12 KB (homebase table + computation logic)
 * RAM impact: ~64 bytes (phase state)
 * 
 * See lib/phase/README.md for documentation.
 * 
 * NOTE: Disabled by default for backward compatibility.
 *       Enable via Makefile: make BOARD=... DISPLAY=... PHASE_ENGINE_ENABLED=1
 *       Or uncomment below:
 */
// #define PHASE_ENGINE_ENABLED

/* ===== END EXPERIMENTAL FEATURES ===== */

#include "movement_faces.h"

const watch_face_t watch_faces[] = {
    // PRIMARY FACES (MODE cycles through 0 → 13, skipping 2-5)
    wyoscan_face,               // 0: Always visible
    clock_face,                 // 1: Main clock (long-press ALARM → playlist)
    
#ifdef PHASE_ENGINE_ENABLED
    // TERTIARY (Zone faces - skipped in MODE rotation, after clocks)
    emergence_face,             // 2: Emergence (0-25)
    active_face,                // 3: Active (26-50)
    momentum_face,              // 4: Momentum (51-75)
    descent_face,               // 5: Descent (76-100)
#endif
    
    timer_face,                 // 6: Countdown
    fast_stopwatch_face,        // 7: Quick timing
    advanced_alarm_face,        // 8: 16 alarms with day modes
    sleep_tracker_face,         // 9: Sleep review
    circadian_score_face,       // 10: Circadian alignment
    oracle_face,                // 11: Daily 2-word reading
    world_clock_face,           // 12: Timezone
    moon_phase_face,            // 13: Lunar phase
    sunrise_sunset_face,        // 14: Solar timing
    // End of PRIMARY faces (MODE cycles 0→1→6→7→...→14)
    
    // SECONDARY FACES (Long-press MODE from face 0 → 15)
    comms_face,                 // 15: Phase telemetry export
    lis2dw_monitor_face,        // 16: Accelerometer data
#ifdef HAS_IR_SENSOR
    light_sensor_face,          // 17: Light sensor data
#endif
    voltage_face,               // 18: Battery voltage
    settings_face,              // 19: Configuration
};

#define MOVEMENT_NUM_FACES (sizeof(watch_faces) / sizeof(watch_face_t))

/* Determines what face to go to from the first face on long press of the Mode button.
 * Also excludes these faces from the normal rotation.
 * In the default firmware, this lets you access temperature and battery voltage with a long press of Mode.
 * Some folks also like to use this to hide the preferences and time set faces from the normal rotation.
 * If you don't want any faces to be excluded, set this to 0 and a long Mode press will have no effect.
 */
#ifdef HAS_IR_SENSOR
#define MOVEMENT_SECONDARY_FACE_INDEX (MOVEMENT_NUM_FACES - 5)  // Last 5 faces: comms, lis2dw, light_sensor, voltage, settings
#else
#define MOVEMENT_SECONDARY_FACE_INDEX (MOVEMENT_NUM_FACES - 4)  // Last 4 faces: comms, lis2dw, voltage, settings

#ifdef PHASE_ENGINE_ENABLED
#define MOVEMENT_TERTIARY_FACE_INDEX 2  // Zone faces start at index 2
#else
#define MOVEMENT_TERTIARY_FACE_INDEX 0  // No tertiary faces when phase engine disabled
#endif
#endif

/* Custom hourly chime tune. Check movement_custom_signal_tunes.h for options. */
#define SIGNAL_TUNE_DEFAULT

/* Determines the intensity of the led colors
 * Set a hex value 0-15 with 0x0 being off and 0xF being max intensity
 */
#define MOVEMENT_DEFAULT_RED_COLOR 0x0
#define MOVEMENT_DEFAULT_GREEN_COLOR 0xF
#define MOVEMENT_DEFAULT_BLUE_COLOR 0x0

/* Set to true for 24h mode or false for 12h mode */
#define MOVEMENT_DEFAULT_24H_MODE false

/* Enable or disable the sound on mode button press */
#define MOVEMENT_DEFAULT_BUTTON_SOUND true

#define MOVEMENT_DEFAULT_BUTTON_VOLUME WATCH_BUZZER_VOLUME_SOFT
#define MOVEMENT_DEFAULT_SIGNAL_VOLUME WATCH_BUZZER_VOLUME_LOUD
#define MOVEMENT_DEFAULT_ALARM_VOLUME WATCH_BUZZER_VOLUME_LOUD

/* Set the timeout before switching back to the main watch face
 * Valid values are:
 * 0: 60 seconds
 * 1: 2 minutes
 * 2: 5 minutes
 * 3: 30 minutes
 */
#define MOVEMENT_DEFAULT_TIMEOUT_INTERVAL 0

/* Set the timeout before switching to low energy mode
 * Valid values are:
 * 0: Never
 * 1: 10 minutes
 * 2: 1 hour
 * 3: 2 hours
 * 4: 6 hours
 * 5: 12 hours
 * 6: 1 day
 * 7: 7 days
 */
#define MOVEMENT_DEFAULT_LOW_ENERGY_INTERVAL 2

/* Set the led duration
 * Valid values are:
 * 0: No LED
 * 1: 1 second
 * 2: 3 seconds
 * 3: 5 seconds
 */
#define MOVEMENT_DEFAULT_LED_DURATION 1

/* Optionally debounce button presses (disable by default).
 * A value of 4 is a good starting point if you have issues
 * with multiple button presses firing.
*/
#define MOVEMENT_DEBOUNCE_TICKS 0

#endif // MOVEMENT_CONFIG_H_
