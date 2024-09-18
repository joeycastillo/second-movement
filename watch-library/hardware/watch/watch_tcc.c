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

void _watch_enable_tcc(void);
void _watch_disable_tcc(void);

bool watch_is_buzzer_or_led_enabled(void){
    return tcc_is_enabled(0);
}

inline void watch_enable_buzzer(void) {
    if (!tcc_is_enabled(0)) {
        _watch_enable_tcc();
    }
}

inline void watch_set_buzzer_period(uint32_t period) {
    tcc_set_period(0, period, true);
    tcc_set_cc(0, (WATCH_BUZZER_TCC_CHANNEL) % 4, period / 2, true);
}

void watch_disable_buzzer(void) {
    _watch_disable_tcc();
}

inline void watch_set_buzzer_on(void) {
    HAL_GPIO_BUZZER_out();
    HAL_GPIO_BUZZER_pmuxen(HAL_GPIO_PMUX_TCC_ALT);
}

inline void watch_set_buzzer_off(void) {
    HAL_GPIO_BUZZER_pmuxdis();
    HAL_GPIO_BUZZER_off();
}

void watch_buzzer_play_note(BuzzerNote note, uint16_t duration_ms) {
    if (note == BUZZER_NOTE_REST) {
        watch_set_buzzer_off();
    } else {
        watch_set_buzzer_period(NotePeriods[note]);
        watch_set_buzzer_on();
    }
    delay_ms(duration_ms);
    watch_set_buzzer_off();
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

    // enable LED PWM pins (the LED driver assumes if the TCC is on, the pins are enabled)
    HAL_GPIO_RED_pmuxen(HAL_GPIO_PMUX_TCC_ALT);
    HAL_GPIO_RED_out();
#ifdef WATCH_GREEN_TCC_CHANNEL
    HAL_GPIO_GREEN_pmuxen(HAL_GPIO_PMUX_TCC_ALT);
    HAL_GPIO_GREEN_out();
#endif
#ifdef WATCH_BLUE_TCC_CHANNEL
    HAL_GPIO_BLUE_pmuxen(HAL_GPIO_PMUX_TCC_ALT);
    HAL_GPIO_BLUE_out();
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
    if (!tcc_is_enabled(0)) {
        _watch_enable_tcc();
    }
}

void watch_disable_leds(void) {
    _watch_disable_tcc();
}

void watch_set_led_color(uint8_t red, uint8_t green) {
#ifdef WATCH_BLUE_TCC_CHANNEL
    watch_set_led_color_rgb(red, green, 0);
#else
    watch_set_led_color_rgb(red, green, green);
#endif
}

void watch_set_led_color_rgb(uint8_t red, uint8_t green, uint8_t blue) {
    if (tcc_is_enabled(0)) {
        uint32_t period = tcc_get_period(0);
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
