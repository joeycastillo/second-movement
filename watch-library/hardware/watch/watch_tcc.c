/*
 * MIT License
 *
 * Copyright (c) 2020 Joey Castillo
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

#include "watch_tcc.h"
#include "delay.h"
#include "tcc.h"
#include "tc.h"

static void _watch_enable_tcc(void);
static void _watch_disable_tcc(void);
static void _watch_maybe_enable_tcc(void);
static void _watch_maybe_disable_tcc(void);
static void _watch_enable_led_pins(void);
static void _watch_disable_led_pins(void);
static void (*_cb_tc0)(void) = NULL;
static void cb_watch_buzzer_seq(void);
static void cb_watch_buzzer_raw_source(void);

static uint16_t _seq_position;
static int8_t _tone_ticks, _repeat_counter;
static int8_t *_sequence;
static watch_buzzer_raw_source_t _raw_source;
static void* _userdata;
static uint8_t _volume;
static void (*_cb_finished)(void);
static watch_cb_t _cb_start_global = NULL;
static watch_cb_t _cb_stop_global = NULL;
static volatile bool _led_is_active = false;
static volatile bool _buzzer_is_active = false;
static volatile uint8_t _current_led_color[3] = {0, 0, 0};

static void _watch_set_led_duty_cycle(uint32_t period, uint8_t red, uint8_t green, uint8_t blue);

static void _tcc_write_RUNSTDBY(bool value) {
    // enables or disables RUNSTDBY of the tcc
    tcc_disable(0);
    tcc_set_run_in_standby(0, value);
    tcc_enable(0);
}

static inline void _tc0_start() {
    // start the TC0 timer
    tc_enable(0);
}

static inline void _tc0_stop() {
    // stop the TC0 timer
    tc_disable(0);
}

static void _tc0_initialize() {
    // setup and initialize TC0 for a 64 Hz interrupt
    tc_init(0, GENERIC_CLOCK_3, TC_PRESCALER_DIV2);
    tc_set_counter_mode(0, TC_COUNTER_MODE_8BIT);
    tc_set_run_in_standby(0, true);
    tc_count8_set_period(0, 7); // 1024 Hz divided by 2 divided by 8 equals 64 Hz
    /// FIXME: #SecondMovement, we need a gossamer wrapper for interrupts.
    TC0->COUNT8.INTENSET.bit.OVF = 1;
    NVIC_ClearPendingIRQ(TC0_IRQn);
    NVIC_EnableIRQ (TC0_IRQn);
}

void watch_buzzer_play_sequence(int8_t *note_sequence, void (*callback_on_end)(void)) {
    watch_buzzer_play_sequence_with_volume(note_sequence, callback_on_end, WATCH_BUZZER_VOLUME_LOUD);
}

void watch_buzzer_play_sequence_with_volume(int8_t *note_sequence, void (*callback_on_end)(void), watch_buzzer_volume_t volume) {
    // Abort any previous sequence
    watch_buzzer_abort_sequence();

    if (_cb_start_global) {
        _cb_start_global();
    }

    watch_enable_buzzer();
    watch_set_buzzer_off();
    _sequence = note_sequence;
    _cb_finished = callback_on_end;
    _volume = volume == WATCH_BUZZER_VOLUME_SOFT ? 5 : 25;
    _seq_position = 0;
    _tone_ticks = 0;
    _repeat_counter = -1;
    // prepare buzzer
    
    _cb_tc0 = cb_watch_buzzer_seq;
    // setup TC0 timer
    _tc0_initialize();
    // start the timer (for the 64 hz callback)
    _tc0_start();
}

void cb_watch_buzzer_seq(void) {
    // callback for reading the note sequence
    if (_tone_ticks == 0) {
        if (_sequence[_seq_position] < 0 && _sequence[_seq_position + 1]) {
            // repeat indicator found
            if (_repeat_counter == -1) {
                // first encounter: load repeat counter
                _repeat_counter = _sequence[_seq_position + 1];
            } else _repeat_counter--;
            if (_repeat_counter > 0)
                // rewind
                if (_seq_position > _sequence[_seq_position] * -2)
                    _seq_position += _sequence[_seq_position] * 2;
                else
                    _seq_position = 0;
            else {
                // continue
                _seq_position += 2;
                _repeat_counter = -1;
            }
        }
        if (_sequence[_seq_position] && _sequence[_seq_position + 1]) {
            // read note
            watch_buzzer_note_t note = _sequence[_seq_position];
            if (note != BUZZER_NOTE_REST) {
                watch_set_buzzer_period_and_duty_cycle(NotePeriods[note], _volume);
                watch_set_buzzer_on();
            } else watch_set_buzzer_off();
            // set duration ticks and move to next tone
            _tone_ticks = _sequence[_seq_position + 1] - 1;
            _seq_position += 2;
        } else {
            // end the sequence
            watch_buzzer_abort_sequence();
        }
    } else _tone_ticks--;
}

void watch_buzzer_play_raw_source(watch_buzzer_raw_source_t raw_source, void* userdata, watch_cb_t callback_on_end) {
    watch_buzzer_play_raw_source_with_volume(raw_source, userdata, callback_on_end, WATCH_BUZZER_VOLUME_LOUD);
}

void watch_buzzer_play_raw_source_with_volume(watch_buzzer_raw_source_t raw_source, void* userdata, watch_cb_t callback_on_end, watch_buzzer_volume_t volume) {
    // Abort any previous sequence
    watch_buzzer_abort_sequence();

    if (_cb_start_global) {
        _cb_start_global();
    }

    watch_enable_buzzer();

    watch_set_buzzer_off();
    _raw_source = raw_source;
    _userdata = userdata;
    _cb_finished = callback_on_end;
    _volume = volume == WATCH_BUZZER_VOLUME_SOFT ? 5 : 25;
    _seq_position = 0;
    _tone_ticks = 0;
    // prepare buzzer

    _cb_tc0 = cb_watch_buzzer_raw_source;
    // setup TC0 timer
    _tc0_initialize();
    // start the timer (for the 64 hz callback)
    _tc0_start();
}

void cb_watch_buzzer_raw_source(void) {
    // callback for reading the note sequence
    uint16_t period;
    uint16_t duration;
    bool done;

    if (_tone_ticks == 0) {
        done = _raw_source(_seq_position, _userdata, &period, &duration);

        if (done || duration == 0) {
            // end the sequence
            watch_buzzer_abort_sequence();
        } else {
            if (period == WATCH_BUZZER_PERIOD_REST) {
                watch_set_buzzer_off();
            } else {
                watch_set_buzzer_period_and_duty_cycle(period, _volume);
                watch_set_buzzer_on();
            }

            // set duration ticks and move to next tone
            _tone_ticks = duration - 1;
            _seq_position += 1;
        }
    } else {
        _tone_ticks--;
    }
}

void watch_buzzer_abort_sequence(void) {
    // ends/aborts the sequence
    if (!_buzzer_is_active) {
        return;
    }

    _tc0_stop();

    watch_set_buzzer_off();

    // disable TCC
    watch_disable_buzzer();

    if (_cb_stop_global) {
        _cb_stop_global();
    }

    if (_cb_finished) {
        _cb_finished();
    }
}

void watch_buzzer_register_global_callbacks(watch_cb_t cb_start, watch_cb_t cb_stop) {
    _cb_stop_global = cb_start;
    _cb_stop_global = cb_stop;
}

void irq_handler_tc0(void) {
    // interrupt handler for TC0 (globally!)
    if (_cb_tc0) {
        _cb_tc0();
    }
    TC0->COUNT8.INTFLAG.reg |= TC_INTFLAG_OVF;
}

void _watch_maybe_enable_tcc(void) {
    if (!_buzzer_is_active && !_led_is_active) {
        return;
    }

    if (!tcc_is_enabled(0)) {
        // tcc_set_run_in_standby(0, true);
        _watch_enable_tcc();
        // TCC should run in standby mode
        _tcc_write_RUNSTDBY(true);
    }
}

void _watch_maybe_disable_tcc(void) {
    if (_buzzer_is_active || _led_is_active) {
        return;
    }

    if (tcc_is_enabled(0)) {
        _tcc_write_RUNSTDBY(false);
        _watch_disable_tcc();
    }
}

void watch_enable_buzzer(void) {
    _buzzer_is_active = true;
    _watch_maybe_enable_tcc();
}

void watch_disable_buzzer(void) {
    _buzzer_is_active = false;
    watch_set_buzzer_off();
    _watch_maybe_disable_tcc();
}

void watch_set_buzzer_period_and_duty_cycle(uint32_t period, uint8_t duty) {
    tcc_set_period(0, period, true);
    tcc_set_cc(0, (WATCH_BUZZER_TCC_CHANNEL) % 4, period / (100 / duty), true);
    // The buzzer determines the period, which means that if the LED was active before it will flicker
    // Update the LED duty cycle to match the new period required by the buzzer.
    if (_led_is_active) {
        _watch_set_led_duty_cycle(period, _current_led_color[0], _current_led_color[1], _current_led_color[2]);
    }
}

inline void watch_set_buzzer_on(void) {
    HAL_GPIO_BUZZER_out();
    HAL_GPIO_BUZZER_pmuxen(HAL_GPIO_PMUX_TCC_ALT);
}

inline void watch_set_buzzer_off(void) {
    HAL_GPIO_BUZZER_pmuxdis();
    HAL_GPIO_BUZZER_off();
}

void watch_buzzer_play_note(watch_buzzer_note_t note, uint16_t duration_ms) {
    watch_buzzer_play_note_with_volume(note, duration_ms, WATCH_BUZZER_VOLUME_LOUD);
}

void watch_buzzer_play_note_with_volume(watch_buzzer_note_t note, uint16_t duration_ms, watch_buzzer_volume_t volume) {
    static int8_t single_note_sequence[3];

    single_note_sequence[0] = note;
    // 64 ticks per second for the tc0
    // Each tick is approximately 15ms
    uint16_t duration = duration_ms / 15;
    if (duration > 127) duration = 127;
    single_note_sequence[1] = (int8_t)duration;
    single_note_sequence[2] = 0;

    watch_buzzer_play_sequence_with_volume(single_note_sequence, NULL, volume);
}

void _watch_enable_tcc(void) {
    // set up the TCC with a 1 MHz clock, but there's a trick:
    if (USB->DEVICE.CTRLA.bit.ENABLE) {
        // if USB is enabled, we are running an 8 MHz clock, so we divide by 8.
        tcc_init(0, GENERIC_CLOCK_0, TCC_PRESCALER_DIV8);
    } else {
        // otherwise it's 4 Mhz and we divide by 4.
        tcc_init(0, GENERIC_CLOCK_0, TCC_PRESCALER_DIV4);
    }
    // We're going to use normal PWM mode, which means period is controlled by PER, and duty cycle is controlled by
    // each compare channel's value:
    //  * Buzzer tones are set by setting PER to the desired period for a given frequency, and CC[1] to half of that
    //    period (i.e. a square wave with a 50% duty cycle).
    //  * LEDs on CC[0] CC[2] and CC[3] can be set to any value from 0 (off) to PER (fully on).
    tcc_set_wavegen(0, TCC_WAVEGEN_NORMAL_PWM);
#ifdef WATCH_INVERT_LED_POLARITY
    // invert all channels, we'll flip the buzzer back in just a moment.
    // this is easier than writing a maze of #ifdefs.
    tcc_set_channel_polarity(0, 4, TCC_CHANNEL_POLARITY_INVERTED);
    tcc_set_channel_polarity(0, 5, TCC_CHANNEL_POLARITY_INVERTED);
    tcc_set_channel_polarity(0, 6, TCC_CHANNEL_POLARITY_INVERTED);
    tcc_set_channel_polarity(0, 7, TCC_CHANNEL_POLARITY_INVERTED);
#endif // WATCH_INVERT_LED_POLARITY
    tcc_set_channel_polarity(0, WATCH_BUZZER_TCC_CHANNEL, TCC_CHANNEL_POLARITY_NORMAL);

    // Set the period to 1 kHz to start.
    tcc_set_period(0, 1000, false);

    // Set the duty cycle of all pins to 0: LED's off, buzzer not buzzing.
    tcc_set_cc(0, (WATCH_BUZZER_TCC_CHANNEL) % 4, 0, false);
    tcc_set_cc(0, (WATCH_RED_TCC_CHANNEL) % 4, 0, false);
#ifdef WATCH_GREEN_TCC_CHANNEL
    tcc_set_cc(0, (WATCH_GREEN_TCC_CHANNEL) % 4, 0, false);
#endif
#ifdef WATCH_BLUE_TCC_CHANNEL
    tcc_set_cc(0, (WATCH_BLUE_TCC_CHANNEL) % 4, 0, false);
#endif

    // Enable the TCC
    tcc_enable(0);
}

void _watch_disable_tcc(void) {
    // disable all PWM pins
    HAL_GPIO_BUZZER_pmuxdis();
    HAL_GPIO_BUZZER_off();
    HAL_GPIO_RED_pmuxdis();
    HAL_GPIO_RED_off();
#ifdef WATCH_GREEN_TCC_CHANNEL
    HAL_GPIO_GREEN_pmuxdis();
    HAL_GPIO_GREEN_off();
#endif
#ifdef WATCH_BLUE_TCC_CHANNEL
    HAL_GPIO_BLUE_pmuxdis();
    HAL_GPIO_BLUE_off();
#endif
    tcc_disable(0);
}

void watch_enable_leds(void) {
    _led_is_active = true;
    _watch_enable_led_pins();
    _watch_maybe_enable_tcc();
}

void watch_disable_leds(void) {
    _led_is_active = false;
    _watch_disable_led_pins();
    _watch_maybe_disable_tcc();
}

void _watch_enable_led_pins(void) {
    // enable LED PWM pins (the LED driver assumes if the TCC is on, the pins are enabled)
    HAL_GPIO_RED_pmuxen(HAL_GPIO_PMUX_TCC_ALT);
    HAL_GPIO_RED_drvstr(1);
    HAL_GPIO_RED_out();
#ifdef WATCH_GREEN_TCC_CHANNEL
    HAL_GPIO_GREEN_pmuxen(HAL_GPIO_PMUX_TCC_ALT);
    HAL_GPIO_GREEN_drvstr(1);
    HAL_GPIO_GREEN_out();
#endif
#ifdef WATCH_BLUE_TCC_CHANNEL
    HAL_GPIO_BLUE_pmuxen(HAL_GPIO_PMUX_TCC_ALT);
    HAL_GPIO_BLUE_drvstr(1);
    HAL_GPIO_BLUE_out();
#endif
}

void _watch_disable_led_pins(void) {
    HAL_GPIO_RED_pmuxdis();
    HAL_GPIO_RED_off();
#ifdef WATCH_GREEN_TCC_CHANNEL
    HAL_GPIO_GREEN_pmuxdis();
    HAL_GPIO_GREEN_off();
#endif
#ifdef WATCH_BLUE_TCC_CHANNEL
    HAL_GPIO_BLUE_pmuxdis();
    HAL_GPIO_BLUE_off();
#endif
}

void watch_set_led_color(uint8_t red, uint8_t green) {
#ifdef WATCH_BLUE_TCC_CHANNEL
    watch_set_led_color_rgb(red, green, 0);
#else
    watch_set_led_color_rgb(red, green, green);
#endif
}

static void _watch_set_led_duty_cycle(uint32_t period, uint8_t red, uint8_t green, uint8_t blue) {
    tcc_set_cc(0, (WATCH_RED_TCC_CHANNEL) % 4, ((period * (uint32_t)red * 1000ull) / 255000ull), true);
#ifdef WATCH_GREEN_TCC_CHANNEL
    tcc_set_cc(0, (WATCH_GREEN_TCC_CHANNEL) % 4, ((period * (uint32_t)green * 1000ull) / 255000ull), true);
#else
    (void) green; // silence warning
#endif
#ifdef WATCH_BLUE_TCC_CHANNEL
    tcc_set_cc(0, (WATCH_BLUE_TCC_CHANNEL) % 4, ((period * (uint32_t)blue * 1000ull) / 255000ull), true);
#else
    (void) blue; // silence warning
#endif
}

void watch_set_led_color_rgb(uint8_t red, uint8_t green, uint8_t blue) {
    bool turning_on = (red | green | blue) != 0;

    if (turning_on) {
        _current_led_color[0] = red;
        _current_led_color[1] = green;
        _current_led_color[2] = blue;
        watch_enable_leds();
        uint32_t period = tcc_get_period(0);
        _watch_set_led_duty_cycle(period, red, green, blue);
    } else {
        if (tcc_is_enabled(0)) {
            _watch_set_led_duty_cycle(1, red, green, blue);
        }
        watch_disable_leds();
    }
}

void watch_set_led_red(void) {
    watch_set_led_color_rgb(255, 0, 0);
}

void watch_set_led_green(void) {
    watch_set_led_color_rgb(0, 255, 0);
}

void watch_set_led_yellow(void) {
    watch_set_led_color_rgb(255, 255, 0);
}

void watch_set_led_off(void) {
    watch_set_led_color_rgb(0, 0, 0);
}
