/*
 * This file is copied from Adhan-C/src/calendrical_helper.c
 * It is modified to avoid a conflict with `utz` library's `is_leap_year` function.
*/

#include "calendrical_helper.h"
#include <math.h>
#include "utz.h"

double _julian_day(int year, int month, int day, double hours) {
  /* Equation from Astronomical Algorithms page 60 */

  // NOTE: Integer conversion is done intentionally for the purpose of decimal
  // truncation
  int Y = month > 2 ? year : year - 1;
  int M = month > 2 ? month : month + 12;
  double D = day + (hours / 24);

  int A = Y / 100;
  int B = 2 - A + (A / 4);

  int i0 = (int)(365.25 * (Y + 4716));
  int i1 = (int)(30.6001 * (M + 1));
  return i0 + i1 + D + B - 1524.5;
}

double julian_day(int year, int month, int day) {
  return _julian_day(year, month, day, 0.0);
}

double julian_day_from_time_t(const time_t when) {
  struct tm *date = gmtime(&when);
  return _julian_day(date->tm_year + 1900, date->tm_mon + 1, date->tm_mday,
                     date->tm_hour + date->tm_min / 60.0 +
                         date->tm_sec / 3600.0);
}

double julian_century(double JD) {
  /* Equation from Astronomical Algorithms page 163 */
  return (JD - 2451545.0) / 36525;
}

bool adhan_is_leap_year(int year) {
  return is_leap_year(year);
}

time_t add_seconds(const time_t when, int amount) { return when + amount; }

time_t add_minutes(const time_t when, int amount) {
  return add_seconds(when, amount * 60);
}

time_t add_hours(const time_t when, int amount) {
  return add_minutes(when, amount * 60);
}

time_t add_days(const time_t when, int amount) {
  return add_hours(when, amount * 24);
}

time_t date_from_time(const time_t time) {
  struct tm *tm_date = localtime(&time);
  tm_date->tm_hour = 0;
  tm_date->tm_min = 0;
  tm_date->tm_sec = 0;
  return mktime(tm_date);
}
