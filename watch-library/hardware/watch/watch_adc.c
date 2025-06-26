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

#include "watch_adc.h"
#include "adc.h"

void watch_enable_adc(void) {
    adc_init();
    adc_enable();
}

void watch_enable_analog_input(const uint16_t port_pin) {
    uint8_t port = port_pin >> 8;
    uint16_t pin = port_pin & 0xF;

    PORT->Group[port].DIRCLR.reg = (1 << pin);
    PORT->Group[port].PINCFG[pin].reg |= PORT_PINCFG_INEN;
    PORT->Group[port].PINCFG[pin].reg &= ~PORT_PINCFG_PULLEN;
    PORT->Group[port].PINCFG[pin].reg |= PORT_PINCFG_PMUXEN;
    if (pin & 1) {
        PORT->Group[port].PMUX[pin>>1].bit.PMUXO = HAL_GPIO_PMUX_ADC;
    } else {
        PORT->Group[port].PMUX[pin>>1].bit.PMUXE = HAL_GPIO_PMUX_ADC;
    }
}

uint16_t watch_get_analog_pin_level(const uint16_t pin) {
    return adc_get_analog_value(pin);
}

/// TODO: put reference voltage stuff into gossamer?
void _watch_set_analog_reference_voltage(uint8_t reference);
void _watch_set_analog_reference_voltage(uint8_t reference) {
    ADC->CTRLA.bit.ENABLE = 0;

    if (reference == ADC_REFCTRL_REFSEL_INTREF_Val) SUPC->VREF.bit.VREFOE = 1;
    else SUPC->VREF.bit.VREFOE = 0;

    ADC->REFCTRL.bit.REFSEL = reference;
    ADC->CTRLA.bit.ENABLE = 1;
    while (ADC->SYNCBUSY.reg);
    // throw away one measurement after reference change (the channel doesn't matter).
    adc_get_analog_value_for_channel(ADC_INPUTCTRL_MUXPOS_SCALEDCOREVCC);
}

uint16_t watch_get_vcc_voltage(void) {
    // stash the previous reference so we can restore it when we're done.
    uint8_t oldref = ADC->REFCTRL.bit.REFSEL;
    // same with the previous state of the ADC
    bool adc_was_disabled = !adc_is_enabled();

    // enable the ADC if needed
    if (adc_was_disabled) watch_enable_adc();

    // if we weren't already using the internal reference voltage, select it now.
    if (oldref != ADC_REFCTRL_REFSEL_INTREF_Val) _watch_set_analog_reference_voltage(ADC_REFCTRL_REFSEL_INTREF_Val);

    // get the data
    uint32_t raw_val = adc_get_analog_value_for_channel(ADC_INPUTCTRL_MUXPOS_SCALEDIOVCC_Val);

    // restore the old reference, if needed.
    if (oldref != ADC_REFCTRL_REFSEL_INTREF_Val) _watch_set_analog_reference_voltage(oldref);

    // and restore the ADC to its previous state
    if (adc_was_disabled) watch_disable_adc();

    return (uint16_t)((raw_val * 1000) / (1024 * 1 << ADC->AVGCTRL.bit.SAMPLENUM));
}

inline void watch_disable_analog_input(const uint16_t port_pin) {
    uint8_t port = port_pin >> 8;
    uint16_t pin = port_pin & 0xF;

    PORT->Group[port].DIRSET.reg = (1 << pin);			\
    PORT->Group[port].PINCFG[pin].reg &= ~(PORT_PINCFG_PULLEN | PORT_PINCFG_INEN);	\
    PORT->Group[port].PINCFG[pin].reg &= ~PORT_PINCFG_PMUXEN;	\
}

inline void watch_disable_adc(void) {
    adc_disable();
}
