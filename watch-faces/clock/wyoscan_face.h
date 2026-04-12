#ifndef WYOSCAN_FACE_H_
#define WYOSCAN_FACE_H_

/*
 * Wyoscan-style segment scan (Halmos / Dexter Sinister–inspired).
 * See https://www.o-r-g.com/apps/wyoscan
 *
 * Port notes (vs legacy/): uses public watch_set_pixel APIs only (no watch_private_display.h).
 */

#include "movement.h"

void wyoscan_face_setup(uint8_t watch_face_index, void **context_ptr);
void wyoscan_face_activate(void *context);
bool wyoscan_face_loop(movement_event_t event, void *context);
void wyoscan_face_resign(void *context);

#define wyoscan_face ((const watch_face_t){ \
    wyoscan_face_setup, \
    wyoscan_face_activate, \
    wyoscan_face_loop, \
    wyoscan_face_resign, \
    NULL, \
})

#endif // WYOSCAN_FACE_H_
