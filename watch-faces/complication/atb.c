#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "atb.h"

/**
 * The following timegm stuff was copied directly from stack overflow.
 *
 * I renamed the function so we can actually use literally the same timegm
 * on all platforms, instead of subtle differences if we were so lucky that
 * timegm was in fact available.
 *
 * https://stackoverflow.com/a/58037981
 */

// Algorithm: http://howardhinnant.github.io/date_algorithms.html
int days_from_epoch(int y, int m, int d)
{
    y -= m <= 2;
    int era = y / 400;
    int yoe = y - era * 400;                                   // [0, 399]
    int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;  // [0, 365]
    int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;           // [0, 146096]
    return era * 146097 + doe - 719468;
}

// It  does not modify broken-down time
time_t my_timegm(struct tm const* t)
{
    int year = t->tm_year + 1900;
    int month = t->tm_mon;          // 0-11
    if (month > 11)
    {
        year += month / 12;
        month %= 12;
    }
    else if (month < 0)
    {
        int years_diff = (11 - month) / 12;
        year -= years_diff;
        month += 12 * years_diff;
    }
    int days_since_epoch = days_from_epoch(year, month + 1, t->tm_mday);

    return 60 * (60 * (24L * days_since_epoch + t->tm_hour) + t->tm_min) + t->tm_sec;
}

int is_dst_in_cet(const struct tm *time) {
    int year = time->tm_year + 1900;

    // Calculate last Sunday in March (DST starts)
    struct tm dst_start = {0};
    dst_start.tm_year = year - 1900;
    dst_start.tm_mon = 2; // March
    dst_start.tm_mday = 31; // Start with the last day of March
    dst_start.tm_hour = 2; // DST starts at 02:00
    mktime(&dst_start);

    // Move back to the previous Sunday if needed
    while (dst_start.tm_wday != 0) {
        dst_start.tm_mday--;
        mktime(&dst_start); // Normalize the structure
    }

    // Calculate last Sunday in October (DST ends)
    struct tm dst_end = {0};
    dst_end.tm_year = year - 1900;
    dst_end.tm_mon = 9; // October
    dst_end.tm_mday = 31; // Start with the last day of October
    dst_end.tm_hour = 3; // DST ends at 03:00
    mktime(&dst_end);

    // Move back to the previous Sunday if needed
    while (dst_end.tm_wday != 0) {
        dst_end.tm_mday--;
        mktime(&dst_end); // Normalize the structure
    }

    // Check if the given time is within the DST period
    time_t current = mktime((struct tm *)time);
    return (current >= mktime(&dst_start) && current < mktime(&dst_end));
}

int get_cet_offset_without_setenv(time_t utc_time) {
    struct tm *utc_tm = gmtime(&utc_time);
    int offset = 3600; // Standard CET offset is UTC+1

    // Adjust for DST
    if (is_dst_in_cet(utc_tm)) {
        offset += 3600; // Add 1 hour for DST
    }

    return offset;
}

int atb_get_next_departure(int timestamp, char* route, char* stop_id) {
    ResultSet departures = atb_get_next_departures(timestamp, route, stop_id);
    if (sizeof(departures.resultSet) > 0) {
        return departures.resultSet[0];
    }
    return -1; // No next departure found
}

ResultSet atb_get_next_departures(int timestamp, char* route, char* stop_id) {
    ResultSet result;
    // Calculate the number of elements in the schedules array
    int num_schedules = sizeof(schedules) / sizeof(schedules[0]);
    int num_stops = sizeof(stop_offsets) / sizeof(stop_offsets[0]);

    int result_count = 0;

    // First find the stop id, and its offset on the route.
    int stop_offset_in_minutes = 0;
    for (int i = 0; i < num_stops; i++) {
        if (strcmp(stop_offsets[i].stop_id, stop_id) == 0 && strcmp(stop_offsets[i].route, route) == 0) {
            stop_offset_in_minutes = stop_offsets[i].offset;
            break;
        }
    }

    // Convert the integer to time_t
    time_t ttimestamp = (time_t) timestamp;
    ttimestamp += get_cet_offset_without_setenv(ttimestamp);
    struct tm *time_info = gmtime(&ttimestamp);
    if (time_info == NULL) {
        fprintf(stderr, "Error: Could not convert timestamp.\n");
        return result;
    }

    int day_of_week = time_info->tm_wday;

    for (int i = 0; i < num_schedules; i++) {
        // Compare the route (string comparison).
        if (strcmp(schedules[i].route, route) == 0) {
            if (day_of_week == ATB_SATURDAY || day_of_week == ATB_SUNDAY) {
                if (schedules[i].day_id != day_of_week) {
                    continue;
                }
            }
            else {
                if (schedules[i].day_id != ATB_WEEKDAY) {
                    continue;
                }
            }
            for (int j = 0; j < schedules[i].departureTimes.count; j++) {
                // Create a timestamp from the string, and make it so the timestamp represents the same day.
                char *hour, *minute;
                char *departure_time = schedules[i].departureTimes.departure_times[j];
                char buffer[6]; // Make sure the buffer is large enough for your strings
                strncpy(buffer, departure_time, sizeof(buffer) - 1);
                buffer[sizeof(buffer) - 1] = '\0'; // Ensure null-termination
                // Split the buffer using strtok
                hour = strtok(buffer, ":");
                minute = strtok(NULL, ":");
                // Convert hour and minute to integers
                int new_hour = atoi(hour);
                int new_minute = atoi(minute);

                // Update time_info with new hour and minute
                time_info->tm_hour = new_hour;
                time_info->tm_min = new_minute;
                time_info->tm_sec = 0; // Set seconds to 0

                // Convert back to Unix timestamp
                time_t new_timestamp = my_timegm(time_info);
                new_timestamp += stop_offset_in_minutes * 60;
                // Check if the departure time is after the given timestamp
                if (new_timestamp >= ttimestamp) {
                    result.resultSet[result_count] = new_timestamp -= get_cet_offset_without_setenv(new_timestamp);
                    result_count++;
                }
                if (result_count == MAX_DEPARTURES) {
                    break;
                }

            }
            break; // Exit the outer loop once the route is found
        }
    }

    return result;
}
