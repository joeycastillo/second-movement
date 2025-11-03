/*
 * MIT License
 *
 * Copyright (c) 2025 Eirik S. Morland
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

/**
 * @file fesk_tx.h
 * @brief FESK Audio Data Transmission Library
 *
 * FESK (Frequency Shift Keying) encodes text messages into dual-tone audio
 * sequences for transmission via the Sensor Watch piezo buzzer.
 *
 * Protocol Format:
 *   [START(6bit)] [PAYLOAD(N×6bit)] [CRC8(8bit)] [END(6bit)]
 *
 * Character Set:
 *   - Letters: a-z A-Z (case-insensitive, codes 0-25)
 *   - Digits: 0-9 (codes 26-35)
 *   - Space: ' ' (code 36)
 *   - Punctuation: , : ' " (codes 37-40)
 *   - Newline: \n (code 41)
 *   - Total: 42 supported characters
 *
 * Tones:
 *   - Binary '0': D7# (~2489 Hz)
 *   - Binary '1': G7 (~3136 Hz)
 *   - Timing: 1 tick per bit tone, 2 ticks silence between bits
 *
 * Example Usage:
 *   int8_t *sequence = NULL;
 *   size_t entries = 0;
 *   fesk_result_t result = fesk_encode_cstr("Hello", &sequence, &entries);
 *   if (result == FESK_OK) {
 *       watch_buzzer_play_sequence(sequence, callback);
 *       fesk_free_sequence(sequence);
 *   }
 */

#ifndef FESK_TX_H
#define FESK_TX_H

#include <stddef.h>
#include <stdint.h>

#include "watch_tcc.h"

/** Result codes for FESK operations */
typedef enum {
    FESK_OK = 0,                        /**< Success */
    FESK_ERR_INVALID_ARGUMENT,          /**< NULL pointer, empty string, or length > 1024 */
    FESK_ERR_UNSUPPORTED_CHARACTER,     /**< Character not in supported set */
    FESK_ERR_ALLOCATION_FAILED,         /**< Memory allocation failed or overflow */
} fesk_result_t;

/** Timing: 1 tick tone + 2 ticks silence = 3 ticks per bit (~47ms @ 64Hz) */
#define FESK_TICKS_PER_BIT 1
#define FESK_TICKS_PER_REST 2

/** 6-bit encoding allows 64 codes (0-63) */
#define FESK_BITS_PER_CODE 6

/** Frame markers: codes 62 and 63 are reserved (not in character set) */
#define FESK_START_MARKER 62u
#define FESK_END_MARKER 63u

/** FSK tone indices */
enum {
    FESK_TONE_ZERO = 0,     /**< Index for binary '0' tone */
    FESK_TONE_ONE = 1,      /**< Index for binary '1' tone */
    FESK_TONE_COUNT = 2,
};

/** FSK tones: D7# and G7 chosen for 647Hz separation and good piezo response */
#define FESK_TONE_LOW_NOTE  BUZZER_NOTE_D7SHARP_E7FLAT  /**< ~2489 Hz for binary '0' */
#define FESK_TONE_HIGH_NOTE BUZZER_NOTE_G7               /**< ~3136 Hz for binary '1' */

/** Mapping from tone index to buzzer note */
extern const watch_buzzer_note_t fesk_tone_map[FESK_TONE_COUNT];

/**
 * @brief Encode text into FESK audio sequence
 * @param text Input text (must be valid, non-NULL)
 * @param length Length of text in bytes (max 1024)
 * @param out_sequence Pointer to receive allocated sequence (caller must free with fesk_free_sequence)
 * @param out_entries Optional pointer to receive number of sequence entries
 * @return FESK_OK on success, error code otherwise
 */
fesk_result_t fesk_encode_text(const char *text,
                               size_t length,
                               int8_t **out_sequence,
                               size_t *out_entries);

/**
 * @brief Encode null-terminated string into FESK audio sequence
 * @param text Null-terminated string (must be valid, non-NULL, non-empty)
 * @param out_sequence Pointer to receive allocated sequence (caller must free with fesk_free_sequence)
 * @param out_entries Optional pointer to receive number of sequence entries
 * @return FESK_OK on success, error code otherwise
 */
fesk_result_t fesk_encode_cstr(const char *text,
                               int8_t **out_sequence,
                               size_t *out_entries);

/**
 * @brief Free sequence allocated by fesk_encode_*
 * @param sequence Sequence to free (NULL-safe)
 */
void fesk_free_sequence(int8_t *sequence);

#endif
