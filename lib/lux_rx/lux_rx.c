/*
 * MIT License
 *
 * Copyright (c) 2025 Claude
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

#include "lux_rx.h"
#include <string.h>
#ifdef LUX_RX_DEBUG
#include <stdio.h>
#endif

// Internal states
enum {
    ST_IDLE,
    ST_START,
    ST_DATA,
    ST_DONE,
    ST_ERROR,
};

// 6-bit symbol table: index = symbol (1-62), value = ASCII char
// 0 = END, 63 = START (not in table)
static const char sym_to_char[64] = {
    0,                                                       // 0: END
    'a','b','c','d','e','f','g','h','i','j','k','l','m',     // 1-13
    'n','o','p','q','r','s','t','u','v','w','x','y','z',     // 14-26
    ' ',                                                     // 27
    '0','1','2','3','4','5','6','7','8','9',                  // 28-37
    '.',',','!','?','\'','-',':',';','(',')','/',             // 38-48
    '@','#','&','+','=','_','~','*','"','%','\n',             // 49-59
    'A','B',                                                 // 60-61
    '\\',                                                    // 62
    0,                                                       // 63: START
};

// Reverse lookup: ASCII -> symbol (0 = unmapped)
static uint8_t char_to_sym[128];
static bool table_ready = false;

static void build_reverse_table(void) {
    if (table_ready) return;
    memset(char_to_sym, 0, sizeof(char_to_sym));
    for (uint8_t i = 1; i <= 62; i++) {
        char c = sym_to_char[i];
        if (c > 0 && c < 128) char_to_sym[(uint8_t)c] = i;
    }
    // Map uppercase to lowercase (except A, B which have their own slots)
    for (char c = 'C'; c <= 'Z'; c++) {
        char_to_sym[(uint8_t)c] = char_to_sym[(uint8_t)(c - 'A' + 'a')];
    }
    table_ready = true;
}

char lux_rx_symbol_to_char(uint8_t symbol) {
    if (symbol == 0 || symbol > 62) return 0;
    return sym_to_char[symbol];
}

uint8_t lux_rx_char_to_symbol(char c) {
    build_reverse_table();
    if (c < 0) return 0;
    return char_to_sym[(uint8_t)c];
}

// Timeout covers max 128-char frame with margin.
// At 64 Hz with 8 samples per bit and an 8 bits/s effective rate, the
// longest frame is (6 + 128*6 + 6 + 6)*8 = 6288 samples (~98s).
#define LUX_RX_TIMEOUT_TICKS 6400

// Majority threshold: bit is 1 iff at least this many of the N samples in
// the window are bright. For N=8 this is 4 (ties resolve to 1).
#define LUX_RX_BIT_MAJORITY ((LUX_RX_SAMPLES_PER_BIT + 1) / 2)

static bool is_bright(uint16_t sample) {
    return sample < LUX_RX_BRIGHT_THRESHOLD;
}

// --- Public API ---

void lux_rx_init(lux_rx_t *rx) {
    memset(rx, 0, sizeof(*rx));
    rx->state = ST_IDLE;
    build_reverse_table();
}

static void process_symbol(lux_rx_t *rx, uint8_t symbol) {
    if (symbol == LUX_RX_SYM_END) {
        if (!rx->has_prev) {
            rx->state = ST_ERROR;
            return;
        }
        // prev_symbol was the CRC; remove it from payload
        rx->payload_len--;
        uint8_t data_xor = rx->crc_accum ^ rx->prev_symbol;
        uint8_t expected = 1 + (data_xor % 62);
        if (rx->prev_symbol == expected) {
            rx->payload[rx->payload_len] = '\0';
            rx->state = ST_DONE;
        } else {
            rx->state = ST_ERROR;
        }
    } else if (symbol == LUX_RX_SYM_START) {
        rx->state = ST_ERROR;
    } else {
        if (rx->payload_len >= LUX_RX_MAX_PAYLOAD) {
            rx->state = ST_ERROR;
            return;
        }
        rx->payload[rx->payload_len++] = sym_to_char[symbol];
        rx->crc_accum ^= symbol;
        rx->prev_symbol = symbol;
        rx->has_prev = true;
    }
}

lux_rx_status_t lux_rx_feed(lux_rx_t *rx, uint16_t adc_val) {
    if (rx->state == ST_DONE) return LUX_RX_DONE;
    if (rx->state == ST_ERROR) return LUX_RX_ERROR;

    if (rx->state == ST_START || rx->state == ST_DATA) {
        if (++rx->tick_count >= LUX_RX_TIMEOUT_TICKS) {
            lux_rx_reset(rx);
            return LUX_RX_BUSY;
        }
    }

    bool bright = is_bright(adc_val);

    switch (rx->state) {
        case ST_IDLE:
            // Wait for the first bright sample. The previous sample was dark,
            // so this sample is treated as sample 0 of the first START bit
            // window. Any phase offset vs. the transmitter is < 1 sample.
            if (bright) {
                rx->state = ST_START;
                rx->start_count = 0;
                rx->sample_idx = 1;   // sample 0 has just been consumed
                rx->bright_count = 1;
                rx->tick_count = 1;
            }
            break;

        case ST_START:
            // Accumulate samples of the current START bit window.
            if (bright) rx->bright_count++;
            rx->sample_idx++;
            if (rx->sample_idx >= LUX_RX_SAMPLES_PER_BIT) {
                // All six START bits must be 1 (majority bright).
                if (rx->bright_count < LUX_RX_BIT_MAJORITY) {
                    lux_rx_reset(rx);
                    break;
                }
                rx->start_count++;
                rx->sample_idx = 0;
                rx->bright_count = 0;
                if (rx->start_count >= 6) {
                    // START confirmed. Next sample starts the first data bit.
                    rx->state = ST_DATA;
                    rx->bit_buf = 0;
                    rx->bit_count = 0;
                    rx->payload_len = 0;
                    rx->crc_accum = 0;
                    rx->prev_symbol = 0;
                    rx->has_prev = false;
                }
            }
            break;

        case ST_DATA:
            if (bright) rx->bright_count++;
            rx->sample_idx++;
            if (rx->sample_idx >= LUX_RX_SAMPLES_PER_BIT) {
                uint8_t bit = (rx->bright_count >= LUX_RX_BIT_MAJORITY) ? 1 : 0;
                rx->sample_idx = 0;
                rx->bright_count = 0;
                rx->bit_buf = (rx->bit_buf << 1) | bit;
                rx->bit_count++;
                if (rx->bit_count >= 6) {
                    uint8_t symbol = rx->bit_buf & 0x3F;
                    rx->bit_buf = 0;
                    rx->bit_count = 0;
                    process_symbol(rx, symbol);
                }
            }
            break;
    }

    if (rx->state == ST_DONE) return LUX_RX_DONE;
    if (rx->state == ST_ERROR) return LUX_RX_ERROR;
    return LUX_RX_BUSY;
}

void lux_rx_reset(lux_rx_t *rx) {
    memset(rx, 0, sizeof(*rx));
    rx->state = ST_IDLE;
}

// --- Encoder ---

// Get the logical bit (pre-oversampling) at a given index in the frame.
static uint8_t get_original_bit(lux_rx_encoder_t *enc, uint16_t idx) {
    // START symbol (0b111111)
    if (idx < 6) return 1;
    idx -= 6;

    // Data symbols
    uint16_t data_bits = (uint16_t)enc->text_len * 6;
    if (idx < data_bits) {
        uint16_t sym_index = idx / 6;
        uint8_t bit_pos = idx % 6;
        uint16_t count = 0;
        uint8_t sym = 0;
        for (const char *p = enc->text; *p; p++) {
            sym = lux_rx_char_to_symbol(*p);
            if (sym == 0) continue;
            if (count == sym_index) break;
            count++;
        }
        return (sym >> (5 - bit_pos)) & 1;
    }
    idx -= data_bits;

    // CRC symbol
    if (idx < 6) return (enc->crc >> (5 - idx)) & 1;
    idx -= 6;

    // END symbol (0b000000)
    return 0;
}

void lux_rx_encode(lux_rx_encoder_t *enc, const char *text) {
    build_reverse_table();
    enc->text = text;
    enc->text_len = 0;

    // Count encodable characters and compute CRC
    uint8_t xor = 0;
    for (const char *p = text; *p; p++) {
        uint8_t sym = lux_rx_char_to_symbol(*p);
        if (sym == 0) continue;
        xor ^= sym;
        enc->text_len++;
    }
    enc->crc = 1 + (xor % 62);

    enc->bit_index = 0;
    // Each logical bit is held for LUX_RX_SAMPLES_PER_BIT samples (NRZ).
    uint16_t orig_bits = 6 + (uint16_t)enc->text_len * 6 + 6 + 6;
    enc->total_bits = orig_bits * LUX_RX_SAMPLES_PER_BIT;
}

bool lux_rx_encode_next(lux_rx_encoder_t *enc, uint8_t *out_bit) {
    if (enc->bit_index >= enc->total_bits) return false;
    uint16_t i = enc->bit_index++;

    uint16_t orig_idx = i / LUX_RX_SAMPLES_PER_BIT;
    // NRZ: every sample within the window carries the same level as the bit.
    *out_bit = get_original_bit(enc, orig_idx);
    return true;
}
