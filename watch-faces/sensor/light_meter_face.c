/*
 * MIT License
 *
 * Copyright (c) 2025 Joey Castillo
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

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "light_meter_face.h"
#include "tc.h"
#include "eic.h"
#include "usb.h"
#include "adc.h"

#ifdef HAS_IR_SENSOR

static void _light_meter_face_update_display(light_meter_state_t *state, uint8_t subsecond) {
    if (state->mode >= LIGHT_METER_MODE_AP_IS_SETTING_ISO) {
        // Handle ISO settings mode.
        watch_clear_display();
        // Custom LCD can say "ISO". On Classic, we can't show an S in position 1, so "FI" for FIlm speed will have to suffice.
        watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "ISO", "FI");

        if (subsecond % 2) {
            // for speeds above 50 ISO, last two digits will always be 00.
            if (state->iso >= LIGHT_METER_ISO_100) {
                watch_display_text(WATCH_POSITION_MINUTES, "00");
            }

            switch (state->iso) {
                case LIGHT_METER_ISO_25:
                    watch_display_text(WATCH_POSITION_MINUTES, "25");
                    break;
                case LIGHT_METER_ISO_50:
                    watch_display_text(WATCH_POSITION_MINUTES, "50");
                    break;
                case LIGHT_METER_ISO_100:
                    watch_display_text(WATCH_POSITION_HOURS, " 1");
                    break;
                case LIGHT_METER_ISO_200:
                    watch_display_text(WATCH_POSITION_HOURS, " 2");
                    break;
                case LIGHT_METER_ISO_400:
                    watch_display_text(WATCH_POSITION_HOURS, " 4");
                    break;
                case LIGHT_METER_ISO_800:
                    watch_display_text(WATCH_POSITION_HOURS, " 8");
                    break;
                case LIGHT_METER_ISO_1600:
                    watch_display_text(WATCH_POSITION_HOURS, "16");
                    break;
                case LIGHT_METER_ISO_3200:
                    watch_display_text(WATCH_POSITION_HOURS, "32");
                    break;
                case LIGHT_METER_ISO_COUNT:
                    // will not reach here
                    break;
            }
        }
    } else {
        // we are in aperture or shutter priority mode.
        light_meter_aperture_t aperture;
        light_meter_shutter_speed_t shutter;
        uint16_t light_level;

        if (!adc_is_enabled()) {
            /// TODO: need to test this more. If a background task takes a reading and disables the ADC,
            // we might hang trying to get an analog value below? So enable it just in case?
            // Test and confirm this works before removing this comment.
            adc_enable();
            return;
        }

        light_level = adc_get_analog_value(HAL_GPIO_IRSENSE_pin());

        /// FIXME: This curve is garbage, but in theory it was meant to convert the light level to an exposure index at ISO 25.
        // Readings were taken with the custom LCD and the standard Casio light spreader in place.
        // PLENTY of room for improvement here!
        const float L = 63188.86f;
        const float k = 0.8654f;
        const float x0 = 7.45f;
        const float C = 2491.21f;
        float exposure_index = x0 + (1.0f / k) * log((L / ((float)light_level - C)) - 1);
        if (isnan(exposure_index)) exposure_index = 0;

        int8_t target_exposure_index_at_f1_or_1s = (int8_t) round(exposure_index) + state->iso;

        if (state->mode == LIGHT_METER_MODE_APERTURE_PRIORITY) {
            aperture = state->aperture_priority;
            shutter = target_exposure_index_at_f1_or_1s - (state->aperture_priority - LIGHT_METER_APERTURE_F1);
        } else {
            shutter = state->shutter_priority;
            aperture = target_exposure_index_at_f1_or_1s - (state->shutter_priority - LIGHT_METER_SHUTTER_1_SEC);
        }
        if (shutter < LIGHT_METER_SHUTTER_1_SEC) shutter = LIGHT_METER_SHUTTER_1_SEC;
        if (aperture < LIGHT_METER_APERTURE_F1) aperture = LIGHT_METER_APERTURE_F1;

        // F stop is shown in both AP and SP modes.
        watch_clear_display();
        watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, " F/", " F");

        switch (aperture) {
            case LIGHT_METER_APERTURE_F1:
                watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
                watch_display_text_with_fallback(WATCH_POSITION_TOP, "TooLo", "LO");
                break;
            case LIGHT_METER_APERTURE_F1_4:
                watch_display_text(WATCH_POSITION_TOP_RIGHT, "14");
                break;
            case LIGHT_METER_APERTURE_F2:
                watch_display_text(WATCH_POSITION_TOP_RIGHT, "2 ");
                break;
            case LIGHT_METER_APERTURE_F2_8:
                watch_display_text(WATCH_POSITION_TOP_RIGHT, "28");
                break;
            case LIGHT_METER_APERTURE_F4:
                // classic LCD cannot display 4 in the left digit.
                watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, "4 ", " 4");
                break;
            case LIGHT_METER_APERTURE_F5_6:
                // alas this is the goofy one on classic LCD: 5 will look a bit like a cursed sigil here.
                watch_display_text(WATCH_POSITION_TOP_RIGHT, "56");
                break;
            case LIGHT_METER_APERTURE_F8:
                watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, "8 ", " 8");
                break;
            case LIGHT_METER_APERTURE_F11:
                watch_display_text(WATCH_POSITION_TOP_RIGHT, "11");
                break;
            case LIGHT_METER_APERTURE_F16:
                watch_display_text(WATCH_POSITION_TOP_RIGHT, "16");
                break;
            case LIGHT_METER_APERTURE_F22:
                watch_display_text(WATCH_POSITION_TOP_RIGHT, "22");
                break;
            case LIGHT_METER_APERTURE_F32:
                watch_display_text(WATCH_POSITION_TOP_RIGHT, "32");
                break;
            default:
                watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "too HI", "HIGH  ");
                break;
        }

        // th at bottom is also shown always
        watch_display_text(WATCH_POSITION_SECONDS, "th");
        if (shutter >= LIGHT_METER_SHUTTER_1_500) {
            // over a 500th of a second, last two digits are always 00
            watch_display_text(WATCH_POSITION_MINUTES, "00");
        }

        switch (shutter) {
            case LIGHT_METER_SHUTTER_1_SEC:
            case LIGHT_METER_SHUTTER_1_2:
                watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "tooLOw", " LO   ");
                break;
            case LIGHT_METER_SHUTTER_1_4:
                watch_display_text(WATCH_POSITION_MINUTES, " 4");
                break;
            case LIGHT_METER_SHUTTER_1_8:
                watch_display_text(WATCH_POSITION_MINUTES, " 8");
                break;
            case LIGHT_METER_SHUTTER_1_15:
                watch_display_text(WATCH_POSITION_MINUTES, "15");
                break;
            case LIGHT_METER_SHUTTER_1_30:
                watch_display_text(WATCH_POSITION_MINUTES, "30");
                break;
            case LIGHT_METER_SHUTTER_1_60:
                watch_display_text(WATCH_POSITION_MINUTES, "60");
                break;
            case LIGHT_METER_SHUTTER_1_125:
                watch_display_text(WATCH_POSITION_HOURS, " 1");
                watch_display_text(WATCH_POSITION_MINUTES, "25");
                break;
            case LIGHT_METER_SHUTTER_1_250:
                watch_display_text(WATCH_POSITION_HOURS, " 2");
                watch_display_text(WATCH_POSITION_MINUTES, "50");
                break;
            case LIGHT_METER_SHUTTER_1_500:
                watch_display_text(WATCH_POSITION_HOURS, " 5");
                break;
            case LIGHT_METER_SHUTTER_1_1000:
                watch_display_text(WATCH_POSITION_HOURS, "10");
                break;
            case LIGHT_METER_SHUTTER_1_2000:
                watch_display_text(WATCH_POSITION_HOURS, "20");
                break;
            case LIGHT_METER_SHUTTER_1_4000:
                watch_display_text(WATCH_POSITION_HOURS, "40");
                break;
            default:
                watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "too HI", "HIGH  ");
                break;
        }
    }
}

void light_meter_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(light_meter_state_t));
        memset(*context_ptr, 0, sizeof(light_meter_state_t));
        light_meter_state_t *state = (light_meter_state_t *)*context_ptr;
        state->iso = LIGHT_METER_ISO_100;
        state->mode = LIGHT_METER_MODE_APERTURE_PRIORITY;
        state->aperture_priority = LIGHT_METER_APERTURE_F8;
        state->shutter_priority = LIGHT_METER_SHUTTER_1_250;
    }    
}

void light_meter_face_activate(void *context) {
    light_meter_state_t *state = (light_meter_state_t *)context;
    (void) state;
    HAL_GPIO_IR_ENABLE_out();
    HAL_GPIO_IR_ENABLE_clr();
    HAL_GPIO_IRSENSE_pmuxen(HAL_GPIO_PMUX_ADC);
    adc_init();
    adc_enable();
    movement_request_tick_frequency(4);
}

bool light_meter_face_loop(movement_event_t event, void *context) {
    light_meter_state_t *state = (light_meter_state_t *)context;
    (void) state;

    switch (event.event_type) {
        case EVENT_NONE:
        case EVENT_ACTIVATE:
        case EVENT_TICK:
        {
            _light_meter_face_update_display(state, event.subsecond);
        }
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            // suppress LED, as it would interfere with light sensing
            break;
        case EVENT_LIGHT_BUTTON_UP:
            switch (state->mode) {
                case LIGHT_METER_MODE_APERTURE_PRIORITY:
                    // in AP mode, move to SP
                    state->mode = LIGHT_METER_MODE_SHUTTER_PRIORITY;
                    break;
                case LIGHT_METER_MODE_SHUTTER_PRIORITY:
                    // in SP mode, move to AP
                    state->mode = LIGHT_METER_MODE_APERTURE_PRIORITY;
                    break;
                case LIGHT_METER_MODE_AP_IS_SETTING_ISO:
                    // if we are setting ISO, return to the mode we were in before
                    state->mode = LIGHT_METER_MODE_APERTURE_PRIORITY;
                    break;
                case LIGHT_METER_MODE_SP_IS_SETTING_ISO:
                    state->mode = LIGHT_METER_MODE_SHUTTER_PRIORITY;
                    break;
            }
            _light_meter_face_update_display(state, event.subsecond);
            break;
        case EVENT_ALARM_BUTTON_UP:
            switch (state->mode) {
                case LIGHT_METER_MODE_APERTURE_PRIORITY:
                    state->aperture_priority = (state->aperture_priority + 1) % LIGHT_METER_APERTURE_COUNT;
                    break;
                case LIGHT_METER_MODE_SHUTTER_PRIORITY:
                    state->shutter_priority = (state->shutter_priority + 1) % LIGHT_METER_SHUTTER_COUNT;
                    break;
                case LIGHT_METER_MODE_AP_IS_SETTING_ISO:
                case LIGHT_METER_MODE_SP_IS_SETTING_ISO:
                    state->iso = (state->iso + 1) % LIGHT_METER_ISO_COUNT;
                    break;
            }
             _light_meter_face_update_display(state, event.subsecond);
           break;
        case EVENT_ALARM_LONG_PRESS:
            switch (state->mode) {
                case LIGHT_METER_MODE_APERTURE_PRIORITY:
                case LIGHT_METER_MODE_SHUTTER_PRIORITY:
                    // long press in either aperture or shutter priority mode sets the ISO.
                    state->mode = state->mode + 2;
                    break;
                case LIGHT_METER_MODE_AP_IS_SETTING_ISO:
                case LIGHT_METER_MODE_SP_IS_SETTING_ISO:
                    state->mode = state->mode - 2;
                    break;
            }
            _light_meter_face_update_display(state, event.subsecond);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            if (!watch_sleep_animation_is_running()) {
                // start the animation
                watch_clear_display();
                watch_start_sleep_animation(1000);
                // we are not going to sense light in low energy mode, it's a waste of battery.
                watch_display_text_with_fallback(WATCH_POSITION_TOP, "Meter", "MT");
                watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "OFF   ", "OF F  ");
                // turn off our power hungry sensors
                adc_disable();
                HAL_GPIO_IRSENSE_pmuxdis();
                HAL_GPIO_IRSENSE_off();
                HAL_GPIO_IR_ENABLE_off();
            }
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return false;
}

void light_meter_face_resign(void *context) {
    (void) context;

    adc_disable();
    HAL_GPIO_IRSENSE_pmuxdis();
    HAL_GPIO_IRSENSE_off();
    HAL_GPIO_IR_ENABLE_off();
}

#endif // HAS_IR_SENSOR
