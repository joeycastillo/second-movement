#include "unity/src/unity.h"
#include "../complication/uptime_face.h"
#include "movement.h"
#include "watch.h"

#include <stdlib.h>

void setUp(void) {
    movement_mock_clear_local_date_time();
    watch_display_text(WATCH_POSITION_BOTTOM, "");
}

void tearDown(void) {
    movement_mock_clear_local_date_time();
}

static watch_date_time_t make_date_time(uint16_t year,
                                        uint8_t month,
                                        uint8_t day,
                                        uint8_t hour,
                                        uint8_t minute,
                                        uint8_t second) {
    watch_date_time_t dt = {0};
    dt.unit.year = year;
    dt.unit.month = month;
    dt.unit.day = day;
    dt.unit.hour = hour;
    dt.unit.minute = minute;
    dt.unit.second = second;
    return dt;
}

static void test_uptime_face_displays_seconds_since_boot(void) {
    watch_date_time_t boot_time = make_date_time(5, 1, 1, 0, 0, 0); // 2025-01-01T00:00:00Z
    movement_mock_set_local_date_time(boot_time);

    void *context_ptr = NULL;
    uptime_face_setup(0, &context_ptr);
    TEST_ASSERT_NOT_NULL(context_ptr);

    uptime_state_t *state = (uptime_state_t *)context_ptr;
    uptime_face_activate(state);

    uint32_t current_unix = watch_utility_date_time_to_unix_time(boot_time, movement_get_current_timezone_offset());
    state->boot_time = current_unix - 10;

    uptime_face_loop((movement_event_t){.event_type = EVENT_ACTIVATE}, state);
    uptime_face_loop((movement_event_t){.event_type = EVENT_TICK}, state);

    char *displayed_text = watch_get_display_text(WATCH_POSITION_BOTTOM);
    TEST_ASSERT_NOT_NULL(displayed_text);
    TEST_ASSERT_EQUAL_STRING("10s", displayed_text);

    uptime_face_resign(state);
    free(state);
}

static void test_uptime_face_displays_hours_since_boot(void) {
    watch_date_time_t boot_time = make_date_time(5, 1, 1, 0, 0, 0); // 2025-01-01T00:00:00Z
    movement_mock_set_local_date_time(boot_time);

    void *context_ptr = NULL;
    uptime_face_setup(0, &context_ptr);
    TEST_ASSERT_NOT_NULL(context_ptr);

    uptime_state_t *state = (uptime_state_t *)context_ptr;
    uptime_face_activate(state);

    uint32_t current_unix = watch_utility_date_time_to_unix_time(boot_time, movement_get_current_timezone_offset());
    state->boot_time = current_unix - 7287; // 2+ hours ago

    uptime_face_loop((movement_event_t){.event_type = EVENT_ACTIVATE}, state);
    uptime_face_loop((movement_event_t){.event_type = EVENT_TICK}, state);

    char *displayed_text = watch_get_display_text(WATCH_POSITION_BOTTOM);
    TEST_ASSERT_NOT_NULL(displayed_text);
    TEST_ASSERT_EQUAL_STRING("2h", displayed_text);

    uptime_face_resign(state);
    free(state);
}

static void test_uptime_face_displays_mins_since_boot(void) {
    watch_date_time_t boot_time = make_date_time(5, 1, 1, 0, 0, 0); // 2025-01-01T00:00:00Z
    movement_mock_set_local_date_time(boot_time);

    void *context_ptr = NULL;
    uptime_face_setup(0, &context_ptr);
    TEST_ASSERT_NOT_NULL(context_ptr);

    uptime_state_t *state = (uptime_state_t *)context_ptr;
    uptime_face_activate(state);

    uint32_t current_unix = watch_utility_date_time_to_unix_time(boot_time, movement_get_current_timezone_offset());
    state->boot_time = current_unix - 612; // 10+ minutes ago

    uptime_face_loop((movement_event_t){.event_type = EVENT_ACTIVATE}, state);
    uptime_face_loop((movement_event_t){.event_type = EVENT_TICK}, state);

    char *displayed_text = watch_get_display_text(WATCH_POSITION_BOTTOM);
    TEST_ASSERT_NOT_NULL(displayed_text);
    TEST_ASSERT_EQUAL_STRING("10m", displayed_text);

    uptime_face_resign(state);
    free(state);
}

static void test_uptime_face_displays_days_since_boot(void) {
    watch_date_time_t boot_time = make_date_time(5, 1, 1, 0, 0, 0); // 2025-01-01T00:00:00Z
    movement_mock_set_local_date_time(boot_time);

    void *context_ptr = NULL;
    uptime_face_setup(0, &context_ptr);
    TEST_ASSERT_NOT_NULL(context_ptr);

    uptime_state_t *state = (uptime_state_t *)context_ptr;
    uptime_face_activate(state);

    uint32_t current_unix = watch_utility_date_time_to_unix_time(boot_time, movement_get_current_timezone_offset());
    state->boot_time = current_unix - 172920; // 2+ days ago

    uptime_face_loop((movement_event_t){.event_type = EVENT_ACTIVATE}, state);
    uptime_face_loop((movement_event_t){.event_type = EVENT_TICK}, state);

    char *displayed_text = watch_get_display_text(WATCH_POSITION_BOTTOM);
    TEST_ASSERT_NOT_NULL(displayed_text);
    TEST_ASSERT_EQUAL_STRING("2d", displayed_text);

    uptime_face_resign(state);
    free(state);
}


int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_uptime_face_displays_seconds_since_boot);
    RUN_TEST(test_uptime_face_displays_mins_since_boot);
    RUN_TEST(test_uptime_face_displays_hours_since_boot);
    RUN_TEST(test_uptime_face_displays_days_since_boot);
    return UNITY_END();
}
