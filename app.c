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

void yield(void) {
    tud_task();
    cdc_task();
}

void app_init(void) {
    // check if we are plugged into USB power.
    HAL_GPIO_VBUS_DET_in();
    HAL_GPIO_VBUS_DET_pulldown();
    if (HAL_GPIO_VBUS_DET_read()){
        /// if so, enable USB functionality.
        _watch_enable_usb();
    }
    HAL_GPIO_VBUS_DET_off();

    _watch_init();
}

void app_setup(void) {
    watch_enable_display();
    watch_display_top_left("MO");
    watch_display_top_right("15");
    watch_display_main_line("123456");
    watch_set_colon();
    watch_set_indicator(WATCH_INDICATOR_PM);
}

bool app_loop(void) {
    if (usb_is_enabled()) {
        yield();
    }

    return true;
}
