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

#include "fesk_tx.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "watch_tcc.h"

#define FESK_BIT_LOG_CAP 2048
#define FESK_CODE_LOG_CAP 2048
#define FESK_MAX_MESSAGE_LENGTH 1024

#ifndef FESK_USE_LOG
// Flip to 1 at build time (e.g. -DFESK_USE_LOG=1) to display debug logs.
#define FESK_USE_LOG 0
#endif

typedef struct {
    unsigned char character;
    uint8_t code;
} fesk_code_entry_t;

static const fesk_code_entry_t _code_table[] = {
    {'a', 0},  {'b', 1},  {'c', 2},  {'d', 3},  {'e', 4},  {'f', 5},  {'g', 6},  {'h', 7},
    {'i', 8},  {'j', 9},  {'k', 10}, {'l', 11}, {'m', 12}, {'n', 13}, {'o', 14}, {'p', 15},
    {'q', 16}, {'r', 17}, {'s', 18}, {'t', 19}, {'u', 20}, {'v', 21}, {'w', 22}, {'x', 23},
    {'y', 24}, {'z', 25}, {'0', 26}, {'1', 27}, {'2', 28}, {'3', 29}, {'4', 30}, {'5', 31},
    {'6', 32}, {'7', 33}, {'8', 34}, {'9', 35}, {' ', 36}, {',', 37}, {':', 38}, {'\'', 39},
    {'"', 40}, {'\n', 41},
};

const watch_buzzer_note_t fesk_tone_map[FESK_TONE_COUNT] = {
    [FESK_TONE_ZERO] = FESK_TONE_LOW_NOTE,
    [FESK_TONE_ONE] = FESK_TONE_HIGH_NOTE,
};

#if FESK_USE_LOG
static void _append_to_log(char *buffer,
                           size_t *offset,
                           size_t capacity,
                           const char *fmt, ...) {
    if (!buffer || !offset || *offset >= capacity) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    int written = vsnprintf(buffer + *offset, capacity - *offset, fmt, args);
    va_end(args);

    if (written < 0) {
        buffer[capacity - 1] = '\0';
        *offset = capacity;
        return;
    }

    size_t added = (size_t)written;
    if (added >= capacity - *offset) {
        *offset = capacity - 1;
        buffer[capacity - 1] = '\0';
    } else {
        *offset += added;
    }
}
#else
static inline void _append_to_log(char *buffer,
                                  size_t *offset,
                                  size_t capacity,
                                  const char *fmt, ...) {
    (void)buffer;
    (void)offset;
    (void)capacity;
    (void)fmt;
}
#endif

static bool _lookup_code(unsigned char raw, uint8_t *out_code) {
    if (!out_code) {
        return false;
    }

    unsigned char normalized = raw;
    if (isalpha(raw)) {
        normalized = (unsigned char)tolower(raw);
    }

    for (size_t i = 0; i < sizeof(_code_table) / sizeof(_code_table[0]); i++) {
        if (_code_table[i].character == normalized) {
            *out_code = _code_table[i].code;
            return true;
        }
    }

    return false;
}

/**
 * CRC-8 with polynomial x^3 + x^2 + x + 1 (0x07)
 * Common in embedded systems, provides good error detection for short messages.
 */
static inline uint8_t _crc8_update_bit(uint8_t crc, uint8_t bit) {
    uint8_t mix = ((crc >> 7) & 0x01u) ^ (bit & 0x01u);
    crc <<= 1;
    if (mix) {
        crc ^= 0x07u;  /* Polynomial: x^3 + x^2 + x + 1 */
    }
    return crc;
}

static uint8_t _crc8_update_code(uint8_t crc, uint8_t code) {
    for (int shift = FESK_BITS_PER_CODE - 1; shift >= 0; --shift) {
        uint8_t bit = (uint8_t)((code >> shift) & 0x01u);
        crc = _crc8_update_bit(crc, bit);
    }
    return crc;
}

static inline void _append_bit(uint8_t bit,
                               int8_t *sequence,
                               size_t *pos,
                               char *bit_log,
                               size_t *bit_offset,
                               size_t bit_capacity) {
    watch_buzzer_note_t tone = fesk_tone_map[bit];
    sequence[(*pos)++] = (int8_t)tone;
    sequence[(*pos)++] = FESK_TICKS_PER_BIT;
    sequence[(*pos)++] = (int8_t)BUZZER_NOTE_REST;
    sequence[(*pos)++] = FESK_TICKS_PER_REST;

    if (bit_log) {
        _append_to_log(bit_log,
                       bit_offset,
                       bit_capacity,
                       *bit_offset == 0 ? "%u" : " %u",
                       bit);
    }
}

static void _append_code_to_log(char *buffer,
                                size_t *offset,
                                size_t capacity,
                                const char *label,
                                uint32_t value) {
    if (!buffer || !offset) {
        return;
    }

    _append_to_log(buffer,
                   offset,
                   capacity,
                   *offset == 0 ? "%s(%u)" : " %s(%u)",
                   label,
                   value);
}

static void _append_bits_from_code(uint8_t code,
                                   int8_t *sequence,
                                   size_t *pos,
                                   char *bit_log,
                                   size_t *bit_offset,
                                   size_t bit_capacity) {
    for (int shift = FESK_BITS_PER_CODE - 1; shift >= 0; --shift) {
        uint8_t bit = (uint8_t)((code >> shift) & 0x01u);
        _append_bit(bit, sequence, pos, bit_log, bit_offset, bit_capacity);
    }
}

static void _append_crc_bits(uint8_t crc,
                             int8_t *sequence,
                             size_t *pos,
                             char *bit_log,
                             size_t *bit_offset,
                             size_t bit_capacity) {
    for (int shift = 7; shift >= 0; --shift) {
        uint8_t bit = (uint8_t)((crc >> shift) & 0x01u);
        _append_bit(bit, sequence, pos, bit_log, bit_offset, bit_capacity);
    }
}

static fesk_result_t _encode_internal(const char *text,
                                      size_t length,
                                      int8_t **out_sequence,
                                      size_t *out_entries) {
    if (!text || !out_sequence || length == 0) {
        return FESK_ERR_INVALID_ARGUMENT;
    }

    // Check for maximum message length to prevent excessive allocations
    if (length > FESK_MAX_MESSAGE_LENGTH) {
        return FESK_ERR_INVALID_ARGUMENT;
    }

    uint8_t *payload_codes = malloc(length);
    if (!payload_codes) {
        return FESK_ERR_ALLOCATION_FAILED;
    }

    uint8_t crc = 0;
    size_t payload_count = 0;
    for (size_t i = 0; i < length; i++) {
        unsigned char raw = (unsigned char)text[i];
        uint8_t code = 0;
        if (!_lookup_code(raw, &code)) {
            free(payload_codes);
            return FESK_ERR_UNSUPPORTED_CHARACTER;
        }

        payload_codes[payload_count++] = code;
        crc = _crc8_update_code(crc, code);
    }

    size_t total_bits = FESK_BITS_PER_CODE                                 // start marker
                      + (payload_count * FESK_BITS_PER_CODE)               // payload
                      + 8                                                  // CRC
                      + FESK_BITS_PER_CODE;                                // end marker

    // Check for overflow before multiplication
    if (total_bits > SIZE_MAX / 4) {
        free(payload_codes);
        return FESK_ERR_ALLOCATION_FAILED;
    }
    size_t total_entries = total_bits * 4;

    // Check for overflow before final allocation
    if (total_entries > SIZE_MAX - 1 || (total_entries + 1) > SIZE_MAX / sizeof(int8_t)) {
        free(payload_codes);
        return FESK_ERR_ALLOCATION_FAILED;
    }

    int8_t *sequence = malloc((total_entries + 1) * sizeof(int8_t));
    if (!sequence) {
        free(payload_codes);
        return FESK_ERR_ALLOCATION_FAILED;
    }

    size_t bit_log_offset = 0;
    size_t code_log_offset = 0;
#if FESK_USE_LOG
    char bit_log_storage[FESK_BIT_LOG_CAP];
    char code_log_storage[FESK_CODE_LOG_CAP];
    char *bit_log = bit_log_storage;
    char *code_log = code_log_storage;
    bit_log[0] = '\0';
    code_log[0] = '\0';
#else
    char *bit_log = NULL;
    char *code_log = NULL;
#endif
    size_t *bit_log_offset_ptr = bit_log ? &bit_log_offset : NULL;
    size_t *code_log_offset_ptr = code_log ? &code_log_offset : NULL;
    size_t bit_log_capacity = bit_log ? FESK_BIT_LOG_CAP : 0;
    size_t code_log_capacity = code_log ? FESK_CODE_LOG_CAP : 0;

    size_t pos = 0;

    if (code_log) {
        _append_code_to_log(code_log, code_log_offset_ptr, code_log_capacity, "START", FESK_START_MARKER);
    }
    _append_bits_from_code(FESK_START_MARKER,
                           sequence,
                           &pos,
                           bit_log,
                           bit_log_offset_ptr,
                           bit_log_capacity);

    for (size_t i = 0; i < payload_count; ++i) {
        unsigned char display_char = (unsigned char)text[i];
        if (isalpha(display_char)) {
            display_char = (unsigned char)tolower(display_char);
        }

        if (code_log) {
            char label[8];
            if (display_char >= 32 && display_char <= 126) {
                snprintf(label, sizeof(label), "%c", display_char);
            } else {
                snprintf(label, sizeof(label), "0x%02X", display_char);
            }

            _append_code_to_log(code_log,
                                code_log_offset_ptr,
                                code_log_capacity,
                                label,
                                payload_codes[i]);
        }

        _append_bits_from_code(payload_codes[i],
                               sequence,
                               &pos,
                               bit_log,
                               bit_log_offset_ptr,
                               bit_log_capacity);
    }

    if (code_log) {
        _append_code_to_log(code_log, code_log_offset_ptr, code_log_capacity, "CRC", crc);
    }
    _append_crc_bits(crc, sequence, &pos, bit_log, bit_log_offset_ptr, bit_log_capacity);

    if (code_log) {
        _append_code_to_log(code_log, code_log_offset_ptr, code_log_capacity, "END", FESK_END_MARKER);
    }
    _append_bits_from_code(FESK_END_MARKER,
                           sequence,
                           &pos,
                           bit_log,
                           bit_log_offset_ptr,
                           bit_log_capacity);

    sequence[pos] = 0;

    if (bit_log && bit_log[0] != '\0') {
        printf("FESK bits: %s\n", bit_log);
    }
    if (code_log && code_log[0] != '\0') {
        printf("FESK codes: %s\n", code_log);
    }

    if (out_entries) {
        *out_entries = pos;
    }
    *out_sequence = sequence;

    free(payload_codes);
    return FESK_OK;
}

fesk_result_t fesk_encode(const char *text,
                          int8_t **out_sequence,
                          size_t *out_entries) {
    if (!text) {
        return FESK_ERR_INVALID_ARGUMENT;
    }

    size_t length = strlen(text);
    if (length == 0) {
        return FESK_ERR_INVALID_ARGUMENT;
    }

    return _encode_internal(text, length, out_sequence, out_entries);
}

void fesk_free_sequence(int8_t *sequence) {
    if (sequence) {
        free(sequence);
    }
}
