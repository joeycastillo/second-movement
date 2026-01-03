/*
 * Mock watch_tcc.h for testing
 * Provides minimal definitions needed for fesk_tx unit tests
 */

#ifndef WATCH_TCC_H
#define WATCH_TCC_H

#include <stdint.h>

// Mock buzzer note type
typedef enum {
    BUZZER_NOTE_REST = 0,
    BUZZER_NOTE_D7 = 1,                   // 2349 Hz (for FESK)
    BUZZER_NOTE_D7SHARP_E7FLAT = 2,       // 2489.02 Hz
    BUZZER_NOTE_E7 = 3,                   // 2637 Hz (for FESK)
    BUZZER_NOTE_F7SHARP_G7FLAT = 4,       // 2960 Hz (for FESK)
    BUZZER_NOTE_G7 = 5,                   // 3135.96 Hz
    BUZZER_NOTE_G7SHARP_A7FLAT = 6,       // 3322 Hz (for FESK)
} watch_buzzer_note_t;

#endif // WATCH_TCC_H
