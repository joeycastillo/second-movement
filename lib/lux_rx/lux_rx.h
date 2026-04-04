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

#ifndef LUX_RX_H_
#define LUX_RX_H_

#include <stdint.h>
#include <stdbool.h>

/*
 * lux_rx — 6-bit optical text receiver.
 *
 * Frame: [START] [data symbols...] [CRC6] [END]
 *   START = 0b111111 (6 bright bits, also calibrates bright level)
 *   END   = 0b000000 (6 dark bits)
 *   Data  = 6-bit symbols (values 1-62), mapped to ASCII
 *   CRC   = 1 + (XOR of all data symbols) % 62
 *
 * No sync preamble — the dark-to-bright edge at START aligns bit timing.
 * 62-char alphabet: a-z, space, 0-9, common punctuation.
 *
 * Usage (receiver):
 *   lux_rx_t rx;
 *   lux_rx_init(&rx);
 *   // each tick:
 *   if (lux_rx_feed(&rx, adc_val) == LUX_RX_DONE) {
 *       use(rx.payload);  // null-terminated string
 *       lux_rx_reset(&rx);
 *   }
 *
 * Usage (encoder):
 *   lux_rx_encoder_t enc;
 *   lux_rx_encode(&enc, "hello world");
 *   uint8_t bit;
 *   while (lux_rx_encode_next(&enc, &bit)) { transmit(bit); }
 */

#define LUX_RX_MAX_PAYLOAD 128

#define LUX_RX_SYM_END    0   // 0b000000
#define LUX_RX_SYM_START  63  // 0b111111

typedef enum {
    LUX_RX_BUSY,
    LUX_RX_DONE,
    LUX_RX_ERROR,
} lux_rx_status_t;

typedef struct {
    // --- public (read after LUX_RX_DONE) ---
    char payload[LUX_RX_MAX_PAYLOAD + 1]; // null-terminated text
    uint8_t payload_len;

    // --- private ---
    uint16_t ambient;
    uint16_t bright;
    uint16_t threshold;
    uint32_t bright_accum;
    uint8_t state;
    uint8_t start_count;
    uint8_t bit_buf;
    uint8_t bit_count;
    uint8_t prev_symbol;
    uint8_t crc_accum;
    bool has_prev;
    uint16_t tick_count;
} lux_rx_t;

void lux_rx_init(lux_rx_t *rx);
lux_rx_status_t lux_rx_feed(lux_rx_t *rx, uint16_t adc_val);
void lux_rx_reset(lux_rx_t *rx);

// --- Encoder ---

typedef struct {
    const char *text;
    uint8_t text_len;
    uint8_t crc;
    uint16_t bit_index, total_bits;
} lux_rx_encoder_t;

void lux_rx_encode(lux_rx_encoder_t *enc, const char *text);
bool lux_rx_encode_next(lux_rx_encoder_t *enc, uint8_t *out_bit);

// --- Character table ---

char lux_rx_symbol_to_char(uint8_t symbol);
uint8_t lux_rx_char_to_symbol(char c);

#endif // LUX_RX_H_
