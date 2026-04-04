#include "movement.h"
#include <time.h>

static bool movement_time_override_active = false;
static watch_date_time_t movement_time_override;

bool movement_default_loop_handler(movement_event_t event) {
    (void) event;
    return true;
}

void movement_move_to_face(uint8_t watch_face_index) {
    (void) watch_face_index;
}

void movement_move_to_next_face(void) {
}

watch_date_time_t movement_get_local_date_time(void) {
    if (movement_time_override_active) {
        return movement_time_override;
    }
    watch_date_time_t date_time = {0};
    // Set it to current time.
    time_t current_time;
    current_time = time(NULL);
    struct tm *time_info;
    time_info = gmtime(&current_time);
    date_time.unit.year = (time_info->tm_year + 1900) - 2020;
    date_time.unit.month = time_info->tm_mon + 1;
    date_time.unit.day = time_info->tm_mday;
    date_time.unit.hour = time_info->tm_hour;
    date_time.unit.minute = time_info->tm_min;
    date_time.unit.second = time_info->tm_sec;
    return date_time;
}

int32_t movement_get_current_timezone_offset_for_zone(uint8_t zone_index) {
    (void) zone_index;
    return 0;
}

int32_t movement_get_current_timezone_offset(void) {
    return 0;
}

void movement_request_tick_frequency(uint8_t freq) {
    (void)freq;
}

void movement_mock_set_local_date_time(watch_date_time_t date_time) {
    movement_time_override = date_time;
    movement_time_override_active = true;
}

void movement_mock_clear_local_date_time(void) {
    movement_time_override_active = false;
}
