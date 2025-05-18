/*
 * MIT License
 *
 * Copyright (c) 2022 Joey Castillo
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

#include "thermistor_driver.h"
#include "sam.h"
#include "watch.h"
#include "watch_utility.h"

// assume we have no thermistor until thermistor_driver_init is called.
static bool has_thermistor = false;

bool thermistor_driver_init(void) {
    // once called, assume we have a thermistor unless proven otherwise
    has_thermistor = true;
    uint16_t value;
#if THERMISTOR_ENABLE_VALUE == false
    const uint16_t threshold_value = 60000;
#else
    const uint16_t threshold_value = 5000;
#endif

    watch_enable_adc();
    // Enable analog circuitry on the sense pin, which is tied to the thermistor resistor divider.
    HAL_GPIO_TEMPSENSE_in();
    HAL_GPIO_TEMPSENSE_pmuxen(HAL_GPIO_PMUX_ADC);

    // Enable digital output on the enable pin, which is the power to the thermistor circuit.
    HAL_GPIO_TS_ENABLE_out();
    // and make sure it's DISABLED.
    HAL_GPIO_TS_ENABLE_write(!THERMISTOR_ENABLE_VALUE);

    // If the temperature sensor is connected, pulling the TS_ENABLE line to its disabled value
    // connects both sides of the voltage divider to the same potential. If enable value is false,
    // this will be high, if enable value is true it will be low, and TEMPSENSE should read the same.
    value = watch_get_analog_pin_level(HAL_GPIO_TEMPSENSE_pin());

    // If setting TS_ENABLE has no effect, there is no thermistor circuit connected to TEMPSENSE.
    if (value < threshold_value) has_thermistor = false;

    // now flip it to enable the temperature sensor
    HAL_GPIO_TS_ENABLE_write(THERMISTOR_ENABLE_VALUE);

    // If the temperature sensor is connected, pulling the TS_ENABLE line to its ENABLED value
    // means we should get a reasonable temperature at this point.
    value = watch_get_analog_pin_level(HAL_GPIO_TEMPSENSE_pin());

    // value should be >15000 and <55000 and (between -4° and 76° C)
    if (value < 15000 || value > 55000) has_thermistor = false;

    // clean up, disable everything we enabled earlier.
    watch_disable_adc();
    HAL_GPIO_TEMPSENSE_off();
    HAL_GPIO_TEMPSENSE_pmuxdis();
    HAL_GPIO_TS_ENABLE_write(!THERMISTOR_ENABLE_VALUE);
    HAL_GPIO_TS_ENABLE_off();

    return has_thermistor;
}

void thermistor_driver_enable(void) {
    if (!has_thermistor) return;

    // Enable the ADC peripheral, which we'll use to read the thermistor value.
    watch_enable_adc();
    // Enable analog circuitry on the sense pin, which is tied to the thermistor resistor divider.
    HAL_GPIO_TEMPSENSE_in();
    HAL_GPIO_TEMPSENSE_pmuxen(HAL_GPIO_PMUX_ADC);
    // Enable digital output on the enable pin, which is the power to the thermistor circuit.
    HAL_GPIO_TS_ENABLE_out();
    // and make sure it's off.
    HAL_GPIO_TS_ENABLE_write(!THERMISTOR_ENABLE_VALUE);
}

void thermistor_driver_disable(void) {
    if (!has_thermistor) return;

    // Disable the ADC peripheral.
    watch_disable_adc();
    // Disable analog circuitry on the sense pin to save power.
    HAL_GPIO_TEMPSENSE_pmuxdis();
    HAL_GPIO_TEMPSENSE_off();
    // Disable the enable pin's output circuitry.
    HAL_GPIO_TS_ENABLE_off();
}

float thermistor_driver_get_temperature(void) {
    if (!has_thermistor) return (float) 0xFFFFFFFF;

    // set the enable pin to the level that powers the thermistor circuit.
    HAL_GPIO_TS_ENABLE_write(THERMISTOR_ENABLE_VALUE);
    // get the sense pin level
    uint16_t value = watch_get_analog_pin_level(HAL_GPIO_TEMPSENSE_pin());
    // and then set the enable pin to the opposite value to power down the thermistor circuit.
    HAL_GPIO_TS_ENABLE_write(!THERMISTOR_ENABLE_VALUE);

    return watch_utility_thermistor_temperature(value, THERMISTOR_HIGH_SIDE, THERMISTOR_B_COEFFICIENT, THERMISTOR_NOMINAL_TEMPERATURE, THERMISTOR_NOMINAL_RESISTANCE, THERMISTOR_SERIES_RESISTANCE);
}
