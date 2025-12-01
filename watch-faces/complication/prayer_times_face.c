#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "watch.h"
#include "watch_utility.h"
#include "prayer_times_face.h"
#include "location.h"

#if __EMSCRIPTEN__
#include <emscripten.h>
#endif

static const char prayer_names[][6] = {
    "FAJR ",
    "SUNRI",
    "DHUHR",
    "ASR  ",
    "MAGHR",
    "ISHA ",
    "MIDNI",
};

static const char calculation_method_names[][6] = {
    "MWL  ",
    "EGYPT",
    "KARAC",
    "UMM A",
    "GULF ",
    "MOON ",
    "ISNA ",
    "KUWAI",
    "QATAR"};

static const calculation_method calculation_methods[] = {
    MUSLIM_WORLD_LEAGUE,
    EGYPTIAN,
    KARACHI,
    UMM_AL_QURA,
    GULF,
    MOON_SIGHTING_COMMITTEE,
    NORTH_AMERICA,
    KUWAIT,
    QATAR};

#define NUM_CALCULATION_METHODS (sizeof(calculation_methods) / sizeof(calculation_methods[0]))

static void _calculate_prayer_times(prayer_times_state_t *state, watch_date_time_t date_time);
static time_t _get_prayer_time_by_index(prayer_times_t *prayer_times, uint8_t index);
static uint8_t _get_current_prayer_from_watch_time(prayer_times_t *prayer_times, watch_date_time_t now);
static void _display_prayer_time(prayer_times_state_t *state);
static void _display_set_method(prayer_times_state_t *state);
static void _update_prayer_times(prayer_times_state_t *state, watch_date_time_t now);
static void _increment_day(watch_date_time_t *dt);
static void _update_current_prayer_index(prayer_times_state_t *state, watch_date_time_t now);

static void _calculate_prayer_times(prayer_times_state_t *state, watch_date_time_t date_time)
{
  // Get the local date (the date in the user's timezone).
  // The watch RTC stores UTC time, but we need to know what the LOCAL date is
  // to calculate prayers for the correct day.
  watch_date_time_t local_date_time = watch_utility_date_time_convert_zone(
      date_time, 0, movement_get_current_timezone_offset());

  // Build a UTC midnight timestamp for the LOCAL date.
  // The Adhan library expects a UTC timestamp representing midnight in UTC
  // for the date we want prayers for (in the user's local timezone).
  watch_date_time_t utc_midnight;
  utc_midnight.reg = 0;
  utc_midnight.unit.year = local_date_time.unit.year;
  utc_midnight.unit.month = local_date_time.unit.month;
  utc_midnight.unit.day = local_date_time.unit.day;
  utc_midnight.unit.hour = 0;
  utc_midnight.unit.minute = 0;
  utc_midnight.unit.second = 0;

  // Pass 0 as the offset to get the UTC timestamp for this date at 00:00 UTC
  time_t date = (time_t)watch_utility_date_time_to_unix_time(utc_midnight, 0);

  movement_location_t location = location_load();
  if (location.reg == 0)
  {
    state->location_set = false;
    return;
  }
  state->location_set = true;

  /* movement_location_t stores latitude and longitude as signed 16-bit
     values in hundredths of a degree (int16_t). Use the signed fields
     directly to avoid any unsigned promotion or sign-extension issues.
  */
  int16_t lat_hundredths = (int16_t)location.bit.latitude;
  int16_t lon_hundredths = (int16_t)location.bit.longitude;

  coordinates_t coordinates = {
      .latitude = lat_hundredths / 100.0,
      .longitude = lon_hundredths / 100.0};

  state->prayer_times = new_prayer_times(&coordinates, date, &state->calculation_params);

  // Sanity check: If the calculation returns 0 for Fajr, it has failed.
  if (state->prayer_times.fajr == 0)
  {
    state->times_calculated = false;
    return;
  }

  state->times_calculated = true;
  state->last_calculated_day = date_time.unit.day;
  state->first_run = false;
}

static time_t _get_prayer_time_by_index(prayer_times_t *prayer_times, uint8_t index)
{
  time_t *times[] = {
      &prayer_times->fajr,
      &prayer_times->sunrise,
      &prayer_times->dhuhr,
      &prayer_times->asr,
      &prayer_times->maghrib,
      &prayer_times->isha,
      &prayer_times->midnight};
  if (index > 6)
    return 0;
  return *times[index];
}

static uint8_t _get_current_prayer_from_watch_time(prayer_times_t *prayer_times, watch_date_time_t now)
{
  // Convert the watch's local date/time into a UTC unix timestamp using Movement's
  // timezone offset. This avoids reliance on the C library's localtime/mktime
  // behavior which can differ between emulator and hardware.
  time_t now_time = (time_t)watch_utility_date_time_to_unix_time(now, movement_get_current_timezone_offset());
  prayer_t prayer = currentPrayer(prayer_times, now_time);
  // Default to Fajr
  if (prayer == NONE)
    prayer = FAJR;
  // Convert prayer_t to index
  return prayer - 1;
}

static void _display_prayer_time(prayer_times_state_t *state)
{
  if (!state->location_set)
  {
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "PRAY", "Pray");
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "NO LOC", "No Loc");
    return;
  }

  if (!state->times_calculated)
  {
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "CALC", "Calc");
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "FAIL", "Fail");
    return;
  }

  uint8_t index = state->current_prayer_index;
  if (index > 6)
  {
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "ERROR", "Err");
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "INDEX", "Idx");
    return;
  }

  time_t prayer_time = _get_prayer_time_by_index(&state->prayer_times, index);
  if (prayer_time == 0)
  {
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "ERROR", "Err");
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "TIME", "Time");
    return;
  }

  // Convert the UTC timestamp returned by the Adhan library into a local
  // watch_date_time_t using Movement's timezone offset so display is consistent
  // across emulator and hardware.
  watch_date_time_t prayer_dt = watch_utility_date_time_from_unix_time((uint32_t)prayer_time, movement_get_current_timezone_offset());
  uint8_t hour = prayer_dt.unit.hour;
  uint8_t minute = prayer_dt.unit.minute;

  if (movement_clock_mode_24h())
    watch_set_indicator(WATCH_INDICATOR_24H);
  else
  {
    if (hour >= 12)
      watch_set_indicator(WATCH_INDICATOR_PM);
    else
      watch_clear_indicator(WATCH_INDICATOR_PM);
    hour = hour % 12 ? hour % 12 : 12;
  }

  watch_display_text_with_fallback(WATCH_POSITION_TOP, prayer_names[state->current_prayer_index], prayer_names[state->current_prayer_index]);

  static char time_str[7];
  sprintf(time_str, "%2d%02d", hour, minute);
  watch_display_text(WATCH_POSITION_BOTTOM, time_str);
  watch_set_colon();
}

static void _display_set_method(prayer_times_state_t *state)
{
  watch_display_text_with_fallback(WATCH_POSITION_TOP, calculation_method_names[state->selected_method_index], calculation_method_names[state->selected_method_index]);
  watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "METHOD", "Method");
}

static void _increment_day(watch_date_time_t *dt)
{
  // Increment the watch_date_time_t by one day using Movement's timezone-aware
  // unix-time conversion to avoid libc timezone/DST differences.
  time_t t = (time_t)watch_utility_date_time_to_unix_time(*dt, movement_get_current_timezone_offset());
  t += 86400; // Add seconds in one day
  watch_date_time_t next = watch_utility_date_time_from_unix_time((uint32_t)t, movement_get_current_timezone_offset());
  dt->reg = next.reg;
}

static void _update_current_prayer_index(prayer_times_state_t *state, watch_date_time_t now)
{
  uint8_t new_prayer_index = _get_current_prayer_from_watch_time(&state->prayer_times, now);
  if (state->current_prayer_index != new_prayer_index)
  {
    state->current_prayer_index = new_prayer_index;

    // If alarms are enabled, play a sound for the new prayer time.
    // We exclude Sunrise (index 1) and Midnight (index 6).
    // We also prevent this from running on the very first calculation.
    if (state->alarms_enabled && !state->first_run && new_prayer_index != 1 && new_prayer_index != 6)
    {
      movement_play_alarm();
    }
  }
}

static void _update_prayer_times(prayer_times_state_t *state, watch_date_time_t now)
{
  // Convert current time to UTC timestamp for comparison with prayer times
  time_t now_time = (time_t)watch_utility_date_time_to_unix_time(now, movement_get_current_timezone_offset());

  // Recalculate prayer times if we've passed Islamic midnight
  // Islamic midnight marks the end of the prayer day
  if (state->times_calculated && state->prayer_times.midnight > 0 && now_time >= state->prayer_times.midnight)
  {
    _calculate_prayer_times(state, now);
    state->manual_override = false;
  }
  // Also recalculate if the date changed (fallback for when times aren't calculated yet)
  else if (state->last_calculated_day != now.unit.day)
  {
    _calculate_prayer_times(state, now);
    state->manual_override = false;
  }

  if (!state->manual_override)
  {
    _update_current_prayer_index(state, now);
  }
  _display_prayer_time(state);
}

void prayer_times_face_setup(uint8_t watch_face_index, void **context_ptr)
{
  (void)watch_face_index;
  if (*context_ptr == NULL)
  {
    *context_ptr = malloc(sizeof(prayer_times_state_t));
    memset(*context_ptr, 0, sizeof(prayer_times_state_t));

    prayer_times_state_t *state = (prayer_times_state_t *)*context_ptr;
    state->calculation_params = getParameters(MUSLIM_WORLD_LEAGUE);
    state->last_calculated_day = 0;
    state->alarms_enabled = false;
    state->first_run = true;
  }
}

void prayer_times_face_activate(void *context)
{
  prayer_times_state_t *state = (prayer_times_state_t *)context;
  watch_date_time_t date_time = watch_rtc_get_date_time();
  _update_prayer_times(state, date_time);

  if (state->alarms_enabled)
    watch_set_indicator(WATCH_INDICATOR_BELL);
  else
    watch_clear_indicator(WATCH_INDICATOR_BELL);
}

bool prayer_times_face_loop(movement_event_t event, void *context)
{
  prayer_times_state_t *state = (prayer_times_state_t *)context;

  switch (event.event_type)
  {
  case EVENT_BACKGROUND_TASK:
    if (state->alarms_enabled) {
      movement_play_alarm();
    }
    break;
  case EVENT_LOW_ENERGY_UPDATE:
  case EVENT_TICK:
  {
    if (event.event_type == EVENT_LOW_ENERGY_UPDATE && !watch_sleep_animation_is_running())
      watch_start_sleep_animation(1000);

#if __EMSCRIPTEN__
    int16_t browser_lat = EM_ASM_INT({
      return lat;
    });
    int16_t browser_lon = EM_ASM_INT({
      return lon;
    });
    if ((watch_get_backup_data(1) == 0) && (browser_lat || browser_lon))
    {
      movement_location_t browser_loc;
      browser_loc.bit.latitude = browser_lat;
      browser_loc.bit.longitude = browser_lon;
      watch_store_backup_data(browser_loc.reg, 1);
    }
#endif

    // Only update the display if we are in the main display mode.
    // This prevents the tick event from overwriting the settings screen.
    if (state->mode == PRAYER_TIMES_MODE_DISPLAY)
    {
      watch_date_time_t now = watch_rtc_get_date_time();
      _update_prayer_times(state, now);
    }
    break;
  }
  case EVENT_TIMEOUT:
    if (state->mode == PRAYER_TIMES_MODE_SET_METHOD)
    {
      state->mode = PRAYER_TIMES_MODE_DISPLAY;
      _display_prayer_time(state);
    }
    else
    {
      movement_move_to_face(0);
    }
    return true;
  case EVENT_MODE_LONG_PRESS:
    if (!state->location_set)
    {
      movement_move_to_face(SET_LOCATION_FACE_INDEX);
    }
    else
    {
      if (state->mode == PRAYER_TIMES_MODE_DISPLAY)
      {
        state->mode = PRAYER_TIMES_MODE_SET_METHOD;
        // Find current method to start selection there
        for (uint8_t i = 0; i < NUM_CALCULATION_METHODS; i++)
        {
          if (state->calculation_params.method == calculation_methods[i])
          {
            state->selected_method_index = i;
            break;
          }
        }
        _display_set_method(state);
      }
      else
      {
        state->mode = PRAYER_TIMES_MODE_DISPLAY;
        _display_prayer_time(state);
      }
    }
    break;
  case EVENT_ALARM_BUTTON_UP:
    if (!state->location_set)
      break;
    if (state->mode == PRAYER_TIMES_MODE_DISPLAY)
    {
      state->manual_override = true;
      state->current_prayer_index = (state->current_prayer_index + 1) % 7;
      _display_prayer_time(state);
    }
    else
    {
      state->selected_method_index = (state->selected_method_index + 1) % NUM_CALCULATION_METHODS;
      _display_set_method(state);
    }
    break;
  case EVENT_LIGHT_BUTTON_UP:
    if (!state->location_set)
      break;
    if (state->mode == PRAYER_TIMES_MODE_SET_METHOD)
    {
      watch_clear_display();
      // Save selection, recalculate, and return to display mode
      state->calculation_params = getParameters(calculation_methods[state->selected_method_index]);
      state->mode = PRAYER_TIMES_MODE_DISPLAY;
    }
    _calculate_prayer_times(state, watch_rtc_get_date_time());
    state->manual_override = false;
    _update_current_prayer_index(state, watch_rtc_get_date_time());
    _display_prayer_time(state);
    break;
  case EVENT_ALARM_LONG_PRESS:
    state->alarms_enabled = !state->alarms_enabled;
    if (state->alarms_enabled)
      watch_set_indicator(WATCH_INDICATOR_BELL);
    else
      watch_clear_indicator(WATCH_INDICATOR_BELL);
    break;
  default:
    movement_default_loop_handler(event);
    break;
  }

  return true;
}

void prayer_times_face_resign(void *context)
{
  (void)context;
}

movement_watch_face_advisory_t prayer_times_face_advise(void *context)
{
  movement_watch_face_advisory_t retval = {0};
  prayer_times_state_t *state = (prayer_times_state_t *)context;

  if (!state) return retval;

  if (state->alarms_enabled && state->times_calculated)
  {
    watch_date_time_t now = movement_get_local_date_time();
    for (uint8_t i = 0; i < 7; i++)
    {
      // Skip sunrise (index 1) and midnight (index 6) - no alarms for those
      if (i == 1 || i == 6) continue;

      time_t pt = _get_prayer_time_by_index(&state->prayer_times, i);
      if (!pt) continue;

      // Convert UTC prayer time to local time using Movement's timezone offset
      watch_date_time_t prayer_dt = watch_utility_date_time_from_unix_time((uint32_t)pt, movement_get_current_timezone_offset());

      // Check if we're at the prayer time (matching hour and minute)
      if (prayer_dt.unit.hour == now.unit.hour && prayer_dt.unit.minute == now.unit.minute)
      {
        retval.wants_background_task = true;
        break;
      }
    }
  }

  return retval;
}