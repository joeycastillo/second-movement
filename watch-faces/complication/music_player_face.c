#include <stdlib.h>
#include <string.h>

#include "music_player_face.h"
#include "watch.h"

//Smoke on the water
static int8_t signal_tune[] = {
    BUZZER_NOTE_G4, 7,
    BUZZER_NOTE_REST, 20,

    BUZZER_NOTE_A4SHARP_B4FLAT, 7,
    BUZZER_NOTE_REST, 20,

    BUZZER_NOTE_C5, 7,
    BUZZER_NOTE_REST, 30,

    BUZZER_NOTE_G4, 7,
    BUZZER_NOTE_REST, 20,

    BUZZER_NOTE_A4SHARP_B4FLAT, 7,
    BUZZER_NOTE_REST, 20,

    BUZZER_NOTE_C5SHARP_D5FLAT, 7,
    BUZZER_NOTE_REST, 8,

    BUZZER_NOTE_C5, 7,
    BUZZER_NOTE_REST, 30,

    BUZZER_NOTE_G4, 7,
    BUZZER_NOTE_REST, 20,

    BUZZER_NOTE_A4SHARP_B4FLAT, 7,
    BUZZER_NOTE_REST, 20,

    BUZZER_NOTE_C5, 7,
    BUZZER_NOTE_REST, 30,

    BUZZER_NOTE_A4SHARP_B4FLAT, 7,
    BUZZER_NOTE_REST, 20,

    BUZZER_NOTE_G4, 12,
    0
};


static music_player_face_state_t *music_player_state = NULL;


static void _music_player_face_update_display(
    music_player_face_state_t *state
) {
    watch_display_text_with_fallback(
        WATCH_POSITION_TOP,
        "MUSIC",
        "MU"
    );

    watch_display_text(
        WATCH_POSITION_BOTTOM,
        state->playing ? "PLAY" : "STOP"
    );
}

static void _music_player_face_song_finished(void) {
    if (music_player_state != NULL) {
        music_player_state->playing = false;

        _music_player_face_update_display(music_player_state);
    }
}


void music_player_face_setup(
    uint8_t watch_face_index,
    void **context_ptr
) {
    (void)watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(music_player_face_state_t));

        memset(
            *context_ptr,
            0,
            sizeof(music_player_face_state_t)
        );
    }
}


void music_player_face_activate(void *context) {
    music_player_face_state_t *state =
        (music_player_face_state_t *)context;

    if (state != NULL) {
        music_player_state = state;

        watch_set_colon();

        _music_player_face_update_display(state);
    }
}


bool music_player_face_loop(
    movement_event_t event,
    void *context
) {
    music_player_face_state_t *state =
        (music_player_face_state_t *)context;

    switch (event.event_type) {

        case EVENT_ACTIVATE:
            _music_player_face_update_display(state);
            break;


        case EVENT_ALARM_BUTTON_UP:

            if (state->playing) {
                watch_buzzer_abort_sequence();

                state->playing = false;

            } else {
                watch_buzzer_play_sequence(
                    (int8_t *)signal_tune,
                    _music_player_face_song_finished
                );

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
    if (music_player_state == context) {
        music_player_state = NULL;
    }
}