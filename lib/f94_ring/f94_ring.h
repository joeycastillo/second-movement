/*
 * MIT License
 *
 * Copyright © 2026 Konrad Rieck <konrad@mlsec.org>
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

#ifndef F94_RING_H_
#define F94_RING_H_

/*
 * F-94 QUANTITY RING
 *
 * The Casio F-94 shares the F-91W's LCD glass, so the classic segment map
 * drives it unchanged. Only the printed labels differ: the segments the
 * F-91W labels SIGNAL, PM, 24H, LAP and BELL form a ring at the top right
 * of the F-94, labelled AL, PM, 24H, SPL and SIG there (clockwise). The
 * f94_ring API lets a face use that ring as a gauge for a single quantity
 * instead of as five separate indicators. For example, the ring can show
 * progress towards a daily step goal, or the time left until a scheduled alarm.
 *
 * A source for the ring API registers once from its setup() and supplies a
 * function reporting its current level:
 *
 *     static uint8_t _ring_level(void *context)
 *     {
 *         some_state_t *state = (some_state_t *) context;
 *         return f94_ring_scale(state->value, state->max);
 *     }
 *
 *     f94_ring_register("Label", _ring_level, state, F94_RING_DOUBLE);
 *
 * The clock face calls that function when it repaints, about once a second
 * while the source is displayed. Registering on a watch without
 * f94_clock_face is harmless, since nothing calls the source.
 *
 * Registering the same source twice is harmless: the entry is updated in
 * place and keeps its id. This matters because movement calls setup() again
 * after every wake from deep sleep.
 *
 * There are two modes of operation: F94_RING_FILL and F94_RING_DOUBLE. The
 * former fills the ring and stops there (5 steps), the latter fills the ring
 * and then clears it again (10 steps).
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Segments in the ring */
#define F94_RING_SEGMENTS 5

/* Highest level an F94_RING_FILL source can report */
#define F94_RING_FILL_MAX F94_RING_SEGMENTS

/* Highest level an F94_RING_DOUBLE source can report */
#define F94_RING_DOUBLE_MAX (F94_RING_SEGMENTS * 2)

/* Sources that can register, not counting "off" entry */
#define F94_RING_MAX_SOURCES 8

/* Built-in first entry: "off" */
#define F94_RING_OFF 0

/** @brief Range of a source's levels. */
typedef enum {
    F94_RING_FILL = 0,
    F94_RING_DOUBLE,
} f94_ring_mode_t;

/** @brief Reports a source's current level.
  * @param context The context passed to f94_ring_register.
  * @return A level within the range of the source's mode; higher values are
  *         clamped.
  */
typedef uint8_t (*f94_ring_source_t)(void *context);

/** @brief Registers a source of ring values.
  * @param name Label (6 characters) for source shown in settings.
  * @param source The function reporting the level.
  * @param context Passed to that function; usually the face's own state.
  * @param mode Range of this source's levels. @see f94_ring_mode_t.
  */
void f94_ring_register(const char *name, f94_ring_source_t source, void *context, f94_ring_mode_t mode);

/** @brief Converts a value and its reference into a level.
  * @param value The current value, for example steps walked today.
  * @param max The value that fills the ring.
  * @return A level from 0 to F94_RING_DOUBLE_MAX.
  */
uint8_t f94_ring_scale(uint32_t value, uint32_t max);

/** @brief Number of entries in the registry, including the built-in "off". */
uint8_t f94_ring_count(void);

/** @brief The label of an entry, or NULL if the id is out of range. */
const char *f94_ring_name(uint8_t id);

/** @brief Reads an entry's level, clamped to the range of its mode.
  * @return The level, or 0 for F94_RING_OFF and out of range ids.
  */
uint8_t f94_ring_get(uint8_t id);

#endif                          /* F94_RING_H_ */
