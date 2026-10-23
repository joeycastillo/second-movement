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

#include "watch_slcd.h"
#include "watch_common_display.h"

#include <emscripten.h>
#include <emscripten/html5.h>

//////////////////////////////////////////////////////////////////////////////////////////
// Segmented Display

static char blink_character;
static bool blink_state;
static long blink_interval_id = - 1;
static bool tick_state;
static long tick_interval_id = -1;
static uint8_t indicator_blink_com;
static bool indicator_blink_state;
static long indicator_blink_interval_id = -1;

watch_lcd_type_t watch_get_lcd_type(void) {
#if defined(FORCE_CUSTOM_LCD_TYPE)
    return WATCH_LCD_TYPE_CUSTOM;
#else
    return WATCH_LCD_TYPE_CLASSIC;
#endif
}

void watch_enable_display(void) {
#if defined(FORCE_CUSTOM_LCD_TYPE)
    _watch_update_indicator_segments();
#endif

#if defined(FORCE_CUSTOM_LCD_TYPE)
    EM_ASM({document.getElementById("custom").style.display = "";});
    EM_ASM({document.getElementById("classic").style.display = "none";});
#else
    EM_ASM({document.getElementById("custom").style.display = "none";});
    EM_ASM({document.getElementById("classic").style.display = "";});
#endif

    watch_clear_display();
}

void watch_disable_display(void) {
    watch_clear_display();
    EM_ASM({document.getElementById("classic").style.display = "none";});
    EM_ASM({document.getElementById("custom").style.display = "none";});
}

void watch_set_pixel(uint8_t com, uint8_t seg) {
    EM_ASM({
        document.querySelectorAll("[data-com='" + $0 + "'][data-seg='" + $1 + "']")
            .forEach((e) => e.style.opacity = 1);
    }, com, seg);
}

void watch_clear_pixel(uint8_t com, uint8_t seg) {
    EM_ASM({
        document.querySelectorAll("[data-com='" + $0 + "'][data-seg='" + $1 + "']")
            .forEach((e) => e.style.opacity = 0);
    }, com, seg);
}

void watch_clear_display(void) {
    EM_ASM({
        document.querySelectorAll("[data-com][data-seg]")
            .forEach((e) => e.style.opacity = 0);
    });
}

static void watch_invoke_blink_callback(void *userData) {
    blink_state = !blink_state;
    watch_display_character(blink_state ? blink_character : ' ', 7);
    watch_clear_pixel(2, 10); // clear segment B of position 7 since it can't blink
}

void watch_start_character_blink(char character, uint32_t duration) {
    if (blink_interval_id != -1) return;
    watch_display_character(character, 7);
    watch_clear_pixel(2, 10); // clear segment B of position 7 since it can't blink

    blink_state = true;
    blink_character = character;
    blink_interval_id = emscripten_set_interval(watch_invoke_blink_callback, (double)duration, NULL);
}

static void watch_invoke_indicator_blink_callback(void *userData) {
    indicator_blink_state = !indicator_blink_state;
    // on hardware, indicator blink toggles segment 0 of the selected common
    if (indicator_blink_state) {
        watch_set_pixel(indicator_blink_com, 0);
    } else {
        watch_clear_pixel(indicator_blink_com, 0);
    }
}

void watch_start_indicator_blink_if_possible(watch_indicator_t indicator, uint32_t duration) {
    // Indicators can only blink on the custom LCD.
    if (watch_get_lcd_type() != WATCH_LCD_TYPE_CUSTOM) return;

    switch (indicator) {
    case WATCH_INDICATOR_COLON:
        indicator_blink_com = 0;
        break;
    case WATCH_INDICATOR_LAP:
        indicator_blink_com = 1;
        break;
    case WATCH_INDICATOR_ARROWS:
        indicator_blink_com = 2;
        break;
    case WATCH_INDICATOR_SLEEP:
        indicator_blink_com = 3;
        break;
    default:
        return;
    }

    if (indicator_blink_interval_id != -1) emscripten_clear_interval(indicator_blink_interval_id);
    watch_set_pixel(indicator_blink_com, 0);
    indicator_blink_state = true;
    indicator_blink_interval_id = emscripten_set_interval(watch_invoke_indicator_blink_callback, (double)duration, NULL);
}

void watch_stop_blink(void) {
    emscripten_clear_interval(blink_interval_id);
    blink_interval_id = -1;
    blink_state = false;

    if (indicator_blink_interval_id != -1) {
        emscripten_clear_interval(indicator_blink_interval_id);
        indicator_blink_interval_id = -1;
        // the segment stays set after blinking stops, like on hardware
        watch_set_pixel(indicator_blink_com, 0);
    }
    indicator_blink_state = false;
}

static void watch_invoke_tick_callback(void *userData) {
    tick_state = !tick_state;
    if (tick_state) {
        watch_clear_pixel(0, 2);
        watch_set_pixel(0, 3);
    } else {
        watch_clear_pixel(0, 3);
        watch_set_pixel(0, 2);
    }
}

void watch_start_sleep_animation(uint32_t duration) {
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        // on pro LCD, we just show the sleep indicator
        watch_set_indicator(WATCH_INDICATOR_SLEEP);
    } else {
        // on classic LCD we do the "tick/tock" animation
        if (tick_interval_id != -1) return;
        watch_display_character(' ', 8);
        watch_display_character(' ', 9);

        tick_state = true;
        tick_interval_id = emscripten_set_interval(watch_invoke_tick_callback, (double)duration, NULL);
    }
}

bool watch_sleep_animation_is_running(void) {
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        // COM3, SEG0 contains the half moon icon
        return EM_ASM_INT({
            const e = document.querySelector("[data-com='3'][data-seg='0']");
            return e && e.style.opacity == 1 ? 1 : 0;
        });
    } else {
        return tick_interval_id != -1;
    }
}

void watch_stop_sleep_animation(void) {
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        watch_clear_indicator(WATCH_INDICATOR_SLEEP);
    } else {
        emscripten_clear_interval(tick_interval_id);
        tick_interval_id = -1;
        tick_state = false;

        watch_display_character(' ', 8);
    }
}
