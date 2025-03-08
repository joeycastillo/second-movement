/*
 * MIT License
 *
 * Copyright (c) 2020-2024 Joey Castillo
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

#include "pins.h"
#include "watch_slcd.h"
#include "watch_common_display.h"
#include "slcd.h"
#include "tc.h"
#include "adc.h"

 //////////////////////////////////////////////////////////////////////////////////////////
// Segmented Display

static uint16_t _slcd_framerate = 0;
static uint16_t _slcd_fc_min_ms_bypass = 0;

static watch_lcd_type_t _installed_display = WATCH_LCD_TYPE_UNKNOWN;

void watch_discover_lcd_type(void) {
    #if defined(FORCE_CUSTOM_LCD_TYPE)
    _installed_display = WATCH_LCD_TYPE_CUSTOM;
    goto valid_display_detected;
    #elif defined(FORCE_CLASSIC_LCD_TYPE)
    _installed_display = WATCH_LCD_TYPE_CLASSIC;
    goto valid_display_detected;
    #endif

    // Don't bother detecting the LCD type if we're plugged into USB.
    if (usb_is_enabled()) return;
    
    uint16_t slcd3, slcd4;
    int difference;
    uint8_t valid_frames_classic = 0;
    uint8_t valid_frames_custom = 0;

    /// TODO: Test this at lower battery levels. Do the thresholds hold up?
    const uint16_t lo_threshold = 12000;
    const uint16_t hi_threshold = 32000;

    // taking advantage of the fact that LCD segments are like little capacitors,
    // we're going to introduce an alternating voltage on SLCD2, or COM2.
    HAL_GPIO_SLCD2_out();

    // Then we're going to read the voltage on SLCD3 and SLCD4:
    //  * On classic LCD, we will expect to see the voltage change on SLCD3 (SEG0) and SLCD4 (SEG1)
    //  * On custom LCD, we will expect to see the voltage change on SLCD4 (SEG1) but not SLCD3 (COM4)
    adc_init();
    adc_enable();
    HAL_GPIO_SLCD3_pmuxen(HAL_GPIO_PMUX_ADC);
    HAL_GPIO_SLCD4_pmuxen(HAL_GPIO_PMUX_ADC);

    /// TODO: Remove this and the watch_set_led_off below; it's here for testing (people will see red if their LCD was undetectable)
    watch_set_led_red();

    while(1) {
        HAL_GPIO_SLCD2_set();
        slcd3 = adc_get_analog_value(HAL_GPIO_SLCD3_pin());
        slcd4 = adc_get_analog_value(HAL_GPIO_SLCD4_pin());
        difference = abs((int)slcd4 - (int)slcd3);
        // printf("1, slcd3: %d, slcd4: %d, diff: %d\n", slcd3, slcd4, difference);
        if (slcd3 > hi_threshold && slcd4 > hi_threshold && abs(difference) < 1000) {
            valid_frames_classic++;
        } else if (slcd4 > hi_threshold && abs(difference) > 5000) {
            valid_frames_custom++;
        }
        delay_ms(4);

        HAL_GPIO_SLCD2_clr();
        slcd3 = adc_get_analog_value(HAL_GPIO_SLCD3_pin());
        slcd4 = adc_get_analog_value(HAL_GPIO_SLCD4_pin());
        difference = (int)slcd4 - (int)slcd3;
        // printf("0, slcd3: %d, slcd4: %d, diff: %d\n", slcd3, slcd4, difference);
        if (slcd3 < lo_threshold && slcd4 < lo_threshold && abs(difference) < 100) {
            valid_frames_classic++;
        } else if (slcd4 < lo_threshold && abs(difference) > 5000) {
            valid_frames_custom++;
        }

        if (valid_frames_classic > 16 || valid_frames_custom > 16) {
            break;
        }
        delay_ms(4);
    }

    watch_set_led_off();

    HAL_GPIO_SLCD2_off();
    HAL_GPIO_SLCD3_off();
    HAL_GPIO_SLCD4_off();
    adc_disable();

    if (valid_frames_classic > 16) {
        _installed_display = WATCH_LCD_TYPE_CLASSIC;
    } else if (valid_frames_custom > 16) {
        _installed_display = WATCH_LCD_TYPE_CUSTOM;
    }

valid_display_detected:
    _watch_update_indicator_segments();
}

watch_lcd_type_t watch_get_lcd_type(void) {
    return _installed_display;
}

void watch_enable_display(void) {
    watch_discover_lcd_type();

    HAL_GPIO_SLCD0_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD1_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD2_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD3_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD4_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD5_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD6_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD7_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD8_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD9_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD10_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD11_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD12_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD13_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD14_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD15_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD16_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD17_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD18_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD19_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD20_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD21_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD22_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD23_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD24_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD25_pmuxen(HAL_GPIO_PMUX_B);
    HAL_GPIO_SLCD26_pmuxen(HAL_GPIO_PMUX_B);

    if (_installed_display == WATCH_LCD_TYPE_CUSTOM) {
        // Custom LCD: 1/3 bias, 1/4 duty with a frame rate of 32 Hz
        slcd_init(LCD_PIN_ENABLE, SLCD_BIAS_THIRD, SLCD_DUTY_4_COMMON, SLCD_CLOCKSOURCE_XOSC, SLCD_PRESCALER_DIV64, SLCD_CLOCKDIV_4);
        // exact frame rate is: 32768 / (4 * 64 * 4) ≈ 32 Hz
        _slcd_framerate = 32;
    } else {
        // Original famous Casio LCD: 1/3 bias, 1/3 duty with a frame rate of ~34 Hz
        slcd_init(LCD_PIN_ENABLE, SLCD_BIAS_THIRD, SLCD_DUTY_3_COMMON, SLCD_CLOCKSOURCE_XOSC, SLCD_PRESCALER_DIV64, SLCD_CLOCKDIV_5);
        // exact frame rate is: 32768 / (3 * 64 * 5) ≈ 34.13 Hz
        _slcd_framerate = 34;
    }
    // calculate the smallest duration we can time before we have to engage the frame counter prescaler bypass
    _slcd_fc_min_ms_bypass = 32 * (1000 / _slcd_framerate);

    slcd_clear();

    if (_installed_display == WATCH_LCD_TYPE_CUSTOM) {
        slcd_set_contrast(6);
    } else {
        slcd_set_contrast(9);
    }

    slcd_enable();
}

inline void watch_set_pixel(uint8_t com, uint8_t seg) {
    slcd_set_segment(com, seg);
}

inline void watch_clear_pixel(uint8_t com, uint8_t seg) {
    slcd_clear_segment(com, seg);
}

void watch_clear_display(void) {
    slcd_clear();
}

void watch_start_character_blink(char character, uint32_t duration) {
    slcd_set_frame_counter_enabled(0, false);

    if (duration <= _slcd_fc_min_ms_bypass) {
        slcd_configure_frame_counter(0, (duration / (1000 / _slcd_framerate)) - 1, false);
    } else {
        slcd_configure_frame_counter(0, ((duration / (1000 / _slcd_framerate)) / 8 - 1), true);
    }
    slcd_set_frame_counter_enabled(0, true);

    watch_display_character(character, 7);
    watch_clear_pixel(2, 10); // clear segment B of position 7 since it can't blink

    slcd_disable();
    slcd_set_blink_enabled(false);
    slcd_configure_blink(false, 0x0F, 0x0F, 0);
    slcd_set_blink_enabled(true);
    slcd_enable();
}

void watch_start_indicator_blink_if_possible(watch_indicator_t indicator, uint32_t duration) {
    if (_installed_display == WATCH_LCD_TYPE_CUSTOM) {
        // Indicators can only blink on the custom LCD.
        uint8_t mask = 0;
        switch (indicator) {
        case WATCH_INDICATOR_COLON:
            mask = 0b0001;
            break;
        case WATCH_INDICATOR_LAP:
            mask = 0b0010;
            break;
        case WATCH_INDICATOR_BATTERY:
            mask = 0b0100;
            break;
        case WATCH_INDICATOR_SLEEP:
            mask = 0b1000;
            break;
        default:
            return;
        }
        watch_set_indicator(indicator);

        if (duration <= _slcd_fc_min_ms_bypass) {
            slcd_configure_frame_counter(0, (duration / (1000 / _slcd_framerate)) - 1, false);
        } else {
            slcd_configure_frame_counter(0, ((duration / (1000 / _slcd_framerate)) / 8 - 1), true);
        }
        slcd_set_frame_counter_enabled(0, true);


        slcd_disable();
        slcd_set_blink_enabled(false);
        slcd_configure_blink(false, mask, 0, 0);
        slcd_set_blink_enabled(true);
        slcd_enable();
    }
}

void watch_stop_blink(void) {
    slcd_set_frame_counter_enabled(0, false);
    slcd_set_blink_enabled(false);
}

void watch_start_sleep_animation(uint32_t duration) {
    if (_installed_display == WATCH_LCD_TYPE_CUSTOM) {
        // on pro LCD, we just show the sleep indicator
        watch_set_indicator(WATCH_INDICATOR_SLEEP);
    } else {
        // on classic LCD we do the "tick/tock" animation
        watch_display_character(' ', 8);
        watch_display_character(' ', 9);

        slcd_disable();
        slcd_set_frame_counter_enabled(1, false);
        slcd_set_circular_shift_animation_enabled(false);

        if (duration <= _slcd_fc_min_ms_bypass) {
            slcd_configure_frame_counter(1, (duration / (1000 / _slcd_framerate)) - 1, false);
        } else {
            slcd_configure_frame_counter(1, ((duration / (1000 / _slcd_framerate)) / 8 - 1), true);
        }
        slcd_set_frame_counter_enabled(1, true);

        slcd_configure_circular_shift_animation(0b00000001, 1, SLCD_CSRSHIFT_LEFT, 1);
        slcd_set_circular_shift_animation_enabled(true);
        slcd_enable();
    }
}

bool watch_sleep_animation_is_running(void) {
    // TODO: wrap this in gossamer call
    return SLCD->CTRLD.bit.CSREN;
}

void watch_stop_sleep_animation(void) {
    if (_installed_display == WATCH_LCD_TYPE_CUSTOM) {
        watch_clear_indicator(WATCH_INDICATOR_SLEEP);
    } else {
        slcd_set_circular_shift_animation_enabled(false);
        watch_display_character(' ', 8);
    }
}
