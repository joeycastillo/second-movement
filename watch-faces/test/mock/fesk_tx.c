#include "fesk_tx.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

fesk_result_t fesk_encode(const char *text,
                          fesk_mode_t mode,
                          int8_t **out_sequence,
                          size_t *out_entries) {
    (void)mode;
    if (!text || !out_sequence) {
        return FESK_ERR_INVALID_ARGUMENT;
    }

    int8_t *sequence = (int8_t *)malloc(sizeof(int8_t));
    if (!sequence) {
        return FESK_ERR_ALLOCATION_FAILED;
    }

    sequence[0] = 0;
    *out_sequence = sequence;
    if (out_entries) {
        *out_entries = 0;
    }
    return FESK_OK;
}

fesk_result_t fesk_encode_text(const char *text,
                               size_t length,
                               int8_t **out_sequence,
                               size_t *out_entries) {
    if (!text || length == 0 || !out_sequence) {
        return FESK_ERR_INVALID_ARGUMENT;
    }

    return fesk_encode(text, FESK_MODE_4FSK, out_sequence, out_entries);
}

fesk_result_t fesk_encode_cstr(const char *text,
                               int8_t **out_sequence,
                               size_t *out_entries) {
    if (!text) {
        return FESK_ERR_INVALID_ARGUMENT;
    }
    return fesk_encode_text(text, strlen(text), out_sequence, out_entries);
}

void fesk_free_sequence(int8_t *sequence) {
    free(sequence);
}

const watch_buzzer_note_t fesk_tone_map_2fsk[FESK_2FSK_TONE_COUNT] = {0};
const watch_buzzer_note_t fesk_tone_map_4fsk[FESK_4FSK_TONE_COUNT] = {0};

bool fesk_lookup_char_code(unsigned char ch, uint8_t *out_code) {
    (void)ch;
    if (out_code) *out_code = 0;
    return true;
}

uint8_t fesk_crc8_update_code(uint8_t crc, uint8_t code) {
    (void)code;
    return crc;
}
