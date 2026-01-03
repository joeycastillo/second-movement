/*
 * MIT License
 *
 * Copyright (c) 2022-2024 Joey Castillo
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

#pragma once

/*
 * SETTINGS face (replaces old Preferences watch face)
 *
 * The Settings watch face allows you to configure various options on your
 * Sensor Watch. Like all other screens, you advance the field you’re setting
 * with the Light button, and advance its value with the Alarm button. The
 * Settings watch face labels each setting with a two-letter code up top
 * when using the classic LCD, or something more readable on the custom LCD.
 * The following list describes each setting and their options:
 *
 *  CL / Clock - Clock mode.
 *      This setting allows you to select a 12-or 24-hour clock display. All
 *      watch faces that support displaying the time will respect this setting;
 *      for example, both Simple Clock, World Clock and Sunrise/Sunset will
 *      display the time in 24 hour format if the 24 hour clock is selected here.
 *
 *  BT / BTN - Button beep.
 *      This setting allows you to choose whether the Mode button should emit
 *      a beep when pressed, and if so, how loud it should be. Options are
 *      "Y" for yes and "N" for no.
 * 
 *  SI / SIG - Signal beep.
 *      This setting allows you to choose the hourly chime buzzer volume.
 * 
 *  AL / ALM - Alarm beep.
 *      This setting allows you to choose the alarm buzzer volume.
 *
 *  TO / Tmout - Timeout.
 *      Sets the time until screens that time out (like Settings and Time Set)
 *      snap back to the first screen. 60 seconds is a good default for the
 *      stock firmware, but if you choose a custom firmware with faces that
 *      you’d like to keep on screen for longer, you can set that here.
 *
 *  LE / LoEne - Low Energy mode.
 *      Sets the time until the watch enters its low energy sleep mode.
 *      Options range from 1 hour to 7 days, or Never. The more often Sensor
 *      Watch goes to sleep, the longer its battery will last — but you will
 *      lose the seconds indicator while it is asleep. This setting allows
 *      you to make a tradeoff between the device’s responsiveness and its
 *      longevity.
 *
 *  LT / LED - Light Duration and Color
 *      The first LED screen lets you choose how long the LED should stay lit
 *       when the LIGHT button is pressed. Options are 1 second, 3 seconds
 *       and 5 seconds, or “No LED” to disable the LED entirely.
 *      The remaining screens set the intensity of the red, green or blue LEDs
 *       depending on the target Sensor Board hardware to allow a custom color
 *       blend. Values range from 0 (off) to 15 (full intensity).
 *      On the LED color screens, the LED remains on so that you can see the
 *      effect of mixing the LED colors.
 */

#include "movement.h"

typedef struct {
    void (*display)(uint8_t subsecond);
    void (*advance)();
} settings_screen_t;

typedef struct {
    int8_t current_page;
    int8_t num_settings;
    int8_t led_color_start;
    int8_t led_color_end;
    settings_screen_t *settings_screens;
} settings_state_t;

void settings_face_setup(uint8_t watch_face_index, void ** context_ptr);
void settings_face_activate(void *context);
bool settings_face_loop(movement_event_t event, void *context);
void settings_face_resign(void *context);

#define settings_face ((const watch_face_t){ \
    settings_face_setup, \
    settings_face_activate, \
    settings_face_loop, \
    settings_face_resign, \
    NULL, \
})
