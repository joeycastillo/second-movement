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

#ifdef USE_CUSTOM_LCD
static const uint32_t IndicatorSegments[] = {
    SLCD_SEGID(0, 21), // WATCH_INDICATOR_SIGNAL
    SLCD_SEGID(1, 21), // WATCH_INDICATOR_BELL
    SLCD_SEGID(3, 21), // WATCH_INDICATOR_PM
    SLCD_SEGID(2, 21), // WATCH_INDICATOR_24H
    SLCD_SEGID(1,  0), // WATCH_INDICATOR_LAP
    SLCD_SEGID(2,  0), // WATCH_INDICATOR_BATTERY
    SLCD_SEGID(3,  0),  // WATCH_INDICATOR_SLEEP
};
#else
static const uint32_t IndicatorSegments[] = {
    SLCD_SEGID(0, 17), // WATCH_INDICATOR_SIGNAL
    SLCD_SEGID(0, 16), // WATCH_INDICATOR_BELL
    SLCD_SEGID(2, 17), // WATCH_INDICATOR_PM
    SLCD_SEGID(2, 16), // WATCH_INDICATOR_24H
    SLCD_SEGID(1, 10), // WATCH_INDICATOR_LAP

    // Aliases for indicators unavailable on the original F-91W LCD
    SLCD_SEGID(1, 10), // WATCH_INDICATOR_BATTERY (same as LAP)
    SLCD_SEGID(4, 0),  // WATCH_INDICATOR_SLEEP (does not exist, will set in SDATAL4 which is harmless)
};

#endif

void watch_display_character(uint8_t character, uint8_t position) {
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

    digit_mapping_t segmap = Watch_Display_Mapping[position];
    uint8_t segdata = Watch_Character_Set[character - 0x20];

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

    digit_mapping_t segmap = Watch_Display_Mapping[position];
    uint8_t segdata = Watch_Character_Set[character - 0x20];

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

void watch_display_string(char *string, uint8_t position) {
    size_t i = 0;
    while(string[i] != 0) {
        watch_display_character(string[i], position + i);
        i++;
        if (position + i >= 10) break;
    }
}

void watch_display_top_left(char *string) {
    watch_display_character(string[0], 0);
    if (string[1]) {
        watch_display_character(string[1], 1);
    }
}

void watch_display_top_right(char *string) {
    watch_display_character(string[0], 2);
    if (string[1]) {
        watch_display_character(string[1], 3);
    }
}

void watch_display_main_line(char *string) {
    int i = 0;
    while (string[i] != 0) {
        watch_display_character(string[i], 4 + i);
        i++;
    }
}

void watch_set_colon(void) {
#ifdef USE_CUSTOM_LCD
    watch_set_pixel(0, 0);
#else
    watch_set_pixel(1, 16);
#endif
}

void watch_clear_colon(void) {
#ifdef USE_CUSTOM_LCD
    watch_clear_pixel(0, 0);
#else
    watch_clear_pixel(1, 16);
#endif
}

void watch_set_indicator(WatchIndicatorSegment indicator) {
    uint32_t value = IndicatorSegments[indicator];
    uint8_t com = SLCD_COMNUM(value);
    uint8_t seg = SLCD_SEGNUM(value);
    watch_set_pixel(com, seg);
}

void watch_clear_indicator(WatchIndicatorSegment indicator) {
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
    watch_clear_indicator(WATCH_INDICATOR_BATTERY);
    watch_clear_indicator(WATCH_INDICATOR_SLEEP);
}
