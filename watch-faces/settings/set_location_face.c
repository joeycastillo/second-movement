#include <stdlib.h>
#include <string.h>
#include "set_location_face.h"
#include "watch.h"
#include "location.h"

static void _set_location_display(movement_event_t event, set_location_state_t *state);
static void _blink_digit_at_position(char *buf, set_location_state_t *state);
static void _set_location_advance_digit(set_location_state_t *state);
static void _set_location_next_digit_or_mode(set_location_state_t *state);
static bool _is_valid_coordinate(int16_t lat, int16_t lon);

static void _set_location_display(movement_event_t event, set_location_state_t *state)
{
  char buf[12];
  char display_buf[12];
  watch_clear_display();

  watch_display_text(WATCH_POSITION_TOP_RIGHT, state->sign_is_negative ? "--" : "-+");

  switch (state->mode)
  {
  case LOCATION_MODE_SET_LAT:
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "LAT", "Lat");
    sprintf(buf, "%02d%02d", abs(state->working_latitude) / 100, abs(state->working_latitude) % 100);
    watch_set_colon();
    break;
  case LOCATION_MODE_SET_LON:
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "LON", "Lon");
    sprintf(buf, "%03d.%02d", abs(state->working_longitude) / 100, abs(state->working_longitude) % 100);
    watch_clear_colon();
    break;
  }

  // Copy buf to display_buf for manipulation
  strcpy(display_buf, buf);

  // Handle blinking
  if (event.subsecond % 2)
  {
    if (state->active_digit == 0)
    {
      // Blink the sign
      watch_display_text(WATCH_POSITION_TOP_RIGHT, "   ");
    }
    else
    {
      // Blink the appropriate digit
      _blink_digit_at_position(display_buf, state);
    }
  }

  watch_display_text(WATCH_POSITION_BOTTOM, display_buf);
}

static void _blink_digit_at_position(char *buf, set_location_state_t *state)
{
  uint8_t pos = state->active_digit - 1;

  if (state->mode == LOCATION_MODE_SET_LON && pos >= 3)
  {
    pos++; // Skip decimal point
  }

  if (pos < strlen(buf))
  {
    buf[pos] = ' ';
  }
}

static void _set_location_advance_digit(set_location_state_t *state)
{
  state->location_changed = true;
  int16_t *val_ptr;
  int16_t max_val;

  if (state->mode == LOCATION_MODE_SET_LAT)
  {
    val_ptr = &state->working_latitude;
    max_val = 9000; // 90.00 degrees
  }
  else
  {
    val_ptr = &state->working_longitude;
    max_val = 18000; // 180.00 degrees
  }

  if (state->active_digit == 0)
  {
    // Toggle the sign flag.
    state->sign_is_negative = !state->sign_is_negative;
    // If the value is not zero, make its sign match the flag.
    if (*val_ptr != 0)
    {
      *val_ptr = state->sign_is_negative ? -abs(*val_ptr) : abs(*val_ptr);
    }
  }
  else
  {
    int16_t current_val = *val_ptr;
    int16_t abs_val = abs(current_val);

    // Calculate the divisor based on digit position and mode
    int16_t divisor = 1;
    int power;

    if (state->mode == LOCATION_MODE_SET_LAT)
    {
      // Latitude: 4 digits (00.00)
      power = 4 - state->active_digit;
    }
    else
    {
      // Longitude: 5 digits (000.00)
      power = 5 - state->active_digit;
    }

    for (int i = 0; i < power; i++)
    {
      divisor *= 10;
    }

    int old_digit = (abs_val / divisor) % 10;
    int new_digit = (old_digit + 1) % 10;

    // Reconstruct the number
    abs_val = abs_val - (old_digit * divisor) + (new_digit * divisor);

    // Wrap around when exceeding maximum
    if (abs_val > max_val)
    {
      // If the new value exceeds the maximum valid value (e.g. > 90.00),
      // wrap around to 0.
      abs_val = 0;
    }

    *val_ptr = state->sign_is_negative ? -abs_val : abs_val;
  }
}

static void _set_location_next_digit_or_mode(set_location_state_t *state)
{
  state->active_digit++;
  uint8_t max_digits = (state->mode == LOCATION_MODE_SET_LAT) ? 5 : 6; // lat: sign+4, lon: sign+5
  if (state->active_digit >= max_digits)
  {
    state->active_digit = 0;
    if (state->mode == LOCATION_MODE_SET_LAT)
    {
      state->mode = LOCATION_MODE_SET_LON;
      state->sign_is_negative = (state->working_longitude < 0);
    }
    else
    {
      // Finished setting longitude, so we exit and save.
      movement_move_to_next_face();
    }
  }
}

static bool _is_valid_coordinate(int16_t lat, int16_t lon)
{
  return (abs(lat) <= 9000) && (abs(lon) <= 18000);
}

void set_location_face_setup(uint8_t watch_face_index, void **context_ptr)
{
  (void)watch_face_index;
  if (*context_ptr == NULL)
  {
    *context_ptr = malloc(sizeof(set_location_state_t));
  }
  // This function is only called once when the watch face is first selected.
  // We don't set state here, we do it in activate.
}

void set_location_face_activate(void *context)
{
  set_location_state_t *state = (set_location_state_t *)context;
  memset(state, 0, sizeof(set_location_state_t));

  movement_location_t current_location = location_load();
  state->working_latitude = current_location.bit.latitude;
  state->working_longitude = current_location.bit.longitude;

  state->mode = LOCATION_MODE_SET_LAT;
  state->sign_is_negative = (state->working_latitude < 0);

  movement_request_tick_frequency(4);
}

bool set_location_face_loop(movement_event_t event, void *context)
{
  set_location_state_t *state = (set_location_state_t *)context;

  switch (event.event_type)
  {
  case EVENT_TICK:
    _set_location_display(event, state);
    break;
  case EVENT_ALARM_BUTTON_UP:
    _set_location_advance_digit(state);
    _set_location_display(event, state);
    break;
  case EVENT_LIGHT_BUTTON_UP:
    _set_location_next_digit_or_mode(state);
    _set_location_display(event, state);
    break;
  case EVENT_ALARM_LONG_PRESS:
    // Cancel and exit without saving.
    state->location_changed = false;
    movement_move_to_next_face();
    break;
  case EVENT_TIMEOUT:
    // On timeout, exit without saving.
    state->location_changed = false;
    movement_move_to_next_face();
    break;
  default:
    return movement_default_loop_handler(event);
  }
  return true;
}

void set_location_face_resign(void *context)
{
  set_location_state_t *state = (set_location_state_t *)context;
  if (state->location_changed && _is_valid_coordinate(state->working_latitude, state->working_longitude))
  {
    movement_location_t new_location;
    new_location.bit.latitude = state->working_latitude;
    new_location.bit.longitude = state->working_longitude;
    location_persist(new_location);
  }
  movement_request_tick_frequency(1);
}
