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

// Max ticks before auto-reset (covers max 128-char frame at 8 Hz with margin)
#define LUX_RX_TIMEOUT_TICKS 800

// Bright detection margin: at least ambient/4 or 500 counts
static bool is_bright(uint16_t sample, uint16_t ambient) {
    uint16_t margin = ambient >> 2;
    if (margin < 500) margin = 500;
    return sample > ambient + margin;
}

// --- Public API ---

void lux_rx_init(lux_rx_t *rx) {
    memset(rx, 0, sizeof(*rx));
    rx->state = ST_IDLE;
    build_reverse_table();
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

    switch (rx->state) {
        case ST_IDLE:
            // Track ambient with exponential moving average
            rx->ambient = (uint16_t)(((uint32_t)rx->ambient * 7 + adc_val) >> 3);
            if (is_bright(adc_val, rx->ambient)) {
                rx->state = ST_START;
                rx->start_count = 1;
                rx->bright_accum = adc_val;
                rx->tick_count = 1;
            }
            break;

        case ST_START:
            if (is_bright(adc_val, rx->ambient)) {
                rx->start_count++;
                rx->bright_accum += adc_val;
                if (rx->start_count >= 6) {
                    // START confirmed — compute threshold
                    rx->bright = (uint16_t)(rx->bright_accum / 6);
                    rx->threshold = (rx->ambient + rx->bright) / 2;
                    rx->state = ST_DATA;
                    rx->bit_buf = 0;
                    rx->bit_count = 0;
                    rx->payload_len = 0;
                    rx->crc_accum = 0;
                    rx->prev_symbol = 0;
                    rx->has_prev = false;
                }
            } else {
                // False alarm — back to idle
                rx->state = ST_IDLE;
            }
            break;

        case ST_DATA: {
            uint8_t bit = (adc_val > rx->threshold) ? 1 : 0;
            rx->bit_buf = (rx->bit_buf << 1) | bit;
            rx->bit_count++;

            if (rx->bit_count >= 6) {
                uint8_t symbol = rx->bit_buf & 0x3F;
                rx->bit_buf = 0;
                rx->bit_count = 0;

                if (symbol == LUX_RX_SYM_END) {
                    // Frame complete — verify CRC
                    if (!rx->has_prev) {
                        #ifdef LUX_RX_DEBUG
                        printf("ERROR: empty frame\n");
                        #endif
                        rx->state = ST_ERROR; // empty frame
                        break;
                    }
                    // prev_symbol was the CRC; remove it from payload
                    rx->payload_len--;
                    uint8_t data_xor = rx->crc_accum ^ rx->prev_symbol;
                    uint8_t expected = 1 + (data_xor % 62);
                    if (rx->prev_symbol == expected) {
                        rx->payload[rx->payload_len] = '\0';
                        rx->state = ST_DONE;
                    } else {
                        #ifdef LUX_RX_DEBUG
                        printf("ERROR: CRC mismatch (got %u, expected %u)\n", rx->prev_symbol, expected);
                        #endif
                        rx->state = ST_ERROR;
                    }
                } else if (symbol == LUX_RX_SYM_START) {
                    #ifdef LUX_RX_DEBUG
                    printf("ERROR: unexpected START symbol\n");
                    #endif
                    rx->state = ST_ERROR; // unexpected START in data
                } else {
                    // Data symbol
                    if (rx->payload_len >= LUX_RX_MAX_PAYLOAD) {
                        #ifdef LUX_RX_DEBUG
                        printf("ERROR: payload overflow\n");
                        #endif
                        rx->state = ST_ERROR; // overflow
                        break;
                    }
                    char c = sym_to_char[symbol];
                    #ifdef LUX_RX_DEBUG
                    printf("DATA: symbol=%u char='%c' crc_accum=%u\n", symbol, c, rx->crc_accum);
                    #endif
                    rx->payload[rx->payload_len++] = c;
                    rx->crc_accum ^= symbol;
                    rx->prev_symbol = symbol;
                    rx->has_prev = true;
                }
            }
            break;
        }
    }

    if (rx->state == ST_DONE) return LUX_RX_DONE;
    if (rx->state == ST_ERROR) return LUX_RX_ERROR;
    return LUX_RX_BUSY;
}

void lux_rx_reset(lux_rx_t *rx) {
    uint16_t amb = rx->ambient;
    memset(rx, 0, sizeof(*rx));
    rx->ambient = amb;
    rx->state = ST_IDLE;
}

// --- Encoder ---

void lux_rx_encode(lux_rx_encoder_t *enc, const char *text) {
    build_reverse_table();
    enc->text = text;
    enc->text_len = 0;

    // Count encodable characters and compute CRC
    uint8_t xor = 0;
    for (const char *p = text; *p; p++) {
        uint8_t sym = lux_rx_char_to_symbol(*p);
        if (sym == 0) continue; // skip unmappable chars
        xor ^= sym;
        enc->text_len++;
    }
    enc->crc = 1 + (xor % 62);

    enc->bit_index = 0;
    // START(6) + data(text_len*6) + CRC(6) + END(6)
    enc->total_bits = 6 + (uint16_t)enc->text_len * 6 + 6 + 6;
}

bool lux_rx_encode_next(lux_rx_encoder_t *enc, uint8_t *out_bit) {
    if (enc->bit_index >= enc->total_bits) return false;
    uint16_t i = enc->bit_index++;

    // START symbol (0b111111)
    if (i < 6) {
        *out_bit = 1;
        return true;
    }
    i -= 6;

    // Data symbols
    uint16_t data_bits = (uint16_t)enc->text_len * 6;
    if (i < data_bits) {
        uint16_t sym_index = i / 6;
        uint8_t bit_pos = i % 6;

        // Walk the text to find the sym_index'th encodable char
        uint16_t count = 0;
        uint8_t sym = 0;
        for (const char *p = enc->text; *p; p++) {
            sym = lux_rx_char_to_symbol(*p);
            if (sym == 0) continue;
            if (count == sym_index) break;
            count++;
        }

        *out_bit = (sym >> (5 - bit_pos)) & 1;
        return true;
    }
    i -= data_bits;

    // CRC symbol
    if (i < 6) {
        *out_bit = (enc->crc >> (5 - i)) & 1;
        return true;
    }
    i -= 6;

    // END symbol (0b000000)
    if (i < 6) {
        *out_bit = 0;
        return true;
    }

    return false;
}
