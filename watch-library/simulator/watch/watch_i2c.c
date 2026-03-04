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

#include "watch_i2c.h"
#include <math.h>
#include <emscripten.h>

// OPT3001 I2C device simulation
#define OPT3001_ADDRESS 0x44

#define OPT3001_REG_RESULT          0x00
#define OPT3001_REG_CONFIG          0x01
#define OPT3001_REG_LOW_LIMIT       0x02
#define OPT3001_REG_HIGH_LIMIT      0x03
#define OPT3001_REG_MANUFACTURER_ID 0x7E
#define OPT3001_REG_DEVICE_ID       0x7F

static uint8_t opt3001_register_pointer = 0;
static uint16_t opt3001_config = 0;

/// Convert a lux value to the OPT3001 raw register format (4-bit exponent + 12-bit mantissa).
/// lux = 0.01 * 2^exp * mantissa
static uint16_t opt3001_lux_to_raw(double lux) {
    if (lux <= 0) return 0;
    uint8_t exp = 0;
    while (exp < 11 && lux > 40.95 * pow(2, exp)) {
        exp++;
    }
    uint16_t mantissa = (uint16_t)(lux / (0.01 * pow(2, exp)));
    if (mantissa > 4095) mantissa = 4095;
    return ((uint16_t)exp << 12) | mantissa;
}

static uint16_t opt3001_read_register(uint8_t reg) {
    switch (reg) {
        case OPT3001_REG_RESULT: {
            double lux = EM_ASM_DOUBLE({
                return window.light_level || 0.0;
            });
            return opt3001_lux_to_raw(lux);
        }
        case OPT3001_REG_CONFIG:
            // Always report ConversionReady (bit 7 of low byte).
            return opt3001_config | 0x0080;
        case OPT3001_REG_LOW_LIMIT:
            return 0x0000;
        case OPT3001_REG_HIGH_LIMIT:
            return 0xBFFF;
        case OPT3001_REG_MANUFACTURER_ID:
            return 0x5449; // "TI"
        case OPT3001_REG_DEVICE_ID:
            return 0x3001;
        default:
            return 0;
    }
}

void watch_enable_i2c(void) {}

void watch_disable_i2c(void) {}

int8_t watch_i2c_send(int16_t addr, uint8_t *buf, uint16_t length) {
    if (addr == OPT3001_ADDRESS) {
        if (length >= 1) {
            opt3001_register_pointer = buf[0];
        }
        if (length == 3) {
            // Register write: buf[0]=reg, buf[1..2]=data (big-endian)
            uint16_t value = ((uint16_t)buf[1] << 8) | buf[2];
            if (buf[0] == OPT3001_REG_CONFIG) {
                opt3001_config = value;
            }
        }
    }
    return 0;
}

int8_t watch_i2c_receive(int16_t addr, uint8_t *buf, uint16_t length) {
    if (addr == OPT3001_ADDRESS && length >= 2) {
        uint16_t value = opt3001_read_register(opt3001_register_pointer);
        buf[0] = (uint8_t)(value >> 8);
        buf[1] = (uint8_t)(value & 0xFF);
    }
    return 0;
}

int8_t watch_i2c_write8(int16_t addr, uint8_t reg, uint8_t data) {
    uint8_t buf[2] = {reg, data};
    return watch_i2c_send(addr, buf, 2);
}

uint8_t watch_i2c_read8(int16_t addr, uint8_t reg) {
    uint8_t buf[2] = {reg, 0};
    watch_i2c_send(addr, buf, 1);
    watch_i2c_receive(addr, buf, 2);
    return buf[1];
}

uint16_t watch_i2c_read16(int16_t addr, uint8_t reg) {
    uint8_t buf[2] = {0, 0};
    uint8_t reg_buf = reg;
    watch_i2c_send(addr, &reg_buf, 1);
    watch_i2c_receive(addr, buf, 2);
    return ((uint16_t)buf[0] << 8) | buf[1];
}

uint32_t watch_i2c_read24(int16_t addr, uint8_t reg) {
    (void) addr;
    (void) reg;
    return 0;
}

uint32_t watch_i2c_read32(int16_t addr, uint8_t reg) {
    (void) addr;
    (void) reg;
    return 0;
}
