#include "movement_custom_signal_tunes.h"

#include "movement_tunes_config.h"

#include <stdlib.h>

#include "watch_tcc.h"

const int8_t* signal_tunes[MOVEMENT_N_SIGNAL_TUNES];
const char* signal_tunes_names[MOVEMENT_N_SIGNAL_TUNES];

static const int8_t DEFAULT_NOTE_DURATION = 4;

const int8_t default_signal_tune[] = {
    BUZZER_NOTE_C8, 5,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_C8, 5,
    0
};

static const char* default_signal_tune_name = "SIGNAL";

int8_t* signal_tune = default_signal_tune;
static uint8_t active_signal_tune_index = 0;

#if defined SIGNAL_TUNE_ZELDA_SECRET || defined INCLUDE_SIGNAL_TUNE_ZELDA_SECRET
static const char* signal_tune_zelda_name = " zELdA";
static const int8_t signal_tune_zelda[] = {
    BUZZER_NOTE_G5, 8,
    BUZZER_NOTE_F5SHARP_G5FLAT, 8,
    BUZZER_NOTE_D5SHARP_E5FLAT, 8,
    BUZZER_NOTE_A4, 8,
    BUZZER_NOTE_G4SHARP_A4FLAT, 8,
    BUZZER_NOTE_E5, 8,
    BUZZER_NOTE_G5SHARP_A5FLAT, 8,
    BUZZER_NOTE_C6, 20,
    0
};
#endif // SIGNAL_TUNE_ZELDA_SECRET || INCLUDE_SIGNAL_TUNE_ZELDA_SECRET

#if defined SIGNAL_TUNE_MARIO_THEME || defined INCLUDE_SIGNAL_TUNE_MARIO_THEME
static const char* signal_tune_mario_name = " MARIO";
static const int8_t signal_tune_mario[] = {
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
#endif // SIGNAL_TUNE_MARIO_THEME || INCLUDE_SIGNAL_TUNE_MARIO_THEME

#if defined SIGNAL_TUNE_MARIO_1UP || defined INCLUDE_SIGNAL_TUNE_MARIO_1UP
static const int8_t MARIO_1UP_EIGHTH_NOTE = 6;

static const char* signal_tune_mario_1up_name = " 1 UP";
static const int8_t signal_tune_mario_1up[] = {
    BUZZER_NOTE_E6, DEFAULT_NOTE_DURATION,
    BUZZER_NOTE_REST, MARIO_1UP_EIGHTH_NOTE - DEFAULT_NOTE_DURATION,
    BUZZER_NOTE_G6, DEFAULT_NOTE_DURATION,
    BUZZER_NOTE_REST, MARIO_1UP_EIGHTH_NOTE - DEFAULT_NOTE_DURATION,
    BUZZER_NOTE_E7, DEFAULT_NOTE_DURATION,
    BUZZER_NOTE_REST, MARIO_1UP_EIGHTH_NOTE - DEFAULT_NOTE_DURATION,
    BUZZER_NOTE_C7, DEFAULT_NOTE_DURATION,
    BUZZER_NOTE_REST, MARIO_1UP_EIGHTH_NOTE - DEFAULT_NOTE_DURATION,
    BUZZER_NOTE_D7, DEFAULT_NOTE_DURATION,
    BUZZER_NOTE_REST, MARIO_1UP_EIGHTH_NOTE - DEFAULT_NOTE_DURATION,
    BUZZER_NOTE_G7, DEFAULT_NOTE_DURATION,
    BUZZER_NOTE_REST, MARIO_1UP_EIGHTH_NOTE - DEFAULT_NOTE_DURATION,
    0
};
#endif // SIGNAL_TUNE_MARIO_1UP || INCLUDE_SIGNAL_TUNE_MARIO_1UP

#if defined SIGNAL_TUNE_MARIO_PUP || defined INCLUDE_SIGNAL_TUNE_MARIO_PUP
static const int8_t MARIO_PUP_NOTE_DURATION = 2;

static const char* signal_tune_mario_pup_name = " P UP";
static const int8_t signal_tune_mario_pup[] = {
    BUZZER_NOTE_C5, MARIO_PUP_NOTE_DURATION,

    BUZZER_NOTE_C5, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_E5, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_G5, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_C6, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_G5, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_C6, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_E6, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_G6, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_C7, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_G6, MARIO_PUP_NOTE_DURATION,

    BUZZER_NOTE_G5SHARP_A5FLAT, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_C6, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_D6SHARP_E6FLAT, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_G6SHARP_A6FLAT, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_E6, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_A6, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_C7, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_D7SHARP_E7FLAT, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_G7SHARP_A7FLAT, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_E7, MARIO_PUP_NOTE_DURATION,

    BUZZER_NOTE_A5SHARP_B5FLAT, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_D6, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_F6, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_A6SHARP_B6FLAT, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_F6, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_B6, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_D7, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_F7, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_A7SHARP_B7FLAT, MARIO_PUP_NOTE_DURATION,
    BUZZER_NOTE_F7, MARIO_PUP_NOTE_DURATION,

    0,
};
#endif // SIGNAL_TUNE_MARIO_PUP || INCLUDE_SIGNAL_TUNE_MARIO_PUP

#if defined SIGNAL_TUNE_MGS_CODEC || defined INCLUDE_SIGNAL_TUNE_MGS_CODEC
static const char* signal_tune_msg_codec_name = " NGS c";
static const int8_t signal_tune_msg_codec[] = {
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    0
};
#endif // SIGNAL_TUNE_MGS_CODEC || INCLUDE_SIGNAL_TUNE_MGS_CODEC

#if defined SIGNAL_TUNE_KIM_POSSIBLE || defined INCLUDE_SIGNAL_TUNE_KIM_POSSIBLE
static const char* signal_tune_kim_name = "KiMpOs";
static const int8_t signal_tune_kim[] = {
    BUZZER_NOTE_G7, 6,
    BUZZER_NOTE_G4, 2,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_G7, 6,
    BUZZER_NOTE_G4, 2,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_A7SHARP_B7FLAT, 6,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_G7, 6,
    BUZZER_NOTE_G4, 2,
    0
};
#endif // SIGNAL_TUNE_KIM_POSSIBLE || INCLUDE_SIGNAL_TUNE_KIM_POSSIBLE

#if defined SIGNAL_TUNE_POWER_RANGERS || defined INCLUDE_SIGNAL_TUNE_POWER_RANGERS
static const char* signal_tune_power_rangers_name = " POVEr";
static const int8_t signal_tune_power_rangers[] = {
    BUZZER_NOTE_D8, 6,
    BUZZER_NOTE_REST, 8,
    BUZZER_NOTE_D8, 6,
    BUZZER_NOTE_REST, 8,
    BUZZER_NOTE_C8, 6,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_D8, 6,
    BUZZER_NOTE_REST, 8,
    BUZZER_NOTE_F8, 6,
    BUZZER_NOTE_REST, 8,
    BUZZER_NOTE_D8, 6,
    0
};
#endif // SIGNAL_TUNE_POWER_RANGERS || INCLUDE_SIGNAL_TUNE_POWER_RANGERS

#if defined SIGNAL_TUNE_LAYLA || defined INCLUDE_SIGNAL_TUNE_LAYLA
static const char* signal_tune_layla_name = " LAYLA";
static const int8_t signal_tune_layla[] = {
    BUZZER_NOTE_A6, 5,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_C7, 5,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_D7, 5,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F7, 5,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_D7, 5,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_C7, 5,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_D7, 20,
    0
};
#endif // SIGNAL_TUNE_LAYLA || INCLUDE_SIGNAL_TUNE_LAYLA

#if defined SIGNAL_TUNE_HARRY_POTTER_SHORT || defined INCLUDE_SIGNAL_TUNE_HARRY_POTTER_SHORT
static const char* signal_tune_harry_short_name = "HP SHO";
static const int8_t signal_tune_harry_short[] = {
    BUZZER_NOTE_B5, 12,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_E6, 12,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_G6, 6,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F6SHARP_G6FLAT, 6,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_E6, 16,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_B6, 8,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_A6, 24,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F6SHARP_G6FLAT, 24,
    0
};
#endif // SIGNAL_TUNE_HARRY_POTTER_SHORT || INCLUDE_SIGNAL_TUNE_HARRY_POTTER_SHORT

#if defined SIGNAL_TUNE_HARRY_POTTER_LONG || defined INCLUDE_SIGNAL_TUNE_HARRY_POTTER_LONG
static const char* signal_tune_harry_long_name = "HP LON";
static const int8_t signal_tune_harry_long[] = {
    BUZZER_NOTE_B5, 12,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_E6, 12,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_G6, 6,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F6SHARP_G6FLAT, 6,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_E6, 16,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_B6, 8,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_A6, 24,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F6SHARP_G6FLAT, 24,
    BUZZER_NOTE_REST, 1,

    BUZZER_NOTE_E6, 12,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_G6, 6,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F6SHARP_G6FLAT, 6,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_D6SHARP_E6FLAT, 16,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F6, 8,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_B5, 24,

    0
};
#endif // SIGNAL_TUNE_HARRY_POTTER_LONG || INCLUDE_SIGNAL_TUNE_HARRY_POTTER_LONG

#if defined SIGNAL_TUNE_JURASSIC_PARK || defined INCLUDE_SIGNAL_TUNE_JURASSIC_PARK
static const char* signal_tune_jurassic_name = " J P";
static const int8_t signal_tune_jurassic[] = {
    BUZZER_NOTE_B5, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_A5SHARP_B5FLAT, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_B5, 13,
    BUZZER_NOTE_REST, 13,
    BUZZER_NOTE_F5SHARP_G5FLAT, 13,
    BUZZER_NOTE_REST, 13,
    BUZZER_NOTE_E5, 13,
    0,
};
#endif // SIGNAL_TUNE_JURASSIC_PARK || INCLUDE_SIGNAL_TUNE_JURASSIC_PARK

#if defined SIGNAL_TUNE_EVANGELION || defined INCLUDE_SIGNAL_TUNE_EVANGELION
static const char* signal_tune_evangelion_name = "EVANGE";
static const int8_t signal_tune_evangelion[] = {
    BUZZER_NOTE_C5, 13,
    BUZZER_NOTE_REST, 13,
    BUZZER_NOTE_D5SHARP_E5FLAT, 13,
    BUZZER_NOTE_REST, 13,
    BUZZER_NOTE_F5, 13,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_D5SHARP_E5FLAT, 13,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_F5, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_F5, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_F5, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_A5SHARP_B5FLAT, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_G5SHARP_A5FLAT, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_G5, 3,
    BUZZER_NOTE_REST, 3,
    BUZZER_NOTE_F5, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_G5, 13,
    0,
};
#endif // SIGNAL_TUNE_EVANGELION || INCLUDE_SIGNAL_TUNE_HARRY_POTTER_LONG


void movement_custom_signal_tunes_init(void) {
    // memset((void*)signal_tunes, 0, sizeof(int8_t*) * MOVEMENT_N_SIGNAL_TUNES);
    // memset((void*)signal_tunes_names, 0, sizeof(char*) * MOVEMENT_N_SIGNAL_TUNES);

    uint8_t signal_tune_index = 0;

    signal_tunes[signal_tune_index] = default_signal_tune;
    signal_tunes_names[signal_tune_index] = default_signal_tune_name;

#if defined SIGNAL_TUNE_ZELDA_SECRET || defined INCLUDE_SIGNAL_TUNE_ZELDA_SECRET
    signal_tune_index++;
    signal_tunes[signal_tune_index] = signal_tune_zelda;
    signal_tunes_names[signal_tune_index] = signal_tune_zelda_name;
#if defined SIGNAL_TUNE_ZELDA_SECRET
    active_signal_tune_index = signal_tune_index;
#endif // SIGNAL_TUNE_ZELDA_SECRET
#endif

#if defined SIGNAL_TUNE_MARIO_THEME || defined INCLUDE_SIGNAL_TUNE_MARIO_THEME
    signal_tune_index++;
    signal_tunes[signal_tune_index] = signal_tune_mario;
    signal_tunes_names[signal_tune_index] = signal_tune_mario_name;
#if defined SIGNAL_TUNE_MARIO_THEME
    active_signal_tune_index = signal_tune_index;
#endif // SIGNAL_TUNE_MARIO_THEME
#endif

#if defined SIGNAL_TUNE_MARIO_1UP || defined INCLUDE_SIGNAL_TUNE_MARIO_1UP
    signal_tune_index++;
    signal_tunes[signal_tune_index] = signal_tune_mario_1up;
    signal_tunes_names[signal_tune_index] = signal_tune_mario_1up_name;
#if defined SIGNAL_TUNE_MARIO_1UP
    active_signal_tune_index = signal_tune_index;
#endif // SIGNAL_TUNE_MARIO_1UP
#endif

#if defined SIGNAL_TUNE_MARIO_PUP || defined INCLUDE_SIGNAL_TUNE_MARIO_PUP
    signal_tune_index++;
    signal_tunes[signal_tune_index] = signal_tune_mario_pup;
    signal_tunes_names[signal_tune_index] = signal_tune_mario_pup_name;
#if defined SIGNAL_TUNE_MARIO_PUP
    active_signal_tune_index = signal_tune_index;
#endif // SIGNAL_TUNE_MARIO_PUP
#endif

#if defined SIGNAL_TUNE_MGS_CODEC || defined INCLUDE_SIGNAL_TUNE_MGS_CODEC
    signal_tune_index++;
    signal_tunes[signal_tune_index] = signal_tune_msg_codec;
    signal_tunes_names[signal_tune_index] = signal_tune_msg_codec_name;
#if defined SIGNAL_TUNE_MGS_CODEC
    active_signal_tune_index = signal_tune_index;
#endif // SIGNAL_TUNE_MGS_CODEC
#endif

#if defined SIGNAL_TUNE_KIM_POSSIBLE || defined INCLUDE_SIGNAL_TUNE_KIM_POSSIBLE
    signal_tune_index++;
    signal_tunes[signal_tune_index] = signal_tune_kim;
    signal_tunes_names[signal_tune_index] = signal_tune_kim_name;
#if defined SIGNAL_TUNE_KIM_POSSIBLE
    active_signal_tune_index = signal_tune_index;
#endif // SIGNAL_TUNE_KIM_POSSIBLE
#endif

#if defined SIGNAL_TUNE_POWER_RANGERS || defined INCLUDE_SIGNAL_TUNE_POWER_RANGERS
    signal_tune_index++;
    signal_tunes[signal_tune_index] = signal_tune_power_rangers;
    signal_tunes_names[signal_tune_index] = signal_tune_power_rangers_name;
#if defined SIGNAL_TUNE_POWER_RANGERS
    active_signal_tune_index = signal_tune_index;
#endif // SIGNAL_TUNE_POWER_RANGERS
#endif

#if defined SIGNAL_TUNE_LAYLA || defined INCLUDE_SIGNAL_TUNE_LAYLA
    signal_tune_index++;
    signal_tunes[signal_tune_index] = signal_tune_layla;
    signal_tunes_names[signal_tune_index] = signal_tune_layla_name;
#if defined SIGNAL_TUNE_LAYLA
    active_signal_tune_index = signal_tune_index;
#endif // SIGNAL_TUNE_LAYLA
#endif

#if defined SIGNAL_TUNE_HARRY_POTTER_SHORT || defined INCLUDE_SIGNAL_TUNE_HARRY_POTTER_SHORT
    signal_tune_index++;
    signal_tunes[signal_tune_index] = signal_tune_harry_short;
    signal_tunes_names[signal_tune_index] = signal_tune_harry_short_name;
#if defined SIGNAL_TUNE_HARRY_POTTER_SHORT
    active_signal_tune_index = signal_tune_index;
#endif // SIGNAL_TUNE_HARRY_POTTER_SHORT
#endif

#if defined SIGNAL_TUNE_HARRY_POTTER_LONG || defined INCLUDE_SIGNAL_TUNE_HARRY_POTTER_LONG
    signal_tune_index++;
    signal_tunes[signal_tune_index] = signal_tune_harry_long;
    signal_tunes_names[signal_tune_index] = signal_tune_harry_long_name;
#if defined SIGNAL_TUNE_HARRY_POTTER_LONG
    active_signal_tune_index = signal_tune_index;
#endif // SIGNAL_TUNE_HARRY_POTTER_LONG
#endif

#if defined SIGNAL_TUNE_JURASSIC_PARK || defined INCLUDE_SIGNAL_TUNE_JURASSIC_PARK
    signal_tune_index++;
    signal_tunes[signal_tune_index] = signal_tune_jurassic;
    signal_tunes_names[signal_tune_index] = signal_tune_jurassic_name;
#if defined SIGNAL_TUNE_JURASSIC_PARK
    active_signal_tune_index = signal_tune_index;
#endif // SIGNAL_TUNE_JURASSIC_PARK
#endif

#if defined SIGNAL_TUNE_EVANGELION || defined INCLUDE_SIGNAL_TUNE_EVANGELION
    signal_tune_index++;
    signal_tunes[signal_tune_index] = signal_tune_evangelion;
    signal_tunes_names[signal_tune_index] = signal_tune_evangelion_name;
#if defined SIGNAL_TUNE_EVANGELION
    active_signal_tune_index = signal_tune_index;
#endif // SIGNAL_TUNE_EVANGELION
#endif

    signal_tune = signal_tunes[active_signal_tune_index];
}

uint8_t movement_custom_signal_tunes_get_active_tune_index() {
    return active_signal_tune_index;
}

void movement_custom_signal_tunes_set_active_tune_index(uint8_t index) {
    active_signal_tune_index = index;
    signal_tune = signal_tunes[active_signal_tune_index];
}
