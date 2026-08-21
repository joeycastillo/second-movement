/*
 * MIT License
 *
 * Copyright (c) 2026
 *
 */

#include <stdlib.h>
#include <string.h>

#include "music_player_face.h"
#include "watch.h"

// Sample tune defined inside this face. Ends with a single 0 value.
static int8_t signal_tune[] = {
    BUZZER_NOTE_E6, 7,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_E6, 7,
    BUZZER_NOTE_REST, 10,
    BUZZER_NOTE_E6, 7,
    BUZZER_NOTE_REST, 11,
    BUZZER_NOTE_C6, 7,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_E6, 7,
    BUZZER_NOTE_REST, 10,
    BUZZER_NOTE_G6, 8,
    BUZZER_NOTE_REST, 30,
    BUZZER_NOTE_G5, 8,
    0
};

static void _music_player_face_update_display(music_player_face_state_t *state) {
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "MUSIC", "MU");
    watch_display_text(WATCH_POSITION_BOTTOM, state->playing ? "PLAY" : "STOP");
}

void music_player_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void)watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(music_player_face_state_t));
        memset(*context_ptr, 0, sizeof(music_player_face_state_t));
    }
}

void music_player_face_activate(void *context) {
    music_player_face_state_t *state = (music_player_face_state_t *)context;
    if (state != NULL) {
        watch_set_colon();
        _music_player_face_update_display(state);
    }
}

bool music_player_face_loop(movement_event_t event, void *context) {
    music_player_face_state_t *state = (music_player_face_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _music_player_face_update_display(state);
            break;
        case EVENT_ALARM_BUTTON_UP:
            if (state->playing) {
                watch_buzzer_abort_sequence();
                state->playing = false;
            } else {
                watch_buzzer_play_sequence((int8_t *)signal_tune, NULL);
                state->playing = true;
            }
            _music_player_face_update_display(state);
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void music_player_face_resign(void *context) {
    (void)context;
}
