/*
 * MIT License
 *
 * Copyright (c) 2022 Wesley Ellis (https://github.com/tahnok)
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
#include <math.h>

#include "TOTP.h"
#include "base32.h"

#include "watch.h"
#include "filesystem.h"

#include "totp_lfs_face.h"

#define MAX_TOTP_RECORDS 30
#define MAX_TOTP_SECRET_SIZE 128
#define TOTP_FILE "totp_uris.txt"

const char* TOTP_URI_START = "otpauth://totp/";

struct totp_record {
    char label[4];
    hmac_alg algorithm;
    uint8_t period;
    uint8_t secret_size;

    union {
        uint8_t *secret;
        struct {
            uint16_t file_secret_offset;
            uint16_t file_secret_length;
        };
    };
};

/* This is used if we're not storing all the secrets but instead
 * calculating them on demand. Avoids malloc in normal operation.
 */
static uint8_t current_secret[MAX_TOTP_SECRET_SIZE];

static struct totp_record totp_records[MAX_TOTP_RECORDS];
static uint8_t num_totp_records = 0;

static void init_totp_record(struct totp_record *totp_record) {
    totp_record->label[0] = 'A';
    totp_record->label[1] = 'A';
    totp_record->label[2] = 'A';
    totp_record->label[3] = 0;
    totp_record->algorithm = SHA1;
    totp_record->period = 30;
    totp_record->secret_size = 0;
}

static bool totp_lfs_face_read_param(struct totp_record *totp_record, char *param, char *value) {
    if (!strcmp(param, "issuer")) {
        if (value[0] == '\0') {
            printf("TOTP issuer must be a non-empty string\n");
            return false;
        }
        snprintf(totp_record->label, sizeof(totp_record->label), "%-3s", value);
    } else if (!strcmp(param, "secret")) {
        totp_record->file_secret_length = strlen(value);
        if (UNBASE32_LEN(totp_record->file_secret_length) > MAX_TOTP_SECRET_SIZE) {
            printf("TOTP secret too long: %s\n", value);
            return false;
        }
        totp_record->secret_size = base32_decode((unsigned char *)value, current_secret);
        if (totp_record->secret_size == 0) {
            printf("TOTP can't decode secret: %s\n", value);
            return false;
        }
    } else if (!strcmp(param, "digits")) {
        if (!strcmp(param, "6")) {
            printf("TOTP got %s, not 6 digits\n", value);
            return false;
        }
    } else if (!strcmp(param, "period")) {
        totp_record->period = atoi(value);
        if (totp_record->period == 0) {
            printf("TOTP invalid period %s\n", value);
            return false;
        }
    } else if (!strcmp(param, "algorithm")) {
        if (!strcmp(value, "SHA1")) {
            totp_record->algorithm = SHA1;
        }
        else if (!strcmp(value, "SHA224")) {
            totp_record->algorithm = SHA224;
        }
        else if (!strcmp(value, "SHA256")) {
            totp_record->algorithm = SHA256;
        }
        else if (!strcmp(value, "SHA384")) {
            totp_record->algorithm = SHA384;
        }
        else if (!strcmp(value, "SHA512")) {
            totp_record->algorithm = SHA512;
        }
        else {
            printf("TOTP ignored due to algorithm %s\n", value);
            return false;
        }
    }

    return true;
}

static void totp_lfs_face_read_file(char *filename) {
    // For 'format' of file, see comment at top.
    const size_t uri_start_len = strlen(TOTP_URI_START);

    if (!filesystem_file_exists(filename)) {
        printf("TOTP file error: %s\n", filename);
        return;
    }

    char line[256];
    int32_t offset = 0, old_offset = 0;
    while (old_offset = offset, filesystem_read_line(filename, line, &offset, 255) && strlen(line)) {
        if (num_totp_records == MAX_TOTP_RECORDS) {
            printf("TOTP max records: %d\n", MAX_TOTP_RECORDS);
            break;
        }

        // Check that it looks like a URI
        if (strncmp(TOTP_URI_START, line, uri_start_len)) {
            printf("TOTP invalid uri start: %s\n", line);
            continue;
        }

        // Check that we can find a '?' (to start our parameters)
        char *param;
        char *param_saveptr = NULL;
        char *params = strchr(line + uri_start_len, '?');
        if (params == NULL) {
            printf("TOTP no params: %s\n", line);
            continue;
        }

        // Process the parameters and put them in the record
        init_totp_record(&totp_records[num_totp_records]);
        bool error = false;
        param = strtok_r(params + 1, "&", &param_saveptr);
        do {
            char *param_middle = strchr(param, '=');
            *param_middle = '\0';
            if (totp_lfs_face_read_param(&totp_records[num_totp_records], param, param_middle + 1)) {
                if (!strcmp(param, "secret")) {
                    totp_records[num_totp_records].file_secret_offset = old_offset + (param_middle + 1 - line);
                }
            } else {
                error = true;
            }
        } while ((param = strtok_r(NULL, "&", &param_saveptr)));

        if (error) {
            totp_records[num_totp_records].secret_size = 0;
            continue;
        }

        // If we found a probably valid TOTP record, keep it.
        if (totp_records[num_totp_records].secret_size) {
            num_totp_records += 1;
        } else {
            printf("TOTP missing secret: %s\n", line);
        }
    }
}

void totp_lfs_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(totp_lfs_state_t));
    }

#if !(__EMSCRIPTEN__)
    if (num_totp_records == 0) {
        totp_lfs_face_read_file(TOTP_FILE);
    }
#endif
}

static uint8_t *totp_lfs_face_get_file_secret(struct totp_record *record) {
    char buffer[BASE32_LEN(MAX_TOTP_SECRET_SIZE) + 1];
    int32_t file_secret_offset = record->file_secret_offset;

    /* TODO filesystem_read_line is quite inefficient. Consider writing a new function,
     * and keeping the file open?
     */
    if (!filesystem_read_line(TOTP_FILE, buffer, &file_secret_offset, record->file_secret_length + 1)) {
        /* Shouldn't happen at this point. Return current_secret, which is misleading but will not cause a crash. */
        printf("TOTP can't read expected secret from totp_uris.txt (failed readline)\n");
        return current_secret;
    }
    if (base32_decode((unsigned char *)buffer, current_secret) != record->secret_size) {
        printf("TOTP can't properly decode secret '%s' from totp_uris.txt; failed at offset %d; read to %ld\n", buffer, record->file_secret_offset, file_secret_offset);
    }
    return current_secret;
}

static void totp_face_set_record(totp_lfs_state_t *totp_state, int i) {
    struct totp_record *record;

    if (num_totp_records == 0 && i >= num_totp_records) {
        return;
    }

    totp_state->current_index = i;
    record = &totp_records[i];

    TOTP(
        totp_lfs_face_get_file_secret(record),
        record->secret_size,
        record->period,
        record->algorithm
    );
    totp_state->current_code = getCodeFromTimestamp(totp_state->timestamp);
    totp_state->steps = totp_state->timestamp / record->period;
}

void totp_lfs_face_activate(void *context) {
    memset(context, 0, sizeof(totp_lfs_state_t));
    totp_lfs_state_t *totp_state = (totp_lfs_state_t *)context;

#if __EMSCRIPTEN__
    if (num_totp_records == 0) {
        // Doing this here rather than in setup makes things a bit more pleasant in the simulator, since there's no easy way to trigger
        // setup again after uploading the data.
        totp_lfs_face_read_file(TOTP_FILE);
    }
#endif

    totp_state->timestamp = movement_get_utc_timestamp();
    totp_face_set_record(totp_state, 0);
}

static void totp_face_display(totp_lfs_state_t *totp_state) {
    uint8_t index = totp_state->current_index;
    char buf[7];

    if (num_totp_records == 0) {
        watch_display_text(WATCH_POSITION_FULL, "No2F Codes");
        return;
    }

    div_t result = div(totp_state->timestamp, totp_records[index].period);
    if (result.quot != totp_state->steps) {
        totp_state->current_code = getCodeFromTimestamp(totp_state->timestamp);
        totp_state->steps = result.quot;
    }
    uint8_t valid_for = totp_records[index].period - result.rem;

    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, totp_records[index].label, totp_records[index].label);
    sprintf(buf, "%2d", valid_for);
    watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, buf, buf);
    sprintf(buf, "%06lu", totp_state->current_code);
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
}

bool totp_lfs_face_loop(movement_event_t event, void *context) {

    totp_lfs_state_t *totp_state = (totp_lfs_state_t *)context;

    switch (event.event_type) {
        case EVENT_TICK:
            totp_state->timestamp++;
            totp_face_display(totp_state);
            break;
        case EVENT_ACTIVATE:
            totp_face_display(totp_state);
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        case EVENT_ALARM_BUTTON_UP:
            totp_face_set_record(totp_state, (totp_state->current_index + 1) % num_totp_records);
            totp_face_display(totp_state);
            break;
        case EVENT_LIGHT_BUTTON_UP:
            totp_face_set_record(totp_state, (totp_state->current_index + num_totp_records - 1) % num_totp_records);
            totp_face_display(totp_state);
            break;
        case EVENT_ALARM_BUTTON_DOWN:
        case EVENT_ALARM_LONG_PRESS:
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        case EVENT_LIGHT_LONG_PRESS:
            movement_illuminate_led();
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void totp_lfs_face_resign(void *context) {
    (void) context;
}
