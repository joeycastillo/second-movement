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

#include "watch_gpio.h"

inline void watch_enable_digital_input(const uint8_t pin) {
    PORT->Group[pin >> 5].DIRCLR.reg = (1 << (pin & 0x1F));
    PORT->Group[pin >> 5].PINCFG[pin & 0x1F].reg |= PORT_PINCFG_INEN;
    PORT->Group[pin >> 5].PINCFG[pin & 0x1F].reg &= ~PORT_PINCFG_PULLEN;
}

void watch_disable_digital_input(const uint8_t pin) {
    PORT->Group[pin >> 5].DIRCLR.reg = (1 << (pin & 0x1F));
    PORT->Group[pin >> 5].PINCFG[pin & 0x1F].reg &= ~(PORT_PINCFG_PULLEN | PORT_PINCFG_INEN);
}

void watch_enable_pull_up(const uint8_t pin) {
    PORT->Group[pin >> 5].OUTSET.reg = (1 << (pin & 0x1F));
    PORT->Group[pin >> 5].PINCFG[pin & 0x1F].reg |= PORT_PINCFG_PULLEN;
}

void watch_enable_pull_down(const uint8_t pin) {
    PORT->Group[pin >> 5].OUTCLR.reg = (1 << (pin & 0x1F));
    PORT->Group[pin >> 5].PINCFG[pin & 0x1F].reg |= PORT_PINCFG_PULLEN;
}

bool watch_get_pin_level(const uint8_t pin) {
    return (PORT->Group[pin >> 5].IN.reg & (1 << (pin & 0x1F))) != 0;
}

void watch_enable_digital_output(const uint8_t pin) {
    PORT->Group[pin >> 5].DIRSET.reg = (1 << (pin & 0x1F));
    PORT->Group[pin >> 5].PINCFG[pin & 0x1F].reg |= PORT_PINCFG_INEN;
}

void watch_disable_digital_output(const uint8_t pin) {
    PORT->Group[pin >> 5].DIRCLR.reg = (1 << (pin & 0x1F));
    PORT->Group[pin >> 5].PINCFG[pin & 0x1F].reg &= ~(PORT_PINCFG_PULLEN | PORT_PINCFG_INEN);
}

void watch_set_pin_level(const uint8_t pin, const bool level) {
    if (level) {
        PORT->Group[pin >> 5].OUTSET.reg = (1 << (pin & 0x1F));
    } else {
        PORT->Group[pin >> 5].OUTCLR.reg = (1 << (pin & 0x1F));
    }
}
