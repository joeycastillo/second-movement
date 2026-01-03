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
 * @brief FESK Audio Data Transmission Library (2-FSK / 4-FSK)
 *
 * FESK supports both Binary FSK (2 tones) and Quad FSK (4 tones) modes:
 *   - 2-FSK: 1 bit per symbol (slower, more robust)
 *   - 4-FSK: 2 bits per symbol (faster, default mode)
 *
 * Protocol Format:
 *   [START(6bit)] [PAYLOAD(N×6bit)] [CRC8(8bit)] [END(6bit)]
 *   Transmitted as bits (2-FSK) or dibits (4-FSK)
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
 *   2-FSK (bits):
 *     - Bit 0: D7  (~2349 Hz)
 *     - Bit 1: F7# (~2960 Hz)
 *   4-FSK (dibits, default):
 *     - Dibit 00: D7  (~2349 Hz)
 *     - Dibit 01: E7  (~2637 Hz)
 *     - Dibit 10: F7# (~2960 Hz)
 *     - Dibit 11: G7# (~3322 Hz)
 *
 * Example Usage:
 *   int8_t *sequence = NULL;
 *   size_t entries = 0;
 *   fesk_result_t result = fesk_encode("Hello", FESK_MODE_4FSK, &sequence, &entries);
 *   if (result == FESK_OK) {
 *       watch_buzzer_play_sequence(sequence, callback);
 *       fesk_free_sequence(sequence);
 *   }
 */

#ifndef FESK_TX_H
#define FESK_TX_H

#include <stdbool.h>
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

/** FESK modulation modes */
typedef enum {
    FESK_MODE_2FSK = 0,  /**< Binary FSK (2 tones, 1 bit per symbol) */
    FESK_MODE_4FSK = 1,  /**< Quad FSK (4 tones, 2 bits per symbol, default) */
} fesk_mode_t;


#define FESK_TICKS_PER_SYMBOL 2
#define FESK_TICKS_PER_REST 3

#define FESK_BITS_PER_CODE 6
#define FESK_BITS_PER_SYMBOL 2
#define FESK_DIBITS_PER_CODE 3   /**< 6 bits = 3 dibits */
#define FESK_DIBITS_PER_CRC 4    /**< 8 bits = 4 dibits */

/** Frame markers: codes 62 and 63 are reserved (not in character set) */
#define FESK_START_MARKER 62u
#define FESK_END_MARKER 63u

/** 2-FSK tone indices (bits) */
#define FESK_2FSK_TONE_COUNT 2
enum {
    FESK_2FSK_TONE_0 = 0,  /**< Bit 0 */
    FESK_2FSK_TONE_1 = 1,  /**< Bit 1 */
};

/** 2-FSK tones: D7 and F7# - well-separated for robust discrimination */
#define FESK_2FSK_TONE_0_NOTE  BUZZER_NOTE_D7               /**< ~2349 Hz for bit 0 */
#define FESK_2FSK_TONE_1_NOTE  BUZZER_NOTE_F7SHARP_G7FLAT   /**< ~2960 Hz for bit 1 */

/** 4-FSK tone indices (dibits) */
#define FESK_4FSK_TONE_COUNT 4
enum {
    FESK_4FSK_TONE_00 = 0,  /**< Dibit 00 */
    FESK_4FSK_TONE_01 = 1,  /**< Dibit 01 */
    FESK_4FSK_TONE_10 = 2,  /**< Dibit 10 */
    FESK_4FSK_TONE_11 = 3,  /**< Dibit 11 */
};

/** 4-FSK tones: D7, E7, F7#, G7# - evenly spaced for good discrimination */
#define FESK_4FSK_TONE_00_NOTE  BUZZER_NOTE_D7               /**< ~2349 Hz for dibit 00 */
#define FESK_4FSK_TONE_01_NOTE  BUZZER_NOTE_E7               /**< ~2637 Hz for dibit 01 */
#define FESK_4FSK_TONE_10_NOTE  BUZZER_NOTE_F7SHARP_G7FLAT   /**< ~2960 Hz for dibit 10 */
#define FESK_4FSK_TONE_11_NOTE  BUZZER_NOTE_G7SHARP_A7FLAT   /**< ~3322 Hz for dibit 11 */

/** Legacy definitions for backward compatibility */
#define FESK_TONE_COUNT FESK_4FSK_TONE_COUNT
#define FESK_TONE_00 FESK_4FSK_TONE_00
#define FESK_TONE_01 FESK_4FSK_TONE_01
#define FESK_TONE_10 FESK_4FSK_TONE_10
#define FESK_TONE_11 FESK_4FSK_TONE_11
#define FESK_TONE_00_NOTE FESK_4FSK_TONE_00_NOTE
#define FESK_TONE_01_NOTE FESK_4FSK_TONE_01_NOTE
#define FESK_TONE_10_NOTE FESK_4FSK_TONE_10_NOTE
#define FESK_TONE_11_NOTE FESK_4FSK_TONE_11_NOTE

/** Tone map arrays for both modes */
extern const watch_buzzer_note_t fesk_tone_map_2fsk[FESK_2FSK_TONE_COUNT];
extern const watch_buzzer_note_t fesk_tone_map_4fsk[FESK_4FSK_TONE_COUNT];

/** Legacy tone map (points to 4FSK for backward compatibility) */
extern const watch_buzzer_note_t fesk_tone_map[FESK_4FSK_TONE_COUNT];

/**
 * @brief Encode null-terminated string into FESK audio sequence
 * @param text Null-terminated string (must be valid, non-NULL, non-empty)
 * @param mode Modulation mode (FESK_MODE_2FSK or FESK_MODE_4FSK)
 * @param out_sequence Pointer to receive allocated sequence (caller must free with fesk_free_sequence)
 * @param out_entries Optional pointer to receive number of sequence entries
 * @return FESK_OK on success, error code otherwise
 */
fesk_result_t fesk_encode(const char *text,
                          fesk_mode_t mode,
                          int8_t **out_sequence,
                          size_t *out_entries);

/**
 * @brief Free sequence allocated by fesk_encode
 * @param sequence Sequence to free (NULL-safe)
 */
void fesk_free_sequence(int8_t *sequence);

// Helper functions for raw source generation
bool fesk_lookup_char_code(unsigned char ch, uint8_t *out_code);
uint8_t fesk_crc8_update_code(uint8_t crc, uint8_t code);

#endif  // FESK_TX_H
