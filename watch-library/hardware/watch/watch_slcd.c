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

 //////////////////////////////////////////////////////////////////////////////////////////
// Segmented Display

static uint16_t _slcd_framerate = 0;
static uint16_t _slcd_fc_min_ms_bypass = 0;

void watch_enable_display(void) {
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

    // Original famous Casio LCD: 1/3 bias, 1/3 duty with a frame rate of ~34 Hz
    slcd_init(LCD_PIN_ENABLE, SLCD_BIAS_THIRD, SLCD_DUTY_3_COMMON, SLCD_PRESCALER_DIV64, SLCD_CLOCKDIV_5);
    // exact frame rate is: 32768 / (3 * 64 * 5) ≈ 34.13 Hz
    _slcd_framerate = 34;
    // calculate the smallest duration we can time before we have to engage the frame counter prescaler bypass
    _slcd_fc_min_ms_bypass = 32 * (1000 / _slcd_framerate);

    slcd_clear();
    slcd_set_contrast(6);
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

    slcd_set_blink_enabled(false);
    slcd_configure_blink(false, 0x07, 0x07, 0);
    slcd_set_blink_enabled(true);
}

void watch_stop_blink(void) {
    slcd_set_frame_counter_enabled(0, false);
    slcd_set_blink_enabled(false);
}

void watch_start_tick_animation(uint32_t duration) {
    watch_display_character(' ', 8);

    slcd_set_frame_counter_enabled(1, false);
    slcd_set_circular_shift_animation_enabled(false);

    if (duration <= _slcd_fc_min_ms_bypass) {
        slcd_configure_frame_counter(1, (duration / (1000 / _slcd_framerate)) - 1, false);
    } else {
        slcd_configure_frame_counter(1, ((duration / (1000 / _slcd_framerate)) / 8 - 1), true);
    }
    slcd_set_frame_counter_enabled(1, true);

    slcd_configure_circular_shift_animation(0b00000001, 2, SLCD_CSRSHIFT_LEFT, 1);
    slcd_set_circular_shift_animation_enabled(true);
}

bool watch_tick_animation_is_running(void) {
    // TODO: wrap this in gossamer call
    return SLCD->CTRLD.bit.CSREN;
}

void watch_stop_tick_animation(void) {
    slcd_set_circular_shift_animation_enabled(false);
    watch_display_character(' ', 8);
}
