#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "watch_rtc.h"
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

static int8_t alarm_seq[] = {
    BUZZER_NOTE_G6, 40,
    BUZZER_NOTE_C7, 40,
    BUZZER_NOTE_G6, 40,
    BUZZER_NOTE_C7, 80,
    0};

#define NUM_CALCULATION_METHODS (sizeof(calculation_methods) / sizeof(calculation_methods[0]))

static void _calculate_prayer_times(prayer_times_state_t *state, watch_date_time_t date_time);
static time_t _get_prayer_time_by_index(prayer_times_t *prayer_times, uint8_t index);
static prayer_t _get_current_prayer_from_watch_time(prayer_times_t *prayer_times, watch_date_time_t now);
static void _display_prayer_time(prayer_times_state_t *state);
static void _display_set_method(prayer_times_state_t *state);
static void _update_prayer_times_for_midnight(prayer_times_state_t *state, watch_date_time_t now, struct tm *now_tm);
static void _increment_day(watch_date_time_t *dt);
static void _update_current_prayer_index(prayer_times_state_t *state, watch_date_time_t now);

static void _calculate_prayer_times(prayer_times_state_t *state, watch_date_time_t date_time)
{
  struct tm tm_date = {
      .tm_year = date_time.unit.year + WATCH_RTC_REFERENCE_YEAR - 1900,
      .tm_mon = date_time.unit.month - 1,
      .tm_mday = date_time.unit.day,
      .tm_hour = 0,
      .tm_min = 0,
      .tm_sec = 0,
      .tm_isdst = -1};
  time_t date = mktime(&tm_date);

  movement_location_t location = location_load();
  if (location.reg == 0)
  {
    state->location_set = false;
    return;
  }
  state->location_set = true;

  coordinates_t coordinates = {.latitude = location.bit.latitude / 100.0, .longitude = location.bit.longitude / 100.0};
  state->prayer_times = new_prayer_times(&coordinates, date, &state->calculation_params);

  state->times_calculated = true;
  state->last_calculated_day = date_time.unit.day;
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

static prayer_t _get_current_prayer_from_watch_time(prayer_times_t *prayer_times, watch_date_time_t now)
{
  struct tm now_tm = {
      .tm_year = now.unit.year + WATCH_RTC_REFERENCE_YEAR - 1900,
      .tm_mon = now.unit.month - 1,
      .tm_mday = now.unit.day,
      .tm_hour = now.unit.hour,
      .tm_min = now.unit.minute,
      .tm_sec = now.unit.second,
      .tm_isdst = -1};
  time_t now_time = mktime(&now_tm);
  return currentPrayer(prayer_times, now_time);
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
    return;
  }

  uint8_t index = state->current_prayer_index;
  if (index > 6)
  { // Add bounds checking
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

  struct tm *prayer_tm = localtime(&prayer_time);
  uint8_t hour = prayer_tm->tm_hour;
  uint8_t minute = prayer_tm->tm_min;

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

  if (state->alarms_enabled)
    watch_set_indicator(WATCH_INDICATOR_BELL);
  else
    watch_clear_indicator(WATCH_INDICATOR_BELL);

  watch_display_text_with_fallback(WATCH_POSITION_TOP, prayer_names[state->current_prayer_index], prayer_names[state->current_prayer_index]);

  static char time_str[5];
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
  struct tm tm_val = {
      .tm_year = dt->unit.year - 1900,
      .tm_mon = dt->unit.month - 1,
      .tm_mday = dt->unit.day + 1,
      .tm_hour = dt->unit.hour,
      .tm_min = dt->unit.minute,
      .tm_sec = dt->unit.second};
  mktime(&tm_val);
  dt->unit.year = tm_val.tm_year + 1900;
  dt->unit.month = tm_val.tm_mon + 1;
  dt->unit.day = tm_val.tm_mday;
}

static void _update_current_prayer_index(prayer_times_state_t *state, watch_date_time_t now)
{
  if (state->location_set)
  {
    uint8_t new_prayer_index = _get_current_prayer_from_watch_time(&state->prayer_times, now);
    if (state->current_prayer_index != new_prayer_index)
    {
      // A new prayer time has begun.
      state->current_prayer_index = new_prayer_index;

      // If alarms are enabled, play a sound for the new prayer time.
      // We exclude Sunrise (index 1) and Midnight (index 6).
      if (state->alarms_enabled && new_prayer_index != 1 && new_prayer_index != 6)
      {
        watch_buzzer_play_sequence(alarm_seq, NULL);
      }
    }
  }
}

static void _update_prayer_times_for_midnight(prayer_times_state_t *state, watch_date_time_t now, struct tm *now_tm)
{
  time_t now_time = mktime(now_tm);

  // If after today's midnight, use next day's prayers
  if (state->location_set && state->prayer_times.midnight && now_time >= state->prayer_times.midnight)
  {
    // Calculate for next day
    watch_date_time_t next_date_time = now;
    _increment_day(&next_date_time);
    _calculate_prayer_times(state, next_date_time);
    state->manual_override = false;
    _update_current_prayer_index(state, now);
    _display_prayer_time(state);
  }
  else
  {
    if (state->last_calculated_day != now.unit.day)
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
}

void prayer_times_face_setup(uint8_t watch_face_index, void **context_ptr)
{
  (void)watch_face_index;
  if (*context_ptr == NULL)
  {
    *context_ptr = malloc(sizeof(prayer_times_state_t));
    memset(*context_ptr, 0, sizeof(prayer_times_state_t));

    prayer_times_state_t *state = (prayer_times_state_t *)*context_ptr;
    // Default calculation parameters (Muslim World League)
    state->calculation_params = getParameters(MUSLIM_WORLD_LEAGUE);
    state->last_calculated_day = 0;
  }
}

void prayer_times_face_activate(void *context)
{
  prayer_times_state_t *state = (prayer_times_state_t *)context;
  watch_date_time_t date_time = watch_rtc_get_date_time();
  struct tm now_tm = {
      .tm_year = date_time.unit.year + WATCH_RTC_REFERENCE_YEAR - 1900,
      .tm_mon = date_time.unit.month - 1,
      .tm_mday = date_time.unit.day,
      .tm_hour = date_time.unit.hour,
      .tm_min = date_time.unit.minute,
      .tm_sec = date_time.unit.second,
      .tm_isdst = -1};
  _update_prayer_times_for_midnight(state, date_time, &now_tm);
}

bool prayer_times_face_loop(movement_event_t event, void *context)
{
  prayer_times_state_t *state = (prayer_times_state_t *)context;

  switch (event.event_type)
  {
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
      struct tm now_tm = {
          .tm_year = now.unit.year + WATCH_RTC_REFERENCE_YEAR - 1900,
          .tm_mon = now.unit.month - 1,
          .tm_mday = now.unit.day,
          .tm_hour = now.unit.hour,
          .tm_min = now.unit.minute,
          .tm_sec = now.unit.second,
          .tm_isdst = -1};
      _update_prayer_times_for_midnight(state, now, &now_tm);
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
  case EVENT_ALARM_LONG_PRESS:
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
        // Cancel and return to display mode
        state->mode = PRAYER_TIMES_MODE_DISPLAY;
        _display_prayer_time(state);
      }
    }
    break;
  case EVENT_ALARM_BUTTON_UP:
    if (state->mode == PRAYER_TIMES_MODE_DISPLAY)
    {
      if (state->location_set)
      {
        state->manual_override = true;
        state->current_prayer_index = (state->current_prayer_index + 1) % 7;
        _display_prayer_time(state);
      }
    }
    else
    { // PRAYER_TIMES_MODE_SET_METHOD
      state->selected_method_index = (state->selected_method_index + 1) % NUM_CALCULATION_METHODS;
      _display_set_method(state);
    }
    break;
  case EVENT_LIGHT_BUTTON_UP:
    if (state->mode == PRAYER_TIMES_MODE_SET_METHOD)
    {
      watch_clear_display();
      // Save selection, recalculate, and return to display mode
      state->calculation_params = getParameters(calculation_methods[state->selected_method_index]);
      _calculate_prayer_times(state, watch_rtc_get_date_time());
      state->manual_override = false;
      _update_current_prayer_index(state, watch_rtc_get_date_time());
      state->mode = PRAYER_TIMES_MODE_DISPLAY;
      _display_prayer_time(state);
    }
    else
    {
      // Recalculate prayer times and reset to auto mode
      _calculate_prayer_times(state, watch_rtc_get_date_time());
      state->manual_override = false;
      _update_current_prayer_index(state, watch_rtc_get_date_time());
      _display_prayer_time(state);
    }
    break;
  case EVENT_LIGHT_LONG_PRESS:
    // Toggle the alarm state
    state->alarms_enabled = !state->alarms_enabled;
    // Give the user some feedback
    if (state->alarms_enabled)
    {
      watch_buzzer_play_note(BUZZER_NOTE_C7, 100);
    }
    else
    {
      watch_buzzer_play_note(BUZZER_NOTE_C6, 100);
    }
    // Update the display to show/hide the alarm indicator
    _display_prayer_time(state);
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