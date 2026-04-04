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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uptime_face.h"
#include "watch.h"
#include "watch_utility.h"

#define U32_DEC_DIGITS 10
#define UPTIME_BUFSZ (7 + U32_DEC_DIGITS + 8 + 1) // "uptime " + digits + " seconds" + '\0'

static uint8_t long_data_str[UPTIME_BUFSZ];

static uint32_t uptime_get_seconds_since_boot(uptime_state_t *state);
static size_t build_uptime_line(uint32_t seconds_since_boot);
static void uptime_display_title(void);
static void uptime_display_elapsed(uint32_t seconds_since_boot);
static void uptime_display_countdown(uint8_t seconds_remaining);

static void uptime_on_ready(void *user_data);
static void uptime_on_countdown_begin(void *user_data);
static void uptime_on_countdown_tick(uint8_t seconds_remaining, void *user_data);
static void uptime_on_countdown_complete(void *user_data);
static void uptime_on_transmission_start(void *user_data);
static void uptime_on_transmission_end(void *user_data);
static void uptime_on_cancelled(void *user_data);
static void uptime_on_error(fesk_result_t error, void *user_data);
static fesk_result_t uptime_payload_provider(const char **out_text,
                                             size_t *out_length,
                                             void *user_data);

static uint32_t uptime_get_seconds_since_boot(uptime_state_t *state) {
    if (!state) return 0;
    uint32_t now = watch_utility_date_time_to_unix_time(movement_get_local_date_time(),
                                                        movement_get_current_timezone_offset());
    return now - state->boot_time;
}

static inline size_t build_uptime_line(uint32_t seconds_since_boot) {
    int written = snprintf((char *)long_data_str,
                           UPTIME_BUFSZ,
                           "uptime %u seconds",
                           (unsigned int)seconds_since_boot);
    if (written < 0) {
        long_data_str[0] = '\0';
        return 0;
    }
    if (written >= (int)UPTIME_BUFSZ) {
        written = UPTIME_BUFSZ - 1;
    }
    return (size_t)written;
}

static void uptime_display_title(void) {
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "UPTMe", "UP");
}

static void uptime_display_elapsed(uint32_t seconds_since_boot) {
    // Make sure we clear the display first.
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "      ", "      ");
    char buf[16];
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)seconds_since_boot);
    strncat(buf, "s", sizeof(buf) - strlen(buf) - 1);

    if (seconds_since_boot >= 60) {
        uint32_t mins = seconds_since_boot / 60;
        snprintf(buf, sizeof(buf), "%lum", (unsigned long)mins);
    }
    if (seconds_since_boot >= 3600) {
        uint32_t hours = seconds_since_boot / 3600;
        snprintf(buf, sizeof(buf), "%luh", (unsigned long)hours);
    }
    if (seconds_since_boot >= 86400) {
        uint32_t days = seconds_since_boot / 86400;
        snprintf(buf, sizeof(buf), "%lud", (unsigned long)days);
    }

    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, "0");
}

static void uptime_display_countdown(uint8_t seconds_remaining) {
    char buffer[7] = "      ";
    if (seconds_remaining > 0) {
        char temp[4];
        snprintf(temp, sizeof(temp), "%u", (unsigned int)seconds_remaining);
        size_t len = 0;
        while (len < sizeof(temp) && temp[len] != '\0') {
            len++;
        }
        if (len > sizeof(buffer) - 1) {
            len = sizeof(buffer) - 1;
        }
        memcpy(buffer + (sizeof(buffer) - 1 - len), temp, len);
    } else {
        strncpy(buffer, "    GO", sizeof(buffer));
    }
    buffer[6] = '\0';
    watch_display_text(WATCH_POSITION_BOTTOM, buffer);
}

static void uptime_on_ready(void *user_data) {
    uptime_state_t *state = (uptime_state_t *)user_data;
    if (!state) return;
    state->mode = UT_NONE;
    uptime_display_title();
    uptime_display_elapsed(uptime_get_seconds_since_boot(state));
}

static void uptime_on_countdown_begin(void *user_data) {
    uptime_state_t *state = (uptime_state_t *)user_data;
    if (!state) return;
    state->mode = UT_CHIRPING;
}

static void uptime_on_countdown_tick(uint8_t seconds_remaining, void *user_data) {
    uptime_state_t *state = (uptime_state_t *)user_data;
    if (!state) return;
    state->mode = UT_CHIRPING;
    uptime_display_countdown(seconds_remaining);
}

static void uptime_on_countdown_complete(void *user_data) {
    uptime_state_t *state = (uptime_state_t *)user_data;
    if (!state) return;
    state->mode = UT_CHIRPING;
}

static void uptime_on_transmission_start(void *user_data) {
    uptime_state_t *state = (uptime_state_t *)user_data;
    if (!state) return;
    state->mode = UT_TRANSMITTING;
    watch_display_text(WATCH_POSITION_BOTTOM, "  TX  ");
}

static void uptime_on_transmission_end(void *user_data) {
    uptime_state_t *state = (uptime_state_t *)user_data;
    if (!state) return;
    state->mode = UT_NONE;
    uptime_display_title();
    uptime_display_elapsed(uptime_get_seconds_since_boot(state));
    printf("Uptime transmission complete\n");
}

static void uptime_on_cancelled(void *user_data) {
    uptime_state_t *state = (uptime_state_t *)user_data;
    if (!state) return;
    state->mode = UT_NONE;
    uptime_display_title();
    uptime_display_elapsed(uptime_get_seconds_since_boot(state));
}

static void uptime_on_error(fesk_result_t error, void *user_data) {
    (void)user_data;
    printf("FESK uptime error: %d\n", (int)error);
}

static fesk_result_t uptime_payload_provider(const char **out_text,
                                             size_t *out_length,
                                             void *user_data) {
    uptime_state_t *state = (uptime_state_t *)user_data;
    if (!state || !out_text || !out_length) {
        return FESK_ERR_INVALID_ARGUMENT;
    }

    uint32_t seconds_since_boot = uptime_get_seconds_since_boot(state);
    size_t len = build_uptime_line(seconds_since_boot);
    if (len == 0) {
        return FESK_ERR_INVALID_ARGUMENT;
    }

    *out_text = (const char *)long_data_str;
    *out_length = len;
    return FESK_OK;
}

void uptime_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void)watch_face_index;
    if (*context_ptr != NULL) {
        return;
    }

    uptime_state_t *state = malloc(sizeof(uptime_state_t));
    if (!state) {
        return;
    }
    memset(state, 0, sizeof(*state));

    watch_date_time_t now = movement_get_local_date_time();
    state->boot_time = watch_utility_date_time_to_unix_time(
        now,
        movement_get_current_timezone_offset());

    fesk_session_config_t config = fesk_session_config_defaults();
    state->config = config;

    state->config.enable_countdown = false;
    state->config.provide_payload = uptime_payload_provider;
    state->config.on_transmission_start = uptime_on_transmission_start;
    state->config.on_transmission_end = uptime_on_transmission_end;
    state->config.on_cancelled = uptime_on_cancelled;
    state->config.on_error = uptime_on_error;
    state->config.user_data = state;

    fesk_session_init(&state->session, &state->config);

    *context_ptr = state;
}

void uptime_face_activate(void *context) {
    uptime_state_t *state = (uptime_state_t *)context;
    if (!state) return;
    state->mode = UT_NONE;
    uptime_display_title();
    uptime_display_elapsed(uptime_get_seconds_since_boot(state));
    // Session is ready, no preparation needed
}

bool uptime_face_loop(movement_event_t event, void *context) {
    uptime_state_t *state = (uptime_state_t *)context;
    if (!state) return true;

    bool handled = false;

    switch (event.event_type) {
        case EVENT_MODE_BUTTON_UP:
            if (fesk_session_is_idle(&state->session)) {
                movement_move_to_next_face();
            }
            handled = true;
            break;

        case EVENT_ALARM_BUTTON_UP:
            if (fesk_session_is_idle(&state->session)) {
                fesk_session_start(&state->session);
            } else {
                fesk_session_cancel(&state->session);
            }
            handled = true;
            break;

        case EVENT_TICK:
            if (state->mode == UT_NONE) {
                uptime_display_elapsed(uptime_get_seconds_since_boot(state));
            }
            handled = true;
            break;

        case EVENT_TIMEOUT:
            if (fesk_session_is_idle(&state->session)) {
                movement_move_to_face(0);
            }
            handled = true;
            break;

        default:
            break;
    }

    if (!handled) {
        movement_default_loop_handler(event);
    }

    return fesk_session_is_idle(&state->session);
}

void uptime_face_resign(void *context) {
    uptime_state_t *state = (uptime_state_t *)context;
    if (!state) return;
    fesk_session_cancel(&state->session);
}
