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

#include <stdio.h>
#include "watch_extint.h"
#include "watch_gpio.h"
#include "eic.h"

watch_cb_t eic_callbacks[16] = { NULL };

void watch_eic_callback(uint8_t channel);

void watch_enable_external_interrupts(void) {
    eic_init();
    eic_configure_callback(watch_eic_callback);
    eic_enable();
}

void watch_disable_external_interrupts(void) {
    eic_disable();
}

void watch_register_interrupt_callback(const uint8_t pin, watch_cb_t callback, eic_interrupt_trigger_t trigger) {
    watch_enable_digital_input(pin);
    bool filten = false;

    // check if this is a button pin
    if (pin == HAL_GPIO_BTN_LIGHT_pin() || pin == HAL_GPIO_BTN_MODE_pin() || pin == HAL_GPIO_BTN_ALARM_pin()) {
        // if so, enable the pull-down resistor
        watch_enable_pull_down(pin);
        filten = true;
    }

    int8_t channel = eic_configure_pin(pin, trigger, filten);
    if (channel >= 0 && channel < 16) {
        printf("Configured port %d pin %d on channel %d\n", pin >> 5, pin & 0x1F, channel);
        eic_enable_interrupt(pin);
        eic_callbacks[channel] = callback;
    }
}

void watch_eic_callback(uint8_t channel) {
    if (eic_callbacks[channel] != NULL) {
        eic_callbacks[channel]();
    }
}
