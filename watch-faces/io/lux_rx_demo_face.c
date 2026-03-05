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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "lux_rx_demo_face.h"
#include "opt3001.h"
#include "watch_i2c.h"

#define OPT3001_ADDR 0x44

static const opt3001_Config_t lux_rx_continuous = {
    .RangeNumber = 0b1100,
    .ConversionTime = 0,
    .ModeOfConversionOperation = 0b11,
};

static const opt3001_Config_t lux_rx_shutdown = {
    .ModeOfConversionOperation = 0b00,
};

static uint16_t read_light(void) {
    opt3001_t result = opt3001_readResult(OPT3001_ADDR);
    float scaled = result.lux * 100.0f;
    if (scaled > 65535.0f) return 65535;
    return (uint16_t)scaled;
}

void lux_rx_demo_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(lux_rx_demo_context_t));
        memset(*context_ptr, 0, sizeof(lux_rx_demo_context_t));
    }
}

void lux_rx_demo_face_activate(void *context) {
    lux_rx_demo_context_t *ctx = (lux_rx_demo_context_t *)context;

    watch_enable_i2c();
    opt3001_writeConfig(OPT3001_ADDR, lux_rx_continuous);

    lux_rx_init(&ctx->rx);

    movement_request_tick_frequency(8);
}

bool lux_rx_demo_face_loop(movement_event_t event, void *context) {
    lux_rx_demo_context_t *ctx = (lux_rx_demo_context_t *)context;
    char buf[7];

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "LUX r", "Lr");
            watch_display_text(WATCH_POSITION_BOTTOM, "WAIT  ");
            break;

        case EVENT_TICK:
        {
            lux_rx_status_t status = lux_rx_feed(&ctx->rx, read_light());
            ctx->last_status = status;

            switch (status) {
                case LUX_RX_DONE:
                    watch_display_text_with_fallback(WATCH_POSITION_TOP, "RECV ", "RC");
                    snprintf(buf, 7, " %s    ", ctx->rx.payload);
                    watch_display_text(WATCH_POSITION_BOTTOM, buf);
                    movement_force_led_on(0, 48, 0);
                    break;
                case LUX_RX_ERROR:
                    watch_display_text_with_fallback(WATCH_POSITION_TOP, "ERR  ", "ER");
                    watch_display_text(WATCH_POSITION_BOTTOM, "FAIL  ");
                    movement_force_led_on(48, 0, 0);
                    break;
                case LUX_RX_BUSY:
                    break;
            }
            break;
        }

        case EVENT_ALARM_BUTTON_UP:
            if (ctx->last_status == LUX_RX_DONE || ctx->last_status == LUX_RX_ERROR) {
                movement_force_led_off();
                lux_rx_reset(&ctx->rx);
                watch_display_text_with_fallback(WATCH_POSITION_TOP, "LUX r", "Lr");
                watch_display_text(WATCH_POSITION_BOTTOM, "WAIT  ");
            } else {
                lux_rx_init(&ctx->rx);
                watch_display_text_with_fallback(WATCH_POSITION_TOP, "LUX r", "Lr");
                watch_display_text(WATCH_POSITION_BOTTOM, "WAIT  ");
            }
            break;

        case EVENT_LIGHT_BUTTON_UP:
            break;

        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;

        default:
            return movement_default_loop_handler(event);
    }

    return false;
}

void lux_rx_demo_face_resign(void *context) {
    (void) context;
    movement_force_led_off();
    opt3001_writeConfig(OPT3001_ADDR, lux_rx_shutdown);
    watch_disable_i2c();
}
