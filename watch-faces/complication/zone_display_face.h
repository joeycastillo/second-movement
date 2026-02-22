/*
 * MIT License
 *
 * Copyright (c) 2026 Diego Perez
 *
 * Zone Display Face
 * 
 * Dynamic face that displays the current metric from the active playlist.
 * Queries playlist_get_current_face() to determine which metric to show,
 * then renders metric abbreviation + value on line 1 and trend on line 2.
 * 
 * LCD Format (7-segment, no arrows):
 *   SD  42    (Line 1: metric abbrev + value)
 *   -  -3     (Line 2: trend symbol +/-/= + change)
 * 
 * Button behavior:
 *   - ALARM: Cycles through zone playlist (calls playlist_advance())
 *   - MODE long-press: Returns to clock (exits playlist mode)
 */

#ifndef ZONE_DISPLAY_FACE_H_
#define ZONE_DISPLAY_FACE_H_

#include "movement.h"

#ifdef PHASE_ENGINE_ENABLED

typedef struct {
    uint8_t placeholder;  // No persistent state needed
} zone_display_face_state_t;

void zone_display_face_setup(uint8_t watch_face_index, void **context_ptr);
void zone_display_face_activate(void *context);
bool zone_display_face_loop(movement_event_t event, void *context);
void zone_display_face_resign(void *context);

#define zone_display_face ((const watch_face_t){ \
    zone_display_face_setup, \
    zone_display_face_activate, \
    zone_display_face_loop, \
    zone_display_face_resign, \
    NULL, \
})

#endif // PHASE_ENGINE_ENABLED

#endif // ZONE_DISPLAY_FACE_H_
