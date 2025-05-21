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

////< @file watch_adc.h

#include "watch.h"

/** @addtogroup adc Analog Input
  * @brief This section covers functions related to the SAM L22's analog-to-digital converter,
  *        as well as configuring and reading values from the five analog-capable pins on the
  *        9-pin connector.
  */
/// @{
/** @brief Enables the ADC peripheral. You must call this before attempting to read a value
  *        from an analog pin.
  */
void watch_enable_adc(void);

/** @brief Configures the selected pin for analog input.
  * @param pin One of the analog pins, access using the HAL_GPIO_Ax_pin() macro.
  */
void watch_enable_analog_input(const uint16_t pin);

/** @brief Reads an analog value from one of the pins.
  * @param pin One of the analog pins, access using the HAL_GPIO_Ax_pin() macro.
  * @return a 16-bit unsigned integer from 0-65535 representing the sampled value, unless you
  *         have changed the number of samples. @see watch_set_num_analog_samples for details
  *         on how that function changes the values returned from this one.
  **/
uint16_t watch_get_analog_pin_level(const uint16_t pin);

/** @brief Returns the voltage of the VCC supply in millivolts (i.e. 3000 mV == 3.0 V). If running on
  *        a coin cell, this will be the battery voltage. If the ADC is not running when this function
  *        is called, it enabled the ADC briefly, and returns it to the off state.
  * @details Unlike other ADC functions, this function does not return a raw value from the ADC, but
  *          rather scales it to an actual number of millivolts. This is because the ADC doesn't let
  *          us measure VCC per se; it instead lets us measure VCC / 4, and we choose to measure it
  *          against the internal reference voltage of 1.024 V. In short, the ADC gives us a number
  *          that's complicated to deal with, so we just turn it into a useful number for you :)
  * @note This function depends on INTREF being 1.024V. If you have changed it by poking at the supply
  *       controller's VREF.SEL bits, this function will return inaccurate values.
  */
uint16_t watch_get_vcc_voltage(void);

/** @brief Disables the analog circuitry on the selected pin.
  * @param pin One of pins A0-A4.
  */
void watch_disable_analog_input(const uint16_t pin);

/** @brief Disables the ADC peripheral.
  * @note You will need to call watch_enable_adc to re-enable the ADC peripheral. When you do, it will
  *       have the default settings of 16 samples and 1 measurement cycle; if you customized these
  *       parameters, you will need to set them up again.
  **/
void watch_disable_adc(void);

/// @}
