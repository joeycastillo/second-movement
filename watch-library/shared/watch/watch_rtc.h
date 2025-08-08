/*
 * MIT License
 *
 * Copyright (c) 2020 Joey Castillo
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

#pragma once

////< @file watch_rtc.h

#include "watch.h"
#include "rtc32.h"

/** @addtogroup rtc Real-Time Clock
  * @brief This section covers functions related to the SAM L22's real-time clock peripheral, including
  *        date, time and alarm functions.
  * @details The real-time clock is the only peripheral that main.c enables for you. It is the cornerstone
  *          of low power operation on the watch, and it is required for several key functions that we
  *          assume will be available, namely waking on a press of the ALARM button. It is also required
  *          for the operation of the 1 Hz tick interrupt, which we use to wake from STANDBY mode.
  */
/// @{

extern watch_cb_t btn_alarm_callback;
extern watch_cb_t a2_callback;
extern watch_cb_t a4_callback;
extern watch_cb_t comp_callback;

#define WATCH_RTC_REFERENCE_YEAR (2020)

#define watch_date_time_t rtc_date_time_t
typedef rtc_counter_t watch_counter_t;
typedef uint32_t unix_timestamp_t;

/** @brief Called by main.c to check if the RTC is enabled.
  * You may call this function, but outside of app_init, it should always return true.
  */
bool _watch_rtc_is_enabled(void);

/** @brief Sets the date and time. Calls watch_rtc_set_unix_time internally.
  * @param date_time The date and time you wish to set, with a year value from 0-63 representing 2020-2083.
  * @note The SAM L22 stores the year as six bits representing a value from 0 to 63. It treats this as a year
  *       offset from a reference year, which must be a leap year. Since 2020 was a leap year, and it allows
  *       useful dates through 2083, it is assumed that watch apps will use 2020 as the reference year; thus
  *       1 means 2021, 2 means 2022, etc. **You will be responsible for handling this offset in your code**,
  *       if the calendar year is needed for timestamp calculation logic or display purposes.
  */
void watch_rtc_set_date_time(rtc_date_time_t date_time);

/** @brief Returns the date and time. Calls watch_rtc_get_unix_time internally.
  * @return A rtc_date_time_t with the current date and time, with a year value from 0-63 representing 2020-2083.
  * @see watch_rtc_set_date_time for notes about how the year is stored.
  */
rtc_date_time_t watch_rtc_get_date_time(void);

/** @brief Returns the date and time that the watch defaults to when power cycled. Often comes from the Makefile flags.
  * @return A rtc_date_time_t with the current date and time, with a year value from 0-63 representing 2020-2083.
  */
rtc_date_time_t watch_get_init_date_time(void);

/** @brief Set the current UTC date and time using a unix timestamp
 */
void watch_rtc_set_unix_time(unix_timestamp_t unix_time);

/** @brief Get the current UTC date and time using a unix timestamp
 */ 
unix_timestamp_t watch_rtc_get_unix_time(void);

/** @brief Get the current value of the internal hardware counter
 *  @details The counter starts at 0 and it increases at a 128Hz rate until it overflows and starts over.
 *           We never manually set the counter. Doing so allows us to calculate absolute elapsed and more.
 *           When the user sets the time, what is modified is the reference time (i.e. the date and time when
 *           the counter is 0).
 */ 
rtc_counter_t watch_rtc_get_counter(void);

/** @brief Get the RTC counter frequency.
 */
uint32_t watch_rtc_get_frequency(void);

/** @brief Get how many counter ticks are in one minute.
 */
uint32_t watch_rtc_get_ticks_per_minute(void);

/** @brief Registers a callback that will be called when the RTC counter matches the target counter.
  * @param callback The function you wish to have called when the target counter is reached. If this value is NULL, the comp
  *                 interrupt will still be enabled, but no callback function will be called.
  * @param counter The time that you wish to match. The date is currently ignored.
  * @param index We can have up to 8 active callbacks at a time. This parameter specifies which of the 8 callbacks should be set.
  * @details The hardware RTC provides us with single interrupt that fires when the RTC counter matches a target counter COMP0.
  *          With a little bit of logic, we can provide multiple active compare callbacks. Every time a comp callback is 
  *          registered/disabled/fired we iterate over all the active comp callbacks and set the hardware COMP0 counter
  *          to the next occurring one.
  *          With this very simple API, movement can implement one-shot timers to turn off the led and determine button longpresses
  *          as well as the inactivity timeouts for resigning and sleeping, as well as emulating the top of the minute alarm.
  */
void watch_rtc_register_comp_callback(watch_cb_t callback, rtc_counter_t counter, uint8_t index);

/** @brief Just like watch_rtc_register_comp_callback but doesn't actually schedule the callback
  *
  *  Useful if you need register multiple callbacks at once, avoids multiple calls to the expensive watch_rtc_schedule_next_comp:
  *  Usage:
  *  watch_rtc_register_comp_callback_no_schedule(cb0, counter0, index0);
  *  watch_rtc_register_comp_callback_no_schedule(cb1, counter1, index1);
  *  watch_rtc_schedule_next_comp();
  */
void watch_rtc_register_comp_callback_no_schedule(watch_cb_t callback, rtc_counter_t counter, uint8_t index);

/** @brief Disables the specified comp callback.
  */
void watch_rtc_disable_comp_callback(uint8_t index);

/** @brief Just like watch_rtc_disable_comp_callback but doesn't actually schedule the callback
  *
  *  Useful if you need disable multiple callbacks at once, avoids multiple calls to the expensive watch_rtc_schedule_next_comp:
  *  Usage:
  *  watch_rtc_disable_comp_callback_no_schedule(index0);
  *  watch_rtc_disable_comp_callback_no_schedule(index1);
  *  watch_rtc_schedule_next_comp();
  */
/** @brief Disables the specified comp callback.
  */
void watch_rtc_disable_comp_callback_no_schedule(uint8_t index);

/** @brief Determines the first comp callback that should fire and schedule it with the RTC
  *
  * You would never need to call this manually, unless you used the 'no_schedule' functions above.
  */
void watch_rtc_schedule_next_comp(void);

/** @brief Disables the alarm callback.
  */
// void watch_rtc_disable_alarm_callback(void);

/** @brief Registers a "tick" callback that will be called once per second.
  * @param callback The function you wish to have called when the clock ticks. If you pass in NULL, the tick
  *                 interrupt will still be enabled, but no callback function will be called.
  * @note this is equivalent to calling watch_rtc_register_periodic_callback with a frequency of 1. It can be
  *       disabled with either watch_rtc_disable_tick_callback() or watch_rtc_disable_periodic_callback(1),
  *       and will also be disabled when watch_rtc_disable_all_periodic_callbacks is called.
  */
void watch_rtc_register_tick_callback(watch_cb_t callback);

/** @brief Disables the tick callback for the given period.
  */
void watch_rtc_disable_tick_callback(void);

/** @brief Registers a callback that will be called at a configurable period.
  * @param callback The function you wish to have called at the specified period. If you pass in NULL, the periodic
  *                 interrupt will still be enabled, but no callback function will be called.
  * @param frequency The frequency of the tick in Hz. **Must be a power of 2**, from 1 to 128 inclusive.
  * @note A 1 Hz tick (@see watch_rtc_register_tick_callback) is suitable for most applications, in that it gives you a
  *       chance to update the display once a second — an ideal update rate for a watch! If however you are displaying
  *       a value (such as an accelerometer output) that updates more frequently than once per second, you may want to
  *       tick at 16 or 32 Hz to update the screen more quickly. Just remember that the more frequent the tick, the more
  *       power your app will consume. Ideally you should enable the fast tick only when the user requires it (i.e. in
  *       response to an input event), and move back to the slow tick after some time.
  */
void watch_rtc_register_periodic_callback(watch_cb_t callback, uint8_t frequency);

/** @brief Disables the tick callback for the given period.
  * @param frequency The frequency of the tick you wish to disable, in Hz. **Must be a power of 2**, from 1 to 128.
  */
void watch_rtc_disable_periodic_callback(uint8_t frequency);

/** @brief Disables tick callbacks for the given periods (as a bitmask).
  * @param mask The frequencies of tick callbacks you wish to disable, in Hz.
  * The 128 Hz callback is 0b1, the 64 Hz callback is 0b10, the 32 Hz callback is 0b100, etc.
  */
void watch_rtc_disable_matching_periodic_callbacks(uint8_t mask);

/** @brief Disables all periodic callbacks, including the once-per-second tick callback.
  */
void watch_rtc_disable_all_periodic_callbacks(void);

/** @brief Enable/disable RTC while in-flight. This is quite dangerous operation, so we repeat writing register twice.
 * Used when temporarily pausing RTC when adjusting subsecond, which are not accessible otherwise.
  */
void watch_rtc_enable(bool en);

/** @brief Adjusts frequency correction in single register write. Not waiting for syncronisation to save power - if you won't write new
 * correction value in the same ~millisecond - will not cause issue.
  */
void watch_rtc_freqcorr_write(int16_t value, int16_t sign);

/// @}
