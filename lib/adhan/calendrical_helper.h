/*
 * This file is copied from Adhan-C/src/calendrical_helper.c
 * It is modified to avoid a conflict with `utz` library's `is_leap_year` function.
*/

#ifndef ADHAN_CALENDRICAL_HELPER_H
#define ADHAN_CALENDRICAL_HELPER_H

#include <stdbool.h>
#include <time.h>

double _julian_day(int year, int month, int day, double hours);
double julian_day(int year, int month, int day);
double julian_day_from_time_t(const time_t when);
double julian_century(double JD);

// CalendarUtil
bool adhan_is_leap_year(int year);
time_t add_seconds(const time_t when, int amount);
time_t add_minutes(const time_t when, int amount);
time_t add_hours(const time_t when, int amount);
time_t add_days(const time_t when, int amount);
time_t date_from_time(const time_t time);

#endif // ADHAN_CALENDRICAL_HELPER_H
