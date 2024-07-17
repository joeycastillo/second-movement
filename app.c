/*
 * MIT License
 *
 * Copyright (c) 2021-2024 Joey Castillo
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

#include "app.h"
#include "delay.h"
#include "watch.h"
#include "watch_private.h"
#include "usb.h"
#include "tusb.h"
#include "watch_usb_cdc.h"
#include "filesystem.h"
#include "shell.h"
#include "tcc.h"

uint8_t ticks = 0;
bool beep = false;

void cb_tick(void);
void cb_mode_btn_interrupt(void);
void cb_light_btn_interrupt(void);
void cb_alarm_btn_extwake(void);

void yield(void) {
    tud_task();
    cdc_task();
}

void app_init(void) {
    // initialize the watch hardware.
    _watch_init();

    // check if we are plugged into USB power.
    HAL_GPIO_VBUS_DET_in();
    HAL_GPIO_VBUS_DET_pulldown();
    if (HAL_GPIO_VBUS_DET_read()){
        /// if so, enable USB functionality.
        _watch_enable_usb();
    }
    HAL_GPIO_VBUS_DET_off();

    filesystem_init();
}

void app_setup(void) {
    watch_enable_leds();
    watch_enable_external_interrupts();

    watch_rtc_register_periodic_callback(cb_tick, 1);

    HAL_GPIO_BTN_LIGHT_in();
    HAL_GPIO_BTN_LIGHT_pulldown();
    HAL_GPIO_BTN_LIGHT_pmuxen(HAL_GPIO_PMUX_EIC);
    HAL_GPIO_BTN_MODE_in();
    HAL_GPIO_BTN_MODE_pulldown();
    HAL_GPIO_BTN_MODE_pmuxen(HAL_GPIO_PMUX_EIC);

    watch_register_interrupt_callback(HAL_GPIO_BTN_LIGHT_pin(), cb_light_btn_interrupt, INTERRUPT_TRIGGER_RISING);
    watch_register_interrupt_callback(HAL_GPIO_BTN_MODE_pin(), cb_mode_btn_interrupt, INTERRUPT_TRIGGER_RISING);
    watch_register_extwake_callback(HAL_GPIO_BTN_ALARM_pin(), cb_alarm_btn_extwake, true);

    watch_enable_display();
    watch_display_top_left("WA");
    watch_display_top_right(" 0");
    watch_display_main_line(" test ");
}

bool app_loop(void) {
    if (usb_is_enabled()) {
        yield();
        shell_task();
    }

    if (beep) {
        watch_buzzer_play_note(BUZZER_NOTE_C8, 100);
        beep = false;
    }

    char buf[4];
    sprintf(buf, "%2d", ticks);
    watch_display_top_right(buf);

    if (ticks >= 10) {
        watch_enter_sleep_mode();
    }

    return true;
}

void cb_tick(void) {
    ticks++;
    watch_set_led_off();
}

void cb_light_btn_interrupt(void) {
    watch_set_led_green();
}

void cb_mode_btn_interrupt(void) {
    beep = true;
}

void cb_alarm_btn_extwake(void) {
    ticks = 0;
}