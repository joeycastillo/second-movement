#include "watch.h"
#include <time.h>
#include <stdio.h>
#include <string.h>

void watch_set_led_off(void) {}

// Create a struct to hold the current display text. It should be indexed by the different positions.
static char display_text[WATCH_POSITION_SECONDS + 1][16];

void watch_display_text(watch_position_t location, const char *string) {
    if (location > WATCH_POSITION_SECONDS || !string) {
        return;
    }
    strncpy(display_text[location], string, sizeof(display_text[location]) - 1);
    display_text[location][sizeof(display_text[location]) - 1] = '\0';
}

char *watch_get_display_text(watch_position_t location) {
    if (location > WATCH_POSITION_SECONDS) {
        return NULL;
    }
    // Now return the string at that location.
    return display_text[location];
}

void watch_display_text_with_fallback(watch_position_t location,
                                      const char *string,
                                      const char *fallback) {
    const char *selected = (string && string[0] != '\0') ? string : fallback;
    if (!selected) {
        selected = "";
    }
    watch_display_text(location, selected);
}

uint32_t watch_utility_date_time_to_unix_time(watch_date_time_t date_time, int32_t utc_offset) {
    // Convert from watch_date_time_t to tm.
    struct tm time_info;
    time_info.tm_sec = date_time.unit.second;
    time_info.tm_min = date_time.unit.minute;
    time_info.tm_hour = date_time.unit.hour;
    time_info.tm_mday = date_time.unit.day;
    time_info.tm_mon = date_time.unit.month - 1;
    time_info.tm_year = (date_time.unit.year + 2020) - 1900;
    // This one should be in the UTC timezone.
    time_info.tm_isdst = 0;
    // Now convert to unix time.
    time_t unix_time = mktime(&time_info);
    return unix_time - (utc_offset * 3600);
}

const uint16_t NotePeriods[] = {
    [BUZZER_NOTE_A5] = 1136,
    [BUZZER_NOTE_D7SHARP_E7FLAT] = 402,
    [BUZZER_NOTE_G7] = 319,
    [BUZZER_NOTE_REST] = 0,
};

void watch_set_indicator(watch_indicator_t indicator) {
    (void)indicator;
}

void watch_clear_indicator(watch_indicator_t indicator) {
    (void)indicator;
}

void watch_set_buzzer_period_and_duty_cycle(uint32_t period, uint8_t duty) {
    (void)period;
    (void)duty;
}

void watch_set_buzzer_on(void) {
}

void watch_set_buzzer_off(void) {
}

void watch_buzzer_abort_sequence(void) {
}

void watch_buzzer_play_sequence(int8_t *note_sequence, void (*callback_on_end)(void)) {
    (void)note_sequence;
    if (callback_on_end) {
        callback_on_end();
    }
}

void watch_buzzer_play_raw_source(bool (*raw_source)(uint16_t, void*, uint16_t*, uint16_t*),
                                   void* userdata,
                                   void (*done_callback)(void)) {
    (void)raw_source;
    (void)userdata;
    if (done_callback) {
        done_callback();
    }
}
