#include "location.h"
#include "watch_common_display.h"
#include <math.h>
#include <stdlib.h>
#include "filesystem.h"

void _update_location_register(location_state_t *state) {
    if (state->location_changed) {
        movement_location_t movement_location;
        int16_t lat = _latlon_from_struct(state->working_latitude);
        int16_t lon = _latlon_from_struct(state->working_longitude);
        movement_location.bit.latitude = lat;
        movement_location.bit.longitude = lon;
        persist_location_to_filesystem(movement_location);
        state->location_changed = false;
    }
}

int16_t _latlon_from_struct(lat_lon_settings_t val) {
    int16_t retval = (val.sign ? -1 : 1) *
                        (
                            val.hundreds * 10000 +
                            val.tens * 1000 +
                            val.ones * 100 +
                            val.tenths * 10 +
                            val.hundredths
                        );
    return retval;
}

void _update_location_settings_display(movement_event_t event, location_state_t *state) {
    char buf[12];

    watch_clear_display();

    switch (state->page) {
        case 0:
            return;
        case 1:
            // Latitude
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "LAT", "LA");
            if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
                watch_set_decimal_if_available();
                watch_display_character('0' + state->working_latitude.tens, 4);
                watch_display_character('0' + state->working_latitude.ones, 5);
                watch_display_character('0' + state->working_latitude.tenths, 6);
                watch_display_character('0' + state->working_latitude.hundredths, 7);
                watch_display_character('#', 8);
                if (state->working_latitude.sign) watch_display_character('S', 9);
                else watch_display_character('N', 9);

                if (event.subsecond % 2) {
                    watch_display_character(' ', 4 + state->active_digit);
                    // for degrees N or S, also flash the last character
                    if (state->active_digit == 4) watch_display_character(' ', 9);
                }
            } else {
                sprintf(buf, "%c %04d", state->working_latitude.sign ? '-' : '+', abs(_latlon_from_struct(state->working_latitude)));
                if (event.subsecond % 2) buf[state->active_digit] = ' ';
                watch_display_text(WATCH_POSITION_BOTTOM, buf);
            }
            break;
        case 2:
            // Longitude
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "LON", "LO");
            if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
                watch_set_decimal_if_available();
                // Handle leading 1 for longitudes >99
                if (state->working_longitude.hundreds == 1) watch_set_pixel(0, 22);
                watch_display_character('0' + state->working_longitude.tens, 4);
                watch_display_character('0' + state->working_longitude.ones, 5);
                watch_display_character('0' + state->working_longitude.tenths, 6);
                watch_display_character('0' + state->working_longitude.hundredths, 7);
                watch_display_character('#', 8);
                if (state->working_longitude.sign) watch_display_character('W', 9);
                else watch_display_character('E', 9);
                if (event.subsecond % 2) {
                    watch_display_character(' ', 4 + state->active_digit);
                    // for tens place, also flash leading 1 if present
                    if (state->active_digit == 0) watch_clear_pixel(0, 22);
                    // for degrees E or W, also flash the last character
                    if (state->active_digit == 4) watch_display_character(' ', 9);
                }
            } else {
                sprintf(buf, "%c%05d", state->working_longitude.sign ? '-' : '+', abs(_latlon_from_struct(state->working_longitude)));
                if (event.subsecond % 2) buf[state->active_digit] = ' ';
                watch_display_text(WATCH_POSITION_BOTTOM, buf);
            }
            break;
    }
}