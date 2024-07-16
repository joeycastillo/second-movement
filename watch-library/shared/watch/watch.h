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
/// @file watch.h

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "pins.h"

#ifdef __EMSCRIPTEN__
#include "watch_main_loop.h"
#endif // __EMSCRIPTEN__

/** @mainpage Sensor Watch Documentation
 *  @brief This documentation covers most of the functions you will use to interact with the Sensor Watch
           hardware. It is divided into the following sections:
            - @ref watch - This section covers the top-level functions that affect global operation of the watch.
            - @ref rtc - This section covers functions related to the SAM L22's real-time clock peripheral, including
                         date, time and alarm functions.
            - @ref slcd - This section covers functions related to the Segment LCD display driver, which is responsible
                          for displaying strings of characters and indicators on the main watch display.
            - @ref buttons - This section covers functions related to the three buttons: Light, Mode and Alarm.
            - @ref led - This section covers functions related to the bi-color red/green LED mounted behind the LCD.
            - @ref buzzer - This section covers functions related to the piezo buzzer.
            - @ref adc - This section covers functions related to the SAM L22's analog-to-digital converter, as well as
                         configuring and reading values from the five analog-capable pins on the 9-pin connector.
            - @ref gpio - This section covers functions related to general-purpose input and output signals.
            - @ref i2c - This section covers functions related to the SAM L22's built-I2C driver, including configuring
                         the I2C bus, putting values directly on the bus and reading data from registers on I2C devices.
            - @ref spi - This section covers functions related to the SAM L22's built-in SPI driver.
            - @ref uart - This section covers functions related to the UART peripheral.
            - @ref deepsleep - This section covers functions related to preparing for and entering BACKUP mode, the
                               deepest sleep mode available on the SAM L22.
 */

/** @brief Typedef for a general-purpose callback function.
 */
typedef void (*watch_cb_t)(void);

// #include "watch_rtc.h"
#include "watch_slcd.h"
// #include "watch_extint.h"
#include "watch_led.h"
#include "watch_buzzer.h"
// #include "watch_adc.h"
// #include "watch_gpio.h"
// #include "watch_i2c.h"
// #include "watch_spi.h"
// #include "watch_uart.h"
// #include "watch_storage.h"
// #include "watch_deepsleep.h"

/** @brief Interrupt handler for the SYSTEM interrupt, which handles MCLK,
 *         OSC32KCTRL, OSCCTRL, PAC, PM and SUPC.
 *  @details We use this to detect when the system voltage has dipped below
 *           2.6V, which means it's time to disable the voltage regulator's
 *           high-efficiency mode, which only works down to 2.5V.
 */
void irq_handler_system(void);

/** @brief Resets the watch into the UF2 bootloader mode.
  */
void watch_reset_to_bootloader(void);
