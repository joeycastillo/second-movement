#include <stdlib.h>
#include <string.h>

#include "watch.h"
#include "wyoscan_face.h"

#define MAX_ILLUMINATED_SEGMENTS 16

typedef struct {
    uint8_t animation;
    uint8_t total_frames;
    bool animate;

    bool colon;
    uint8_t position;
    uint8_t segment;
    uint8_t time_digits[6];

    uint8_t start;
    uint8_t end;
    uint8_t illuminated_segments[MAX_ILLUMINATED_SEGMENTS][2];
} wyoscan_state_t;

static const char *const segment_map[10] = {
    "AXFBDEXC",
    "BXXXCXXX",
    "ABGEXXXD",
    "ABGXXXCD",
    "FXGBXXXC",
    "AXFXGXCD",
    "AXFEDCXG",
    "AXXBXXCX",
    "AFGCDEXB",
    "AFGBXXCD",
};

static const uint8_t clock_mapping[6][7][2] = {
    { {1, 18}, {2, 19}, {0, 19}, {1, 18}, {0, 18}, {2, 18}, {1, 19} },
    { {2, 20}, {2, 21}, {1, 21}, {0, 21}, {0, 20}, {1, 17}, {1, 20} },
    { {0, 22}, {2, 23}, {0, 23}, {0, 22}, {1, 22}, {2, 22}, {1, 23} },
    { {2, 1},  {2, 10}, {0, 1},  {0, 0},  {1, 0},  {2, 0},  {1, 1}  },
    { {2, 2},  {2, 3},  {0, 4},  {0, 3},  {0, 2},  {1, 2},  {1, 3}  },
    { {2, 4},  {2, 5},  {1, 6},  {0, 6},  {0, 5},  {1, 4},  {1, 5}  },
};

static void wyoscan_reset_ring(wyoscan_state_t *state) {
    state->start = 0;
    state->end = 0;
    for (uint8_t i = 0; i < MAX_ILLUMINATED_SEGMENTS; i++) {
        state->illuminated_segments[i][0] = 0xFF;
        state->illuminated_segments[i][1] = 0xFF;
    }
}

static void wyoscan_capture_time_digits(wyoscan_state_t *state) {
    watch_date_time_t dt = watch_rtc_get_date_time();
    state->time_digits[0] = (uint8_t)(dt.unit.hour / 10);
    state->time_digits[1] = (uint8_t)(dt.unit.hour % 10);
    state->time_digits[2] = (uint8_t)(dt.unit.minute / 10);
    state->time_digits[3] = (uint8_t)(dt.unit.minute % 10);
    state->time_digits[4] = (uint8_t)(dt.unit.second / 10);
    state->time_digits[5] = (uint8_t)(dt.unit.second % 10);
}

void wyoscan_face_setup(uint8_t watch_face_index, void **context_ptr) {
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(wyoscan_state_t));
        memset(*context_ptr, 0, sizeof(wyoscan_state_t));
    }
    wyoscan_state_t *state = (wyoscan_state_t *)(*context_ptr);
    (void)watch_face_index;
    state->total_frames = 64;
    wyoscan_reset_ring(state);
}

void wyoscan_face_activate(void *context) {
    wyoscan_state_t *state = (wyoscan_state_t *)context;
    movement_request_tick_frequency(32);
    state->total_frames = 64;
    state->animate = false;
    state->animation = 0;
}

void wyoscan_face_resign(void *context) {
    (void)context;
    movement_request_tick_frequency(1);
}

bool wyoscan_face_loop(movement_event_t event, void *context) {
    wyoscan_state_t *state = (wyoscan_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            break;

        case EVENT_TICK:
            if (!state->animate) {
                wyoscan_capture_time_digits(state);
                state->animation = 0;
                state->animate = true;
                wyoscan_reset_ring(state);
            }

            if (state->animate) {
                if ((uint8_t)((state->end + 1) % MAX_ILLUMINATED_SEGMENTS) == state->start) {
                    uint8_t ox = state->illuminated_segments[state->start][0];
                    uint8_t oy = state->illuminated_segments[state->start][1];
                    if (ox != 0xFF && oy != 0xFF) {
                        watch_clear_pixel(ox, oy);
                    }
                    state->start = (uint8_t)((state->start + 1) % MAX_ILLUMINATED_SEGMENTS);
                }

                if ((state->animation % 32) == 0) {
                    if (state->colon) {
                        watch_set_colon();
                    } else {
                        watch_clear_colon();
                    }
                    state->colon = !state->colon;
                }

                if (state->animation < (uint8_t)(state->total_frames - MAX_ILLUMINATED_SEGMENTS)) {
                    state->position = (uint8_t)((state->animation / 8) % 6);

                    uint8_t digit = state->time_digits[state->position];
                    const char *segments = segment_map[digit];

                    state->segment = (uint8_t)(state->animation % 8);
                    char seg = segments[state->segment];

                    if (seg == 'X') {
                        state->illuminated_segments[state->end][0] = 0xFF;
                        state->illuminated_segments[state->end][1] = 0xFF;
                        state->end = (uint8_t)((state->end + 1) % MAX_ILLUMINATED_SEGMENTS);
                        state->animation++;
                        break;
                    }

                    uint8_t seg_idx = (uint8_t)(seg - 'A');
                    uint8_t x = clock_mapping[state->position][seg_idx][0];
                    uint8_t y = clock_mapping[state->position][seg_idx][1];

                    watch_set_pixel(x, y);

                    state->illuminated_segments[state->end][0] = x;
                    state->illuminated_segments[state->end][1] = y;
                    state->end = (uint8_t)((state->end + 1) % MAX_ILLUMINATED_SEGMENTS);

                } else if (state->animation < state->total_frames) {
                    state->end = (uint8_t)((state->end + 1) % MAX_ILLUMINATED_SEGMENTS);
                } else {
                    state->animate = false;
                }

                state->animation++;
            }
            break;

        case EVENT_LOW_ENERGY_UPDATE:
        case EVENT_BACKGROUND_TASK:
            break;

        default:
            return movement_default_loop_handler(event);
    }

    return true;
}
