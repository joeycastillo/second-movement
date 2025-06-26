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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

uint8_t IndicatorSegments[8] = {
    SLCD_SEGID(0, 17), // WATCH_INDICATOR_SIGNAL
    SLCD_SEGID(0, 16), // WATCH_INDICATOR_BELL
    SLCD_SEGID(2, 17), // WATCH_INDICATOR_PM
    SLCD_SEGID(2, 16), // WATCH_INDICATOR_24H
    SLCD_SEGID(1, 10), // WATCH_INDICATOR_LAP

    // Placeholders for indicators unavailable on the original F-91W LCD
    SLCD_SEGID(4, 0),  // WATCH_INDICATOR_ARROWS (does not exist, will set in SDATAL4 which is harmless)
    SLCD_SEGID(4, 0),  // WATCH_INDICATOR_SLEEP (does not exist, will set in SDATAL4 which is harmless)
    SLCD_SEGID(4, 0)   // WATCH_INDICATOR_COLON (does not exist, will set in SDATAL4 which is harmless)
};

void watch_display_character(uint8_t character, uint8_t position) {
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        if (character == 'R' && position > 1 && position < 8) character = 'r'; // We can't display uppercase R in these positions
        else if (character == 'T' && position > 1 && position < 8) character = 't'; // lowercase t is the only option for these positions
    } else {
        // special cases for positions 4 and 6
        if (position == 4 || position == 6) {
            if (character == '7') character = '&'; // "lowercase" 7
            else if (character == 'A') character = 'a'; // A needs to be lowercase
            else if (character == 'o') character = 'O'; // O needs to be uppercase
            else if (character == 'L') character = '!'; // L needs to be in top half
            else if (character == 'M' || character == 'm' || character == 'N') character = 'n'; // M and uppercase N need to be lowercase n
            else if (character == 'c') character = 'C'; // C needs to be uppercase
            else if (character == 'J') character = 'j'; // same
            else if (character == 'v' || character == 'V' || character == 'U' || character == 'W' || character == 'w') character = 'u'; // bottom segment duplicated, so show in top half
        } else {
            if (character == 'u') character = 'v'; // we can use the bottom segment; move to lower half
            else if (character == 'j') character = 'J'; // same but just display a normal J
            else if (character == '.') character = '_'; // we can use the bottom segment; make dot an underscore
        }
        if (position > 1) {
            if (character == 'T') character = 't'; // uppercase T only works in positions 0 and 1
        }
        if (position == 1) {
            if (character == 'a') character = 'A'; // A needs to be uppercase
            else if (character == 'o') character = 'O'; // O needs to be uppercase
            else if (character == 'i') character = 'l'; // I needs to be uppercase (use an l, it looks the same)
            else if (character == 'n') character = 'N'; // N needs to be uppercase
            else if (character == 'r') character = 'R'; // R needs to be uppercase
            else if (character == 'd') character = 'D'; // D needs to be uppercase
            else if (character == 'v' || character == 'V' || character == 'u') character = 'U'; // side segments shared, make uppercase
            else if (character == 'b') character = 'B'; // B needs to be uppercase
            else if (character == 'c') character = 'C'; // C needs to be uppercase
        } else {
            if (character == 'R') character = 'r'; // R needs to be lowercase almost everywhere
        }
        if (position == 0) {
            watch_clear_pixel(0, 15); // clear funky ninth segment
        } else {
            if (character == 'I') character = 'l'; // uppercase I only works in position 0
        }
    }

    digit_mapping_t segmap;
    uint8_t segdata;

    /// TODO: This could be optimized by doing this check once and setting a pointer in watch_discover_lcd_type.

    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        segmap = Custom_LCD_Display_Mapping[position];
        segdata = Custom_LCD_Character_Set[character - 0x20];
    } else {
        segmap = Classic_LCD_Display_Mapping[position];
        segdata = Classic_LCD_Character_Set[character - 0x20];
    }

    for (int i = 0; i < 8; i++) {
        if (segmap.segment[i].value == segment_does_not_exist) {
            // Segment does not exist; skip it.
            segdata = segdata >> 1;
            continue;
        }
        uint8_t com = segmap.segment[i].address.com;
        uint8_t seg = segmap.segment[i].address.seg;

        if (segdata & 1) {
            watch_set_pixel(com, seg);
        }
        else {
            watch_clear_pixel(com, seg);
        }

        segdata = segdata >> 1;
    }

    if (character == 'T' && position == 1) watch_set_pixel(1, 12); // add descender
    else if (position == 0 && (character == 'B' || character == 'D' || character == '@')) watch_set_pixel(0, 15); // add funky ninth segment
    else if (position == 1 && (character == 'B' || character == 'D' || character == '@')) watch_set_pixel(0, 12); // add funky ninth segment
}

void watch_display_character_lp_seconds(uint8_t character, uint8_t position) {
    // Will only work for digits and for positions  8 and 9 - but less code & checks to reduce power consumption

    digit_mapping_t segmap;
    uint8_t segdata;

    /// TODO: See optimization note above.

    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        segmap = Custom_LCD_Display_Mapping[position];
        segdata = Custom_LCD_Character_Set[character - 0x20];
    } else {
        segmap = Classic_LCD_Display_Mapping[position];
        segdata = Classic_LCD_Character_Set[character - 0x20];
    }

    for (int i = 0; i < 8; i++) {
        if (segmap.segment[i].value == segment_does_not_exist) {
            // Segment does not exist; skip it.
            segdata = segdata >> 1;
            continue;
        }
        uint8_t com = segmap.segment[i].address.com;
        uint8_t seg = segmap.segment[i].address.seg;

        if (segdata & 1) {
            watch_set_pixel(com, seg);
        }
        else {
            watch_clear_pixel(com, seg);
        }

        segdata = segdata >> 1;
    }
}

void watch_display_string(const char *string, uint8_t position) {
    size_t i = 0;
    while(string[i] != 0) {
        watch_display_character(string[i], position + i);
        i++;
        if (position + i >= 10) break;
    }
}

void watch_display_text(watch_position_t location, const char *string) {
    switch (location) {
        case WATCH_POSITION_TOP:
        case WATCH_POSITION_TOP_LEFT:
            watch_display_character(string[0], 0);
            if (string[1]) {
                watch_display_character(string[1], 1);
            }
            break;
        case WATCH_POSITION_TOP_RIGHT:
            watch_display_character(string[0], 2);
            if (string[1]) {
                watch_display_character(string[1], 3);
            }
            break;
        case WATCH_POSITION_BOTTOM:
            {
                if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
                    watch_clear_pixel(0, 22);
                }
                int i = 0;
                while (string[i] != 0) {
                    watch_display_character(string[i], 4 + i);
                    i++;
                }
        }
            break;
        case WATCH_POSITION_HOURS:
            watch_display_character(string[0], 4);
            if (string[1]) {
                watch_display_character(string[1], 5);
            }
            break;
        case WATCH_POSITION_MINUTES:
            watch_display_character(string[0], 6);
            if (string[1]) {
                watch_display_character(string[1], 7);
            }
            break;
        case WATCH_POSITION_SECONDS:
            watch_display_character(string[0], 8);
            if (string[1]) {
                watch_display_character(string[1], 9);
            }
            break;
        case WATCH_POSITION_FULL:
            // This is deprecated, but we use it for the legacy behavior.
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            watch_display_string(string, 0);
            #pragma GCC diagnostic pop
            if (watch_get_lcd_type()  == WATCH_LCD_TYPE_CUSTOM) {
                if (strlen(string) >= 11) watch_display_character(string[10], 10);
                else watch_display_character(' ', 10);
            }
    }
}

void watch_display_text_with_fallback(watch_position_t location, const char *string, const char *fallback) {
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        switch (location) {
            case WATCH_POSITION_TOP:
                for (size_t i = 0; i < strlen(string); i++) {
                    if (i < 2) watch_display_character(string[i], i);
                    else if (i == 2) watch_display_character(string[i], 10);
                    else if (i < 5) watch_display_character(string[i], i - 1);
                    else break;
                }
                break;
            case WATCH_POSITION_TOP_LEFT:
                watch_display_character(string[0], 0);
                if (string[1]) {
                    watch_display_character(string[1], 1);
                } else {
                    return;
                }
                if (string[2]) {
                    // position 3 is at index 10 in the display mapping
                    watch_display_character(string[2], 10);
                }
                break;
            case WATCH_POSITION_BOTTOM:
            {
                watch_clear_pixel(0, 22);
                int i = 0;
                int offset = 0;
                size_t len = strlen(string);
                if (len == 7 && string[0] == '1') {
                    watch_set_pixel(0, 22);
                    offset = 1;
                    i++;
                }
                while (string[i] != 0) {
                    if (4 + i - offset == 10) break;
                    watch_display_character(string[i], 4 + i - offset);
                    i++;
                }
            }
                break;
            case WATCH_POSITION_TOP_RIGHT:
            case WATCH_POSITION_HOURS:
            case WATCH_POSITION_MINUTES:
            case WATCH_POSITION_SECONDS:
            case WATCH_POSITION_FULL:
                watch_display_text(location, string);
                break;
        }
    } else {
        watch_display_text(location, fallback);
    }
}

void watch_display_float_with_best_effort(float value, const char *units) {
    char buf[8];
    char buf_fallback[8];
    const char *blank_units = "  ";

    if (value < -99.9) {
        watch_clear_decimal_if_available();
        watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "Undflo", " Unflo");
        return;
    } else if (value > 199.99) {
        watch_clear_decimal_if_available();
        watch_display_text(WATCH_POSITION_BOTTOM, "Ovrflo");
        return;
    }

    uint16_t value_times_100 = abs((int)round(value * 100.0));
    bool set_decimal = true;

    if (value < 0 && value_times_100 != 0) {
        if (value_times_100 > 999) {
            // decimal point isn't in the right place for these numbers; use same format as classic.
            set_decimal = false;
            snprintf(buf, sizeof(buf), "-%4.1f%s", -value, units ? units : blank_units);
            snprintf(buf_fallback, sizeof(buf_fallback), "%s", buf);
        } else {
            snprintf(buf, sizeof(buf), "-%03d%s", value_times_100 % 1000u, units ? units : blank_units);
            snprintf(buf_fallback, sizeof(buf_fallback), "-%3.1f%s", -value, units ? units : blank_units);
        }
    } else if (value_times_100 > 9999) {
        snprintf(buf, sizeof(buf), "%5u%s", value_times_100, units ? units : blank_units);
        snprintf(buf_fallback, sizeof(buf_fallback), "%4.1f%s", value, units ? units : blank_units);
    } else if (value_times_100 > 999) {
        snprintf(buf, sizeof(buf), "%4u%s", value_times_100, units ? units : blank_units);
        snprintf(buf_fallback, sizeof(buf_fallback), "%4.1f%s", value, units ? units : blank_units);
    } else {
        snprintf(buf, sizeof(buf), " %03u%s", value_times_100 % 1000u, units ? units : blank_units);
        snprintf(buf_fallback, sizeof(buf_fallback), "%4.2f%s", value, units ? units : blank_units);
    }

    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf_fallback);
    if (set_decimal) {
        watch_set_decimal_if_available();
    } else {
        watch_clear_decimal_if_available();
    }
}

void watch_set_colon(void) {
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        watch_set_pixel(0, 0);
    } else {
        watch_set_pixel(1, 16);
    }
}

void watch_clear_colon(void) {
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        watch_clear_pixel(0, 0);
    } else {
        watch_clear_pixel(1, 16);
    }
}

void watch_set_decimal_if_available(void) {
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        watch_set_pixel(0, 14);
    }
}

void watch_clear_decimal_if_available(void) {
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        watch_clear_pixel(0, 14);
    }
}

void watch_set_indicator(watch_indicator_t indicator) {
    uint32_t value = IndicatorSegments[indicator];
    uint8_t com = SLCD_COMNUM(value);
    uint8_t seg = SLCD_SEGNUM(value);
    watch_set_pixel(com, seg);
}

void watch_clear_indicator(watch_indicator_t indicator) {
    uint32_t value = IndicatorSegments[indicator];
    uint8_t com = SLCD_COMNUM(value);
    uint8_t seg = SLCD_SEGNUM(value);
    watch_clear_pixel(com, seg);
}

void watch_clear_all_indicators(void) {
    /// TODO: Optimize this? Can be 3-4 writes to SDATAL registers
    watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
    watch_clear_indicator(WATCH_INDICATOR_BELL);
    watch_clear_indicator(WATCH_INDICATOR_PM);
    watch_clear_indicator(WATCH_INDICATOR_24H);
    watch_clear_indicator(WATCH_INDICATOR_LAP);
    watch_clear_indicator(WATCH_INDICATOR_ARROWS);
    watch_clear_indicator(WATCH_INDICATOR_SLEEP);
}

void _watch_update_indicator_segments(void) {
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        IndicatorSegments[0] = SLCD_SEGID(0, 21); // WATCH_INDICATOR_SIGNAL
        IndicatorSegments[1] = SLCD_SEGID(1, 21); // WATCH_INDICATOR_BELL
        IndicatorSegments[2] = SLCD_SEGID(3, 21); // WATCH_INDICATOR_PM
        IndicatorSegments[3] = SLCD_SEGID(2, 21); // WATCH_INDICATOR_24H
        IndicatorSegments[4] = SLCD_SEGID(1,  0); // WATCH_INDICATOR_LAP
        IndicatorSegments[5] = SLCD_SEGID(2,  0); // WATCH_INDICATOR_ARROWS
        IndicatorSegments[6] = SLCD_SEGID(3,  0); // WATCH_INDICATOR_SLEEP
        IndicatorSegments[7] = SLCD_SEGID(4,  0); // WATCH_INDICATOR_SLEEP
    }
}
