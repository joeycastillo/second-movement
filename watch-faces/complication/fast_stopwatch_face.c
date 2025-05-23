/*
 * MIT License
 *
 * Copyright (c) 2022 Andreas Nebinger
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

#include <stdlib.h>
#include <string.h>
#include "fast_stopwatch_face.h"
#include "watch.h"
#include "watch_common_display.h"
#include "watch_utility.h"
#include "watch_rtc.h"

/*
    This watch face implements the original F-91W stopwatch functionality
    including counting hundredths of seconds and lap timing. There are two
    improvements compared to the functionality of the original: 
    1. When reaching 59:59 the counter does not simply jump back to zero,
       but keeps track of hours in the upper right hand corner. (Up to 24h)
    2. Long pressing the light button toggles the led behaviour: it either
       turns on on each button press or it doesn't.
*/

#if __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#else
#include "tc.h"
#endif

// distant future for background task: January 1, 2083
static const watch_date_time_t distant_future = {
    .unit = {0, 0, 0, 1, 1, 63}
};

static uint32_t _ticks;
static uint32_t _lap_ticks;
static uint8_t _blink_ticks;
static uint32_t _old_seconds;
static uint8_t _old_minutes;
static uint8_t _hours;
static bool _colon;
static bool _is_running;

#if __EMSCRIPTEN__

static long _em_interval_id = 0;

void em_cb_handler(void *userData) {
    // interrupt handler for emscripten 128 Hz callbacks
    (void) userData;
    _ticks++;
}

static void _cb_initialize() { }

static inline void _cb_stop() {
    emscripten_clear_interval(_em_interval_id);
    _em_interval_id = 0;
    _is_running = false;
}

static inline void _cb_start() {
    // initiate 128 hz callback
    _em_interval_id = emscripten_set_interval(em_cb_handler, (double)(1000/128), (void *)NULL);
}

#else

static inline void _cb_start() {
    // start the TC1 timer
    tc_enable(1);
    _is_running = true;
}

static inline void _cb_stop() {
    // stop the TC1 timer
    tc_disable(1);
    _is_running = false;
}

static void _cb_initialize() {
    tc_init(1, GENERIC_CLOCK_3, TC_PRESCALER_DIV4);
    tc_set_counter_mode(1, TC_COUNTER_MODE_8BIT);
    tc_set_run_in_standby(1, true);
    _cb_stop();
    tc_count8_set_period(1, 1); // 1024 Hz divided by 4 divided by 2 results in a 128 Hz interrupt
    /// FIXME: #SecondMovement, we need a gossamer wrapper for interrupts.
    TC1->COUNT8.INTENSET.bit.OVF = 1;
    NVIC_ClearPendingIRQ(TC1_IRQn);
    NVIC_EnableIRQ (TC1_IRQn);
}

void irq_handler_tc1(void);
void irq_handler_tc1(void) {
    // interrupt handler for TC1 (globally!)
    _ticks++;
    TC1->COUNT8.INTFLAG.reg |= TC_INTFLAG_OVF;
}

#endif

static inline void _button_beep() {
    // play a beep as confirmation for a button press (if applicable)
    if (movement_button_should_sound()) watch_buzzer_play_note_with_volume(BUZZER_NOTE_C7, 50, movement_button_volume());
}

/// @brief Display minutes, seconds and fractions derived from 128 Hz tick counter
///        on the lcd.
/// @param ticks
static void _display_ticks(uint32_t ticks) {
    char buf[14];
    uint8_t sec_100 = (ticks & 0x7F) * 100 / 128;
    uint32_t seconds = ticks >> 7;
    uint32_t minutes = seconds / 60;
    if (_hours) {
        sprintf(buf, "%2u", _hours);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    } else {
        watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
    }

    sprintf(buf, "%02lu%02lu%02u", minutes, (seconds % 60), sec_100);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

/// @brief Displays the current stopwatch time on the LCD (more optimized than _display_ticks())
static void _draw() {
    if (_lap_ticks == 0) {
        char buf[14];
        uint8_t sec_100 = (_ticks & 0x7F) * 100 / 128;
        if (_is_running) {
            uint32_t seconds = _ticks >> 7;
            if (seconds != _old_seconds) {
                // seconds have changed
                _old_seconds = seconds;
                uint8_t minutes = seconds / 60;
                seconds %= 60;
                if (minutes != _old_minutes) {
                    // minutes have changed, draw everything
                    _old_minutes = minutes;
                    minutes %= 60;
                    if (_hours) {
                        // with hour indicator
                        sprintf(buf, "%2u", _hours);
                        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
                    } else {
                        // no hour indicator
                        watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
                    }
                    sprintf(buf, "%02u%02lu%02u", minutes, seconds, sec_100);
                    watch_display_text(WATCH_POSITION_BOTTOM, buf);
                } else {
                    // just draw seconds
                    sprintf(buf, "%02lu", seconds);
                    // note that we're drawing the seconds in the "minutes" position, since this
                    // watch face uses the "seconds" position for hundredths of seconds
                    watch_display_text(WATCH_POSITION_MINUTES, buf);
                    watch_display_character_lp_seconds('0' + sec_100 / 10, 8);
                    watch_display_character_lp_seconds('0' + sec_100 % 10, 9);
                }
            } else {
                // only draw 100ths of seconds
                watch_display_character_lp_seconds('0' + sec_100 / 10, 8);
                watch_display_character_lp_seconds('0' + sec_100 % 10, 9);
            }
        } else {
            _display_ticks(_ticks);
        }
    }
    if (_is_running) {
        // blink the colon every half second
        uint8_t blink_ticks = ((_ticks >> 6) & 1);
        if (blink_ticks != _blink_ticks) {
            _blink_ticks = blink_ticks;
            _colon = !_colon;
            if (_colon) watch_set_colon();
            else watch_clear_colon();
        }
    }
}

static inline void _update_lap_indicator() {
    if (_lap_ticks) watch_set_indicator(WATCH_INDICATOR_LAP);
    else  watch_clear_indicator(WATCH_INDICATOR_LAP);
}

static inline void _set_colon() {
    watch_set_colon();
    _colon = true;
}

void fast_stopwatch_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(fast_stopwatch_state_t));
        memset(*context_ptr, 0, sizeof(fast_stopwatch_state_t));
        fast_stopwatch_state_t *state = (fast_stopwatch_state_t *)*context_ptr;
        _ticks = _lap_ticks = _blink_ticks = _old_minutes = _old_seconds = _hours = 0;
    _is_running = _colon = false;
        state->light_on_button = true;
    }
    if (!_is_running) {
        // prepare the 128 Hz callback source
        _cb_initialize();
    }
}

void fast_stopwatch_face_activate(void *context) {
    (void) context;
    if (_is_running) {
        // The background task will keep the watch from entering low energy mode while the stopwatch is on screen.
        movement_schedule_background_task(distant_future);
    }
}

bool fast_stopwatch_face_loop(movement_event_t event, void *context) {
    fast_stopwatch_state_t *state = (fast_stopwatch_state_t *)context;

    // handle overflow of fast ticks
    while (_ticks >= (128 * 60 * 60)) {
        _ticks -= (128 * 60 * 60);
        _hours++;
        if (_hours >= 24) _hours -= 24;
        // initiate a re-draw
        _old_minutes = 59;
    }

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _set_colon();
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "STW", "ST");
            _update_lap_indicator();
            if (_is_running) movement_request_tick_frequency(16);
            _display_ticks(_lap_ticks ? _lap_ticks : _ticks);
            break;
        case EVENT_TICK:
            _draw();
            break;
        case EVENT_LIGHT_LONG_PRESS:
            // kind od hidden feature: long press toggles light on or off
            state->light_on_button = !state->light_on_button;
            if (state->light_on_button) movement_illuminate_led();
            else watch_set_led_off();
            break;
        case EVENT_ALARM_BUTTON_DOWN:
            _is_running = !_is_running;
            if (_is_running) {
                // start or continue stopwatch
                movement_request_tick_frequency(16);
                // register 128 hz callback for time measuring
                _cb_start();
                // schedule the keepalive task when running
                movement_schedule_background_task(distant_future);
            } else {
                // stop the stopwatch
                _cb_stop();
                movement_request_tick_frequency(1);
                _set_colon();
                // cancel the keepalive task
                movement_cancel_background_task();
            }
            _draw();
            _button_beep();
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            if (state->light_on_button) movement_illuminate_led();
            if (_is_running) {
                if (_lap_ticks) {
                    // clear lap and continue running
                    _lap_ticks = 0;
                    movement_request_tick_frequency(16);
                } else {
                    // set lap ticks and stop updating the display
                    _lap_ticks = _ticks;
                    movement_request_tick_frequency(2);
                    _set_colon();
                }
            } else {
                if (_lap_ticks) {
                    // clear lap and show running stopwatch
                    _lap_ticks = 0;
                } else if (_ticks) {
                    // reset stopwatch
                    _ticks = _lap_ticks = _blink_ticks = _old_minutes = _old_seconds = _hours = 0;
                    _button_beep();
                }
            }
            _display_ticks(_ticks);
            _update_lap_indicator();
            break;
        case EVENT_TIMEOUT:
            if (!_is_running) movement_move_to_face(0);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            _draw();
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }
    return true;
}

void fast_stopwatch_face_resign(void *context) {
    (void) context;
    // cancel the keepalive task
    movement_cancel_background_task();
}
