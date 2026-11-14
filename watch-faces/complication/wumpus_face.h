/*
 * MIT License
 *
 * Copyright (c) 2024 Evgeny Stepanischev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef WUMPUS_FACE_H_
#define WUMPUS_FACE_H_

#include "movement.h"

/*
 * Hunt the Wumpus
 * ===============
 *
 * Explore a dodecahedral cave of 20 rooms, each connected to three others.
 * Hunt the Wumpus with five arrows while avoiding two pits and two giant bats.
 * In this variant, entering any hazard's room is fatal; bats do not transport you.
 *
 * LIGHT cycles between GO and SHOT; ALARM confirms the selection.
 * GO: LIGHT cycles through the three neighboring rooms and your current room.
 * ALARM moves to the selected room; selecting the current room cancels the move.
 * SHOT: choose a length of 1-5 rooms, then confirm each room along the arrow's
 * route. The first target is a neighbor; subsequent targets can be any room.
 * If a target is not connected to the arrow's actual position, it follows a
 * random tunnel. Arrows can kill bats, continue past them, and hit the player.
 * A hit on the Wumpus wins, including with the last arrow; a fifth miss loses.
 *
 * The top-right digits show the current or selected room (numbered 1-20).
 * Bottom-right warnings cycle through nearby hazards: W (wide W on the original
 * LCD), Bt (bat), and Pt (pit). Draw a map as you explore: you cannot see beyond
 * the adjacent rooms. DIED shows the cause of death; Ar means an arrow-related
 * death. Great and a green backlight indicate victory. Results last three
 * seconds, then a fresh game starts automatically.
 *
 * After a completed move or missed shot, the Wumpus has a 75% chance to move
 * through one tunnel. Menu selections and cancelled moves do not advance time.
 * Pits and bats remain in place when the Wumpus visits or leaves their rooms.
 *
 * MODE switches faces. The current game and partially entered shot are kept in
 * the face's context while away; the inactivity timeout returns to face 0.
 * State is retained in RAM until reset, without writing to flash.
 * Supports both the original Casio LCD and the custom LCD.
 *
 * To enable, add wumpus_face to the watch_faces array in movement_config.h.
 * Native regression tests: make -C tests/wumpus
 */

void wumpus_face_setup(uint8_t watch_face_index, void **context_ptr);
void wumpus_face_activate(void *context);
bool wumpus_face_loop(movement_event_t event, void *context);
void wumpus_face_resign(void *context);

#define wumpus_face ((const watch_face_t){ \
    .setup = wumpus_face_setup, \
    .activate = wumpus_face_activate, \
    .loop = wumpus_face_loop, \
    .resign = wumpus_face_resign, \
    .advise = NULL, \
})

#endif // WUMPUS_FACE_H_
