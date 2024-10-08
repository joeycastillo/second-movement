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
 * PREFERENCES face
 *
 * The Preferences watch face allows you to configure various options on your
 * Sensor Watch. Like all other screens, you advance the field you’re setting
 * with the Light button, and advance its value with the Alarm button. The
 * Preferences watch face labels each setting with a two-letter code on the
 * top row; the following list describes each setting and their options:
 *
 *  CL - Clock mode.
 *      This setting allows you to select a 12-or 24-hour clock display. All
 *      watch faces that support displaying the time will respect this setting;
 *      for example, both Simple Clock, World Clock and Sunrise/Sunset will
 *      display the time in 24 hour format if the 24 hour clock is selected here.
 *
 *  MO - Mode button.
 *      This setting allows you to choose whether the Mode button should emit
 *      a beep when pressed, and if so, how loud it should be. Options are
 *      "-" (no beep), "L" (low volume) and "H" (high volume).
 *
 *  MO - Mode button.
 *      This setting allows you to choose whether the Mode button should emit
 *      a beep when pressed, and if so, how loud it should be. Options are
 *      "-" (no beep), "L" (low volume) and "H" (high volume).
 *
 *  TO - Timeout.
 *      Sets the time until screens that time out (like Settings and Time Set)
 *      snap back to the first screen. 60 seconds is a good default for the
 *      stock firmware, but if you choose a custom firmware with faces that
 *      you’d like to keep on screen for longer, you can set that here.
 *
 *  LE - Low Energy mode.
 *      Sets the time until the watch enters its low energy sleep mode.
 *      Options range from 1 hour to 7 days, or Never. The more often Sensor
 *      Watch goes to sleep, the longer its battery will last — but you will
 *      lose the seconds indicator while it is asleep. This setting allows
 *      you to make a tradeoff between the device’s responsiveness and its
 *      longevity.
 *
 *  LT - Light.
 *      This setting has three screens.
 *      The first lets you choose how long the LED should stay lit when the
 *       LIGHT button is pressed. Options are 1 second, 3 seconds and 5
 *       seconds, or “No LED” to disable the LED entirely.
 *      The second screen, titled “blu” or “grn”, sets the intensity of the
 *       blue or green LED depending on the target Sensor Board hardware.
 *       Values range from 0 (off) to 15 (full intensity).
 *      The third screen, “red”, sets the intensity of the red LED, again
 *       from 0 to 15.
 *      On the last two screens, the LED remains on so that you can see the
 *      effect of mixing the two LED colors. On the Special Edition boards,
 *      you’ll have red, blue and a variety of shades of pink and purple to
 *      experiment with!
 */

#include "movement.h"

typedef enum {
    PREFERENCES_PAGE_CLOCK_MODE = 0,
    PREFERENCES_PAGE_BUTTON_SOUND,
    PREFERENCES_PAGE_TIMEOUT,
    PREFERENCES_PAGE_LOW_ENERGY,
    PREFERENCES_PAGE_LED_DURATION,
    PREFERENCES_PAGE_LED_RED,
    PREFERENCES_PAGE_LED_GREEN,
    PREFERENCES_PAGE_LED_BLUE,
    PREFERENCES_PAGE_NUM_PREFERENCES,
} preferences_page_t;


typedef struct {
    bool setting_enabled[PREFERENCES_PAGE_NUM_PREFERENCES];
    preferences_page_t current_page;
} preferences_state_t;

void preferences_face_setup(uint8_t watch_face_index, void ** context_ptr);
void preferences_face_activate(void *context);
bool preferences_face_loop(movement_event_t event, void *context);
void preferences_face_resign(void *context);

#define preferences_face ((const watch_face_t){ \
    preferences_face_setup, \
    preferences_face_activate, \
    preferences_face_loop, \
    preferences_face_resign, \
    NULL, \
})
