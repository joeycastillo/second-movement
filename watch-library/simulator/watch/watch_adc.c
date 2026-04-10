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

#include "watch_adc.h"
#include <emscripten.h>

static uint16_t _sim_get_light_adc_value(void) {
    // If light_tx_queue has entries, consume one per ADC read (for
    // synchronized light-protocol transmission from the UI).
    // Otherwise fall back to the static light_level set by the swatches.
    double level = EM_ASM_DOUBLE({
        if (window.light_tx_queue && window.light_tx_queue.length > 0) {
            if (!window._lux_tx_started) {
                window._lux_tx_started = true;
                window._lux_tx_start_ms = Date.now();
                console.log("LUX TX start: " + window._lux_tx_start_ms + " ms");
            }
            var val = window.light_tx_queue.shift();
            if (window.light_tx_queue.length === 0) {
                var now = Date.now();
                console.log("LUX TX done: " + now + " ms (elapsed: " + (now - window._lux_tx_start_ms) + " ms)");
                window._lux_tx_started = false;
                window._lux_tx_start_ms = 0;
            }
            return val;
        }
        return window.light_level || 0.0;
    });
    if (level <= 0) return 0;
    if (level >= 65535.0) return 65535;
    return (uint16_t)level;
}

void watch_enable_adc(void) {}

void watch_enable_analog_input(const uint16_t pin) {
    (void) pin;
}

uint16_t watch_get_analog_pin_level(const uint16_t pin) {
    (void) pin;
    return _sim_get_light_adc_value();
}

void watch_set_analog_num_samples(uint16_t samples) {
    (void) samples;
}

void watch_set_analog_sampling_length(uint8_t cycles) {
    (void) cycles;
}

void watch_set_analog_reference_voltage(uint8_t reference) {
    (void) reference;
}

uint16_t watch_get_vcc_voltage(void) {
    // TODO: (a2) hook to UI
    return 3000;
}

inline void watch_disable_analog_input(const uint16_t pin) {
    (void) pin;
}

inline void watch_disable_adc(void) {}

// Gossamer-level ADC stubs: these are the low-level peripheral functions
// that some watch faces call directly (e.g. light_sensor_face).

void adc_init(void) {}

void adc_enable(void) {}

void adc_disable(void) {}

uint16_t adc_get_analog_value(uint16_t pin) {
    (void) pin;
    return _sim_get_light_adc_value();
}
