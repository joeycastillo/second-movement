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

#include "f94_ring.h"

/* F94_RING_OFF is built in and always reads zero. Sources register from 1 up. */
#define F94_RING_ENTRIES (F94_RING_MAX_SOURCES + 1)

typedef struct {
    const char *name;
    f94_ring_source_t source;   /* NULL for the built-in "off" entry */
    void *context;
    f94_ring_mode_t mode;
} f94_ring_entry_t;

static f94_ring_entry_t _entries[F94_RING_ENTRIES] = {
    [F94_RING_OFF] = { "OFF   ", NULL, NULL, F94_RING_FILL },
};

static uint8_t _count = 1;

void f94_ring_register(const char *name, f94_ring_source_t source, void *context, f94_ring_mode_t mode)
{
    if (source == NULL)
        return;

    /* Faces register from setup(), which movement calls again after every
     * wake from deep sleep. A known source is thus updated in place, so that
     * the table does not fill up with one copy per wake. */
    uint8_t id;
    for (id = 1; id < _count; id++) {
        if (_entries[id].source == source && _entries[id].context == context)
            break;
    }

    if (id >= F94_RING_ENTRIES)
        return;

    _entries[id].name = name;
    _entries[id].source = source;
    _entries[id].context = context;
    _entries[id].mode = mode;

    if (id == _count)
        _count++;
}

uint8_t f94_ring_scale(uint32_t value, uint32_t max)
{
    if (max == 0)
        return 0;

    /* Clamp first: value * F94_RING_SEGMENTS overflows uint32_t for large
     * values, and nothing above twice max is representable. */
    if (value > max * 2)
        return F94_RING_DOUBLE_MAX;

    return (uint8_t) (value * F94_RING_SEGMENTS / max);
}

uint8_t f94_ring_count(void)
{
    return _count;
}

const char *f94_ring_name(uint8_t id)
{
    return id < _count ? _entries[id].name : NULL;
}

uint8_t f94_ring_get(uint8_t id)
{
    if (id >= _count || _entries[id].source == NULL)
        return 0;

    uint8_t level = _entries[id].source(_entries[id].context);

    /* Clamping per mode keeps the mode out of the clock face: a filling source
     * never returns a level in the second lap's range. */
    uint8_t max = _entries[id].mode == F94_RING_DOUBLE ? F94_RING_DOUBLE_MAX : F94_RING_FILL_MAX;

    return level > max ? max : level;
}
