/* SPDX-License-Identifier: MIT */

/*
 * MIT License
 *
 * Copyright © 2021-2022 Joey Castillo <joeycastillo@utexas.edu> <jose.castillo@gmail.com>
 * Copyright © 2022 Alexsander Akers <me@a2.io>
 * Copyright © 2023 Alex Utter <ooterness@gmail.com>
 * Copyright © 2024 Matheus Afonso Martins Moreira <matheus.a.m.moreira@gmail.com> (https://www.matheusmoreira.com/)
 * Copyright © 2026 Craig McQueen <craig@mcqueen.au>
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

#ifndef METRONOME_FACE_H
#define METRONOME_FACE_H

/*
 * METRONOME face
 *
 * The metronome implements a musical metronome, to tick at a specified tempo in beats per minute
 * (BPM). The user can adjust the tempo between 40 and 180 BPM. The default tempo is 120 BPM.
 */

#include "movement.h"

void metronome_face_setup(uint8_t watch_face_index, void ** context_ptr);
void metronome_face_activate(void *context);
bool metronome_face_loop(movement_event_t event,void *context);
void metronome_face_resign(void *context);

#define metronome_face ((const watch_face_t){ \
    metronome_face_setup, \
    metronome_face_activate, \
    metronome_face_loop, \
    metronome_face_resign, \
    NULL, \
})

#endif // METRONOME_FACE_H
