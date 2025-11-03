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
    BUZZER_NOTE_D7SHARP_E7FLAT = 1,  // 2489.02 Hz
    BUZZER_NOTE_G7 = 2,               // 3135.96 Hz
} watch_buzzer_note_t;

#endif // WATCH_TCC_H
