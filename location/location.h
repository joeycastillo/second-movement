
#ifndef LOCATION_H_
#define LOCATION_H_

#include "movement.h"

typedef struct {
    uint8_t sign: 1;    // 0-1
    uint8_t hundreds: 5;    // 0-1, ignored for latitude
    uint8_t tens: 5;        // 0-18 (w:raps at 10 on clasic LCD, 18 on custom LCD)
    uint8_t ones: 4;        // 0-9 (must wrap at 10)
    uint8_t tenths: 4;      // 0-9 (must wrap at 10)
    uint8_t hundredths: 4;  // 0-9 (must wrap at 10)
} lat_lon_settings_t;

typedef struct {
    uint8_t page;
    uint8_t active_digit;
    bool location_changed;
    lat_lon_settings_t working_latitude;
    lat_lon_settings_t working_longitude;
} location_state_t;


typedef struct {
    char name[3];
    int16_t latitude;
    int16_t longitude;
} long_lat_presets_t;

static const long_lat_presets_t longLatPresets[] =
{
    { .name = "  "},  // Default, the long and lat get replaced by what's set in the watch
//    { .name = "Ny", .latitude = 4072, .longitude = -7401 },  // New York City, NY
//    { .name = "FL", .latitude = 2906, .longitude = -8205 },  // Belleview, Florida
//    { .name = "LA", .latitude = 3405, .longitude = -11824 },  // Los Angeles, CA
//    { .name = "dE", .latitude = 4221, .longitude = -8305 },  // Detroit, MI
};

int16_t _latlon_from_struct(lat_lon_settings_t val);
void _update_location_settings_display(movement_event_t event, location_state_t *state);
void _update_location_register(location_state_t *state);

#endif // LOCATION_H_
