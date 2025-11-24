/*
 * MIT License
 *
 * Copyright (c) 2022 Joey Castillo
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

#ifndef SUNRISE_SUNSET_FACE_H_
#define SUNRISE_SUNSET_FACE_H_

#include "movement.h"

/*
 * SUNRISE & SUNSET FACE
 *
 * The Sunrise/Sunset face is designed to display the next sunrise or sunset
 * for a given location. To set the location, navigate to the dedicated
 * Location Setting face.
 *
 * Refer to the wiki for usage instructions:
 *  https://www.sensorwatch.net/docs/watchfaces/complication/#sunrisesunset
 */

typedef struct
{
  uint8_t rise_index;
  watch_date_time_t rise_set_expires;
  uint8_t longLatToUse;
} sunrise_sunset_state_t;

void sunrise_sunset_face_setup(uint8_t watch_face_index, void **context_ptr);
void sunrise_sunset_face_activate(void *context);
bool sunrise_sunset_face_loop(movement_event_t event, void *context);
void sunrise_sunset_face_resign(void *context);

#define sunrise_sunset_face ((const watch_face_t){ \
    sunrise_sunset_face_setup,                     \
    sunrise_sunset_face_activate,                  \
    sunrise_sunset_face_loop,                      \
    sunrise_sunset_face_resign,                    \
    NULL,                                          \
})

typedef struct
{
  char name[3];
  int16_t latitude;
  int16_t longitude;
} long_lat_presets_t;

static const long_lat_presets_t longLatPresets[] =
    {
        {.name = "  "}, // Default, the long and lat get replaced by what's set in the watch
        //    { .name = "Ny", .latitude = 4072, .longitude = -7401 },  // New York City, NY
        //    { .name = "LA", .latitude = 3405, .longitude = -11824 },  // Los Angeles, CA
        //    { .name = "dE", .latitude = 4221, .longitude = -8305 },  // Detroit, MI
};

#endif // SUNRISE_SUNSET_FACE_H_
