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

#include <stddef.h>
#include "watch_extint.h"
#include "app.h"
#include <emscripten.h>

static uint32_t watch_backup_data[8];

static bool _wake_up = false;
static watch_cb_t _callback = NULL;


void _wake_up_simulator(void) {
    _wake_up = true;
}

static void cb_extwake_wrapper(void) {
    _wake_up_simulator();

    if (_callback) {
        _callback();
    }
}

void watch_register_extwake_callback(uint8_t pin, watch_cb_t callback, bool level) {
    if (pin == HAL_GPIO_BTN_ALARM_pin()) {
        _callback = callback;
        watch_enable_external_interrupts();
        watch_register_interrupt_callback(pin, cb_extwake_wrapper, level ? INTERRUPT_TRIGGER_RISING : INTERRUPT_TRIGGER_FALLING);
    }
}

void watch_disable_extwake_interrupt(uint8_t pin) {
    if (pin == HAL_GPIO_BTN_ALARM_pin()) {
        _callback = NULL;
        watch_register_interrupt_callback(pin, NULL, INTERRUPT_TRIGGER_NONE);
    }
}

void watch_store_backup_data(uint32_t data, uint8_t reg) {
    if (reg < 8) {
        watch_backup_data[reg] = data;
    }
}

uint32_t watch_get_backup_data(uint8_t reg) {
    if (reg < 8) {
        return watch_backup_data[reg];
    }

    return 0;
}

void watch_enter_sleep_mode(void) {
    // TODO: (a2) hook to UI

    // disable tick interrupt
    watch_rtc_disable_all_periodic_callbacks();

    // // disable all buttons but alarm
    watch_register_interrupt_callback(HAL_GPIO_BTN_MODE_pin(), NULL, INTERRUPT_TRIGGER_NONE);
    watch_register_interrupt_callback(HAL_GPIO_BTN_LIGHT_pin(), NULL, INTERRUPT_TRIGGER_NONE);

    sleep(4);

    // call app_setup so the app can re-enable everything we disabled.
    app_setup();
}

void watch_enter_backup_mode(void) {
    // TODO: (a2) hook to UI

    // go into backup sleep mode (5). when we exit, the reset controller will take over.
    sleep(5);
}

void sleep(const uint8_t mode) {
    (void) mode;

    // we basically hang out here until an interrupt wakes us.
    while(!_wake_up) {
        emscripten_sleep(100);
    }

    _wake_up = false;
}
