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
    BUZZER_NOTE_A5 = 7,                   // For countdown
} watch_buzzer_note_t;

// Mock note periods (dummy values for testing)
static const uint16_t NotePeriods[] = {
    0,      // REST
    100,    // D7
    110,    // D7#/E7b
    120,    // E7
    130,    // F7#/G7b
    140,    // G7
    150,    // G7#/A7b
    200,    // A5
};

#define WATCH_BUZZER_PERIOD_REST 0

#endif // WATCH_TCC_H
