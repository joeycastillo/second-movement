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

#include "watch_led.h"
#include "watch_private.h"
#include "tcc.h"

void _watch_enable_tcc(void);

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
