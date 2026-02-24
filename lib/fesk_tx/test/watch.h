/*
 * Mock watch.h for testing fesk_session
 * Provides minimal definitions needed for unit tests
 */

#ifndef WATCH_H
#define WATCH_H

#include <stdint.h>
#include <stdbool.h>

// Watch indicator constants
#define WATCH_INDICATOR_BELL 0

// Watch position constants
#define WATCH_POSITION_BOTTOM 0

// Function declarations
void watch_display_text(uint8_t position, const char *text);
void watch_set_indicator(uint8_t indicator);
void watch_clear_indicator(uint8_t indicator);
void watch_buzzer_abort_sequence(void);
void watch_set_buzzer_off(void);
void watch_buzzer_play_raw_source(bool (*raw_source)(uint16_t, void*, uint16_t*, uint16_t*),
                                   void* userdata,
                                   void (*done_callback)(void));
void watch_buzzer_play_sequence(int8_t *sequence, void (*done_callback)(void));

#endif // WATCH_H
