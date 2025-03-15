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
#pragma once

// Union representing a single segment mapping
// COM occupies two bits, SEG occupes the rest.
typedef union segment_mapping_t {
    struct {
        uint8_t com : 2;
        uint8_t seg : 6;
    } address;
    uint8_t value;
} segment_mapping_t;

// Value to indicate that a segment does not exist
static const uint8_t segment_does_not_exist = 0xff;

// Union representing 8 segment mappings, A-H
typedef union digit_mapping_t {
    segment_mapping_t segment[8];
    uint64_t value;
} digit_mapping_t;

// Custom extended LCD

// Character set is slightly different since we don't have to work around as much stuff.
static const uint8_t Custom_LCD_Character_Set[] =
{
    0b00000000, // [space]
    0b00111100, // ! L with an extra C segment (use !J to make a W)
    0b00100010, // "
    0b01100011, // # (degree symbol, hash mark doesn't fit)
    0b11101101, // $ (S with a downstroke)
    0b00000000, // % (unused)
    0b01000100, // & ("lowercase 7" for positions 4 and 6)
    0b00100000, // '
    0b00111001, // (
    0b00001111, // )
    0b11000000, // * (The + sign for use in position 0)
    0b01110000, // + (segments E, F and G; looks like ┣╸)
    0b00000100, // ,
    0b01000000, // -
    0b00001000, // . (same as _, semantically most useful)
    0b00010010, // /
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111, // 9
    0b00000000, // : (unused)
    0b00000000, // ; (unused)
    0b01011000, // <
    0b01001000, // =
    0b01001100, // >
    0b01010011, // ?
    0b11111111, // @ (all segments on)
    0b01110111, // A
    0b11001111, // B (with downstroke, only in weekday / seconds)
    0b00111001, // C
    0b10001111, // D (with downstroke, only in weekday / seconds)
    0b01111001, // E
    0b01110001, // F
    0b00111101, // G
    0b01110110, // H
    0b10001001, // I (only works in position 0)
    0b00011110, // J
    0b01110101, // K
    0b00111000, // L
    0b10110111, // M (only works in position 0)
    0b00110111, // N
    0b00111111, // O
    0b01110011, // P
    0b01100111, // Q
    0b11000111, // R
    0b01101101, // S
    0b10000001, // T (only works in position 0; set (1, 12) to make it work in position 1)
    0b00111110, // U
    0b00111110, // V
    0b10111110, // W (only works in position 0)
    0b11110110, // X
    0b01101110, // Y
    0b00011011, // Z
    0b00111001, // [
    0b00100100, // backslash
    0b00001111, // ]
    0b00100011, // ^
    0b00001000, // _
    0b00000010, // `
    0b01011111, // a
    0b01111100, // b
    0b01011000, // c
    0b01011110, // d
    0b01111011, // e
    0b01110001, // f
    0b01101111, // g
    0b01110100, // h
    0b00010000, // i
    0b00001110, // j
    0b01110101, // k
    0b00110000, // l
    0b10110111, // m (only works in position 0)
    0b01010100, // n
    0b01011100, // o
    0b01110011, // p
    0b01100111, // q
    0b01010000, // r
    0b01101101, // s
    0b01111000, // t
    0b00011100, // u
    0b00011100, // v (looks like u)
    0b10111110, // w
    0b01111110, // x
    0b01101110, // y
    0b00011011, // z
    0b00010110, // { (open brace doesn't really work; overriden to represent the two character ligature "il")
    0b00110110, // | (overriden to represent the two character ligature "ll")
    0b00110100, // } (overriden to represent the two character ligature "li")
    0b00000001, // ~
};

static const digit_mapping_t Custom_LCD_Display_Mapping[] = {
    {
        .segment = {
            { .address = { .com = 0, .seg = 19 } }, // 0A
            { .address = { .com = 2, .seg = 19 } }, // 0B
            { .address = { .com = 3, .seg = 19 } }, // 0C
            { .address = { .com = 3, .seg = 20 } }, // 0D
            { .address = { .com = 2, .seg = 20 } }, // 0E
            { .address = { .com = 0, .seg = 20 } }, // 0F
            { .address = { .com = 1, .seg = 20 } }, // 0G
            { .address = { .com = 1, .seg = 19 } }, // 0H
        },
    },
    {
        .segment = {
            { .address = { .com = 0, .seg = 17 } }, // 1A
            { .address = { .com = 2, .seg = 17 } }, // 1B
            { .address = { .com = 3, .seg = 17 } }, // 1C
            { .address = { .com = 3, .seg = 18 } }, // 1D
            { .address = { .com = 2, .seg = 18 } }, // 1E
            { .address = { .com = 0, .seg = 18 } }, // 1F
            { .address = { .com = 1, .seg = 18 } }, // 1G
            { .address = { .com = 1, .seg = 17 } }, // 1H
        },
    },
    {
        .segment = {
            { .address = { .com = 0, .seg = 11 } }, // 2A
            { .address = { .com = 0, .seg = 10 } }, // 2B
            { .address = { .com = 2, .seg = 10 } }, // 2C
            { .address = { .com = 3, .seg = 11 } }, // 2D
            { .address = { .com = 2, .seg = 11 } }, // 2E
            { .address = { .com = 1, .seg = 11 } }, // 2F
            { .address = { .com = 1, .seg = 10 } }, // 2G
            { .value = segment_does_not_exist },    // 2H
        },
    },
    {
        .segment = {
            { .address = { .com = 0, .seg =  9 } }, // 3A
            { .address = { .com = 0, .seg =  8 } }, // 3B
            { .address = { .com = 2, .seg =  8 } }, // 3C
            { .address = { .com = 3, .seg =  9 } }, // 3D
            { .address = { .com = 2, .seg =  9 } }, // 3E
            { .address = { .com = 1, .seg =  9 } }, // 3F
            { .address = { .com = 1, .seg =  8 } }, // 3G
            { .value = segment_does_not_exist },    // 3H
        },
    },
    {
        .segment = {
            { .address = { .com = 3, .seg = 16 } }, // 4A
            { .address = { .com = 2, .seg = 16 } }, // 4B
            { .address = { .com = 1, .seg = 16 } }, // 4C
            { .address = { .com = 0, .seg = 16 } }, // 4D
            { .address = { .com = 1, .seg = 22 } }, // 4E
            { .address = { .com = 3, .seg = 22 } }, // 4F
            { .address = { .com = 2, .seg = 22 } }, // 4G
            { .value = segment_does_not_exist },    // 4H
        },
    },
    {
        .segment = {
            { .address = { .com = 3, .seg = 14 } }, // 5A
            { .address = { .com = 2, .seg = 14 } }, // 5B
            { .address = { .com = 1, .seg = 14 } }, // 5C
            { .address = { .com = 0, .seg = 15 } }, // 5D
            { .address = { .com = 1, .seg = 15 } }, // 5E
            { .address = { .com = 3, .seg = 15 } }, // 5F
            { .address = { .com = 2, .seg = 15 } }, // 5G
            { .value = segment_does_not_exist },    // 5H
        },
    },
    {
        .segment = {
            { .address = { .com = 3, .seg =  1 } }, // 6A
            { .address = { .com = 2, .seg =  2 } }, // 6B
            { .address = { .com = 0, .seg =  2 } }, // 6C
            { .address = { .com = 0, .seg =  1 } }, // 6D
            { .address = { .com = 1, .seg =  1 } }, // 6E
            { .address = { .com = 2, .seg =  1 } }, // 6F
            { .address = { .com = 1, .seg =  2 } }, // 6G
            { .value = segment_does_not_exist },    // 6H
        },
    },
    {
        .segment = {
            { .address = { .com = 3, .seg =  3 } }, // 7A
            { .address = { .com = 2, .seg =  4 } }, // 7B
            { .address = { .com = 0, .seg =  4 } }, // 7C
            { .address = { .com = 0, .seg =  3 } }, // 7D
            { .address = { .com = 1, .seg =  3 } }, // 7E
            { .address = { .com = 2, .seg =  3 } }, // 7F
            { .address = { .com = 1, .seg =  4 } }, // 7G
            { .value = segment_does_not_exist },    // 7H
        },
    },
    {
        .segment = {
            { .address = { .com = 3, .seg = 10 } }, // 8A
            { .address = { .com = 3, .seg =  8 } }, // 8B
            { .address = { .com = 0, .seg =  5 } }, // 8C
            { .address = { .com = 1, .seg =  5 } }, // 8D
            { .address = { .com = 3, .seg =  4 } }, // 8E
            { .address = { .com = 3, .seg =  2 } }, // 8F
            { .address = { .com = 2, .seg =  5 } }, // 8G
            { .address = { .com = 3, .seg =  5 } }, // 8H
        },
    },
    {
        .segment = {
            { .address = { .com = 3, .seg = 6 } }, // 9A
            { .address = { .com = 3, .seg = 7 } }, // 9B
            { .address = { .com = 2, .seg = 7 } }, // 9C
            { .address = { .com = 0, .seg = 7 } }, // 9D
            { .address = { .com = 0, .seg = 6 } }, // 9E
            { .address = { .com = 2, .seg = 6 } }, // 9F
            { .address = { .com = 1, .seg = 6 } }, // 9G
            { .address = { .com = 1, .seg = 7 } }, // 9H
        },
    },
    // Position 10 is the third digit in the weekday, stashed at the end for backwards compatibility.
    {
        .segment = {
            { .address = { .com = 0, .seg = 12 } }, // 10A
            { .address = { .com = 2, .seg = 12 } }, // 10B
            { .address = { .com = 3, .seg = 12 } }, // 10C
            { .address = { .com = 3, .seg = 13 } }, // 10D
            { .address = { .com = 2, .seg = 13 } }, // 10E
            { .address = { .com = 0, .seg = 13 } }, // 10F
            { .address = { .com = 1, .seg = 13 } }, // 10G
            { .address = { .com = 1, .seg = 12 } }, // 10H
        },
    },
};

// Original famous Casio LCD
static const uint8_t Classic_LCD_Character_Set[] =
{
    0b00000000, // [space]
    0b01100000, // ! (L in the top half for positions 4 and 6)
    0b00100010, // "
    0b01100011, // # (degree symbol, hash mark doesn't fit)
    0b00101101, // $ (S without the center segment)
    0b00000000, // % (unused)
    0b01000100, // & ("lowercase 7" for positions 4 and 6)
    0b00100000, // '
    0b00111001, // (
    0b00001111, // )
    0b11000000, // * (The + sign for use in position 0)
    0b01110000, // + (segments E, F and G; looks like ┣╸)
    0b00000100, // ,
    0b01000000, // -
    0b01000000, // . (same as -, semantically most useful)
    0b00010010, // /
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111, // 9
    0b00000000, // : (unused)
    0b00000000, // ; (unused)
    0b01011000, // <
    0b01001000, // =
    0b01001100, // >
    0b01010011, // ?
    0b11111111, // @ (all segments on)
    0b01110111, // A
    0b01111111, // B
    0b00111001, // C
    0b00111111, // D
    0b01111001, // E
    0b01110001, // F
    0b00111101, // G
    0b01110110, // H
    0b10001001, // I (only works in position 0)
    0b00001110, // J
    0b01110101, // K
    0b00111000, // L
    0b10110111, // M (only works in position 0)
    0b00110111, // N
    0b00111111, // O
    0b01110011, // P
    0b01100111, // Q
    0b11110111, // R (only works in position 1)
    0b01101101, // S
    0b10000001, // T (only works in position 0; set (1, 12) to make it work in position 1)
    0b00111110, // U
    0b00111110, // V
    0b10111110, // W (only works in position 0)
    0b01111110, // X
    0b01101110, // Y
    0b00011011, // Z
    0b00111001, // [
    0b00100100, // backslash
    0b00001111, // ]
    0b00100011, // ^
    0b00001000, // _
    0b00000010, // `
    0b01011111, // a
    0b01111100, // b
    0b01011000, // c
    0b01011110, // d
    0b01111011, // e
    0b01110001, // f
    0b01101111, // g
    0b01110100, // h
    0b00010000, // i
    0b01000010, // j (appears as superscript to work in more positions)
    0b01110101, // k
    0b00110000, // l
    0b10110111, // m (only works in position 0)
    0b01010100, // n
    0b01011100, // o
    0b01110011, // p
    0b01100111, // q
    0b01010000, // r
    0b01101101, // s
    0b01111000, // t
    0b01100010, // u (appears in (u)pper half to work in more positions)
    0b00011100, // v (looks like u but in the lower half)
    0b10111110, // w (only works in position 0)
    0b01111110, // x
    0b01101110, // y
    0b00011011, // z
    0b00010110, // { (open brace doesn't really work; overriden to represent the two character ligature "il")
    0b00110110, // | (overriden to represent the two character ligature "ll")
    0b00110100, // } (overriden to represent the two character ligature "li")
    0b00000001, // ~
};

static const digit_mapping_t Classic_LCD_Display_Mapping[] = {
    // Positions 0 and 1 are the Weekday or Mode digits
    {
        .segment = {
            { .address = { .com = 0, .seg = 13 } }, // 0A
            { .address = { .com = 1, .seg = 13 } }, // 0B
            { .address = { .com = 2, .seg = 13 } }, // 0C
            { .address = { .com = 2, .seg = 15 } }, // 0D
            { .address = { .com = 2, .seg = 14 } }, // 0E
            { .address = { .com = 0, .seg = 14 } }, // 0F
            { .address = { .com = 1, .seg = 15 } }, // 0G
            { .address = { .com = 1, .seg = 14 } }, // 0H
        },
    },
    {
        .segment = {
            { .address = { .com = 0, .seg = 11 } }, // 1A
            { .address = { .com = 1, .seg = 11 } }, // 1B, note that 1B has the same address as 1C
            { .address = { .com = 1, .seg = 11 } }, // 1C, will override 1B when displaying a character
            { .address = { .com = 2, .seg = 11 } }, // 1D
            { .address = { .com = 1, .seg = 12 } }, // 1E, note that 1E has the same address as 1F
            { .address = { .com = 1, .seg = 12 } }, // 1F, will override 1E when displaying a character
            { .address = { .com = 2, .seg = 12 } }, // 1G
            { .address = { .com = 0, .seg = 12 } }, // 1H
        },
    },
    // Positions 2 and 3 are the Day of Month digits
    {
        .segment = {
            { .address = { .com = 1, .seg =  9 } }, // 2A, note that 2A, 2D and 2G have the same address
            { .address = { .com = 0, .seg =  9 } }, // 2B
            { .address = { .com = 2, .seg =  9 } }, // 2C
            { .address = { .com = 1, .seg =  9 } }, // 2D, same address as 2A and 2G
            { .address = { .com = 0, .seg = 10 } }, // 2E
            { .value = segment_does_not_exist },    // 2F
            { .address = { .com = 1, .seg =  9 } }, // 2G, will override 2A and 2D when displaying a character
            { .value = segment_does_not_exist },    // 2H
        },
    },
    {
        .segment = {
            { .address = { .com = 0, .seg =  7 } }, // 3A
            { .address = { .com = 1, .seg =  7 } }, // 3B
            { .address = { .com = 2, .seg =  7 } }, // 3C
            { .address = { .com = 2, .seg =  6 } }, // 3D
            { .address = { .com = 2, .seg =  8 } }, // 3E
            { .address = { .com = 0, .seg =  8 } }, // 3F
            { .address = { .com = 1, .seg =  8 } }, // 3G
            { .value = segment_does_not_exist },    // 3H
        },
    },
    // Positions 4-9 are the Clock digits
    {
        .segment = {
            { .address = { .com = 1, .seg = 18 } }, // 4A, note that 4A and 4D have the same address
            { .address = { .com = 2, .seg = 19 } }, // 4B
            { .address = { .com = 0, .seg = 19 } }, // 4C
            { .address = { .com = 1, .seg = 18 } }, // 4D, will override 4A when displaying a character
            { .address = { .com = 0, .seg = 18 } }, // 4E
            { .address = { .com = 2, .seg = 18 } }, // 4F
            { .address = { .com = 1, .seg = 19 } }, // 4G
            { .value = segment_does_not_exist },    // 4H
        },
    },
    {
        .segment = {
            { .address = { .com = 2, .seg = 20 } }, // 5A
            { .address = { .com = 2, .seg = 21 } }, // 5B
            { .address = { .com = 1, .seg = 21 } }, // 5C
            { .address = { .com = 0, .seg = 21 } }, // 5D
            { .address = { .com = 0, .seg = 20 } }, // 5E
            { .address = { .com = 1, .seg = 17 } }, // 5F
            { .address = { .com = 1, .seg = 20 } }, // 5G
            { .value = segment_does_not_exist },    // 5H
        },
    },
    {
        .segment = {
            { .address = { .com = 0, .seg = 22 } }, // 6A, note that 6A and 6D have the same address
            { .address = { .com = 2, .seg = 23 } }, // 6B
            { .address = { .com = 0, .seg = 23 } }, // 6C
            { .address = { .com = 0, .seg = 22 } }, // 6D, will override 6A when displaying a character
            { .address = { .com = 1, .seg = 22 } }, // 6E
            { .address = { .com = 2, .seg = 22 } }, // 6F
            { .address = { .com = 1, .seg = 23 } }, // 6G
            { .value = segment_does_not_exist },    // 6H
        },
    },
    {
        .segment = {
            { .address = { .com = 2, .seg =  1 } }, // 7A
            { .address = { .com = 2, .seg = 10 } }, // 7B
            { .address = { .com = 0, .seg =  1 } }, // 7C
            { .address = { .com = 0, .seg =  0 } }, // 7D
            { .address = { .com = 1, .seg =  0 } }, // 7E
            { .address = { .com = 2, .seg =  0 } }, // 7F
            { .address = { .com = 1, .seg =  1 } }, // 7G
            { .value = segment_does_not_exist },    // 7H
        },
    },
    {
        .segment = {
            { .address = { .com = 2, .seg =  2 } }, // 8A
            { .address = { .com = 2, .seg =  3 } }, // 8B
            { .address = { .com = 0, .seg =  4 } }, // 8C
            { .address = { .com = 0, .seg =  3 } }, // 8D
            { .address = { .com = 0, .seg =  2 } }, // 8E
            { .address = { .com = 1, .seg =  2 } }, // 8F
            { .address = { .com = 1, .seg =  3 } }, // 8G
            { .value = segment_does_not_exist },    // 8H
        },
    },
    {
        .segment = {
            { .address = { .com = 2, .seg =  4 } }, // 9A
            { .address = { .com = 2, .seg =  5 } }, // 9B
            { .address = { .com = 1, .seg =  6 } }, // 9C
            { .address = { .com = 0, .seg =  6 } }, // 9D
            { .address = { .com = 0, .seg =  5 } }, // 9E
            { .address = { .com = 1, .seg =  4 } }, // 9F
            { .address = { .com = 1, .seg =  5 } }, // 9G
            { .value = segment_does_not_exist },    // 9H
        },
    },
};

void watch_display_character(uint8_t character, uint8_t position);
void watch_display_character_lp_seconds(uint8_t character, uint8_t position);

void _watch_update_indicator_segments(void);
