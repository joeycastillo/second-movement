/*
 * MIT License
 *
 * Copyright (c) 2021 Joey Castillo
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

#include "lis2dw.h"
#include "watch.h"

#if __EMSCRIPTEN__
#include <emscripten.h>
#endif

bool lis2dw_begin(void) {
#ifdef I2C_SERCOM
    if (lis2dw_get_device_id() != LIS2DW_WHO_AM_I_VAL) {
        return false;
    }
    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL2, LIS2DW_CTRL2_VAL_BOOT);
    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL2, LIS2DW_CTRL2_VAL_SOFT_RESET);
    // Enable block data update (output registers not updated until MSB and LSB have been read) and address autoincrement
    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL2, LIS2DW_CTRL2_VAL_BDU | LIS2DW_CTRL2_VAL_IF_ADD_INC);

    // Parameters at startup: 
    //  * Data rate 0 (powered down)
    //  * Low power mode enabled
    //  * LP mode 1 (12-bit)
    //  * Bandwidth filtering ODR/2
    //  * Low pass filter path
    //  * ±2g range
    //  * Low noise mode off
    //  * FIFO disabled

    return true;
#else
    return false;
#endif
}

uint8_t lis2dw_get_device_id(void) {
#ifdef I2C_SERCOM
    return watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_WHO_AM_I);
#else
    return 0;
#endif
}

bool lis2dw_have_new_data(void) {
#ifdef I2C_SERCOM
    uint8_t retval = watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_STATUS);
    return retval & LIS2DW_STATUS_VAL_DRDY;
#else
    return false;
#endif
}

lis2dw_reading_t lis2dw_get_raw_reading(void) {
    lis2dw_reading_t retval = {0};
#ifdef I2C_SERCOM
    uint8_t buffer[6];
    uint8_t reg = LIS2DW_REG_OUT_X_L | 0x80; // set high bit for consecutive reads

    watch_i2c_send(LIS2DW_ADDRESS, &reg, 1);
    watch_i2c_receive(LIS2DW_ADDRESS, (uint8_t *)&buffer, 6);

    retval.x = buffer[0];
    retval.x |= ((uint16_t)buffer[1]) << 8;
    retval.y = buffer[2];
    retval.y |= ((uint16_t)buffer[3]) << 8;
    retval.z = buffer[4];
    retval.z |= ((uint16_t)buffer[5]) << 8;
#endif
    
    return retval;
}

lis2dw_acceleration_measurement_t lis2dw_get_acceleration_measurement(lis2dw_reading_t *out_reading) {
    lis2dw_acceleration_measurement_t retval = {0};

    #ifdef I2C_SERCOM
    lis2dw_reading_t reading = lis2dw_get_raw_reading();
    /// FIXME: We should stash the range in a static variable to avoid reading this every time, but
    /// I'm not sure the values cribbed from Adafruit's LID3DH driver are right for the LIS2DW.
    /// In particular, we have a 12 and a 14 bit mode, and I'm not sure why the value for 16g is 48.
    /// Just making a note of this before moving on; we don't currently need to read acceleration
    /// values over the bus, but once we do, this will need to be fixed.
    uint8_t range = lis2dw_get_range();
    if (out_reading != NULL) *out_reading = reading;

    // this bit is cribbed from Adafruit's LIS3DH driver; from their notes, the magic number below
    // converts from 16-bit lsb to 10-bit and divides by 1k to convert from milli-gs.
    // final value is raw_lsb => 10-bit lsb -> milli-gs -> gs
    uint8_t lsb_value = 1;
    if (range == LIS2DW_RANGE_2_G) lsb_value = 4;
    if (range == LIS2DW_RANGE_4_G) lsb_value = 8;
    if (range == LIS2DW_RANGE_8_G) lsb_value = 16;
    if (range == LIS2DW_RANGE_16_G) lsb_value = 48;

    retval.x = lsb_value * ((float)reading.x / 64000.0);
    retval.y = lsb_value * ((float)reading.y / 64000.0);
    retval.z = lsb_value * ((float)reading.z / 64000.0);
#else
    (void) out_reading;
#endif

    return retval;
}

uint16_t lis2dw_get_temperature(void) {
#ifdef I2C_SERCOM
    return watch_i2c_read16(LIS2DW_ADDRESS, LIS2DW_REG_OUT_TEMP_L);
#else
    return 0;
#endif
}

void lis2dw_set_data_rate(lis2dw_data_rate_t dataRate) {
#ifdef I2C_SERCOM
    uint8_t val = watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL1) & ~(0b1111 << 4);
    uint8_t bits = dataRate << 4;

    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL1, val | bits);
#else
    (void)dataRate;
#endif
}

lis2dw_data_rate_t lis2dw_get_data_rate(void) {
#ifdef I2C_SERCOM
    return watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL1) >> 4;
#else
    return 0;
#endif
}

void lis2dw_set_mode(lis2dw_mode_t mode) {
#ifdef I2C_SERCOM
    uint8_t val = watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL1) & ~(0b1100);
    uint8_t bits = (mode << 2) & 0b1100;

    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL1, val | bits);
#else
    (void)mode;
#endif
}

lis2dw_mode_t lis2dw_get_mode(void) {
#ifdef I2C_SERCOM
    return (lis2dw_mode_t)(watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL1) & 0b1100) >> 2;
#else
    return 0;
#endif
}

void lis2dw_set_low_power_mode(lis2dw_low_power_mode_t mode) {
#ifdef I2C_SERCOM
    uint8_t val = watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL1) & ~(0b11);
    uint8_t bits = mode & 0b11;

    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL1, val | bits);
#else
    (void)mode;
#endif
}

lis2dw_low_power_mode_t lis2dw_get_low_power_mode(void) {
#ifdef I2C_SERCOM
    return watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL1) & 0b11;
#else
    return 0;
#endif
}

void lis2dw_set_bandwidth_filtering(lis2dw_bandwidth_filtering_mode_t bwfilter) {
#ifdef I2C_SERCOM
    uint8_t val = watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL6) & ~(LIS2DW_CTRL6_VAL_BANDWIDTH_DIV20);
    uint8_t bits = bwfilter << 6;
    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL6, val | bits);
#else
    (void)bwfilter;
#endif
}

lis2dw_bandwidth_filtering_mode_t lis2dw_get_bandwidth_filtering(void) {
#ifdef I2C_SERCOM
    uint8_t retval = watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL6) & (LIS2DW_CTRL6_VAL_BANDWIDTH_DIV20);
    retval >>= 6;
    return (lis2dw_bandwidth_filtering_mode_t)retval;
#else
    return 0;
#endif
}

void lis2dw_set_range(lis2dw_range_t range) {
#ifdef I2C_SERCOM
    uint8_t val = watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL6) & ~(LIS2DW_RANGE_16_G << 4);
    uint8_t bits = range << 4;

    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL6, val | bits);
#else
    (void)range;
#endif
}

lis2dw_range_t lis2dw_get_range(void) {
#ifdef I2C_SERCOM
    uint8_t retval = watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL6) & (LIS2DW_RANGE_16_G << 4);
    retval >>= 4;
    return (lis2dw_range_t)retval;
#else
    return 0;
#endif
}

void lis2dw_set_filter_type(lis2dw_filter_t bwfilter) {
#ifdef I2C_SERCOM
    uint8_t val = watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL6) & ~(LIS2DW_CTRL6_VAL_FDS_HIGH);
    uint8_t bits = bwfilter << 3;
    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL6, val | bits);
#else
    (void)bwfilter;
#endif
}

lis2dw_filter_t lis2dw_get_filter_type(void) {
#ifdef I2C_SERCOM
    uint8_t retval = watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL6) & (LIS2DW_CTRL6_VAL_FDS_HIGH);
    retval >>= 3;
    return (lis2dw_filter_t)retval;
#else
    return 0;
#endif
}

void lis2dw_set_low_noise_mode(bool on) {
#ifdef I2C_SERCOM
    uint8_t val = watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL6) & ~(LIS2DW_CTRL6_VAL_LOW_NOISE);
    uint8_t bits = on ? LIS2DW_CTRL6_VAL_LOW_NOISE : 0;

    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL6, val | bits);
#else
    (void)on;
#endif
}

bool lis2dw_get_low_noise_mode(void) {
#ifdef I2C_SERCOM
    return (watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL1) & LIS2DW_CTRL6_VAL_LOW_NOISE) != 0;
#else
    return false;
#endif
}

inline void lis2dw_enable_fifo(void) {
#ifdef I2C_SERCOM
    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_FIFO_CTRL, LIS2DW_FIFO_CTRL_MODE_COLLECT_AND_STOP | LIS2DW_FIFO_CTRL_FTH);
#endif
}

inline void lis2dw_disable_fifo(void) {
#ifdef I2C_SERCOM
    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_FIFO_CTRL, LIS2DW_FIFO_CTRL_MODE_OFF);
#endif
}

bool lis2dw_read_fifo(lis2dw_fifo_t *fifo_data) {
#ifdef I2C_SERCOM
    uint8_t temp = watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_FIFO_SAMPLE);
    bool overrun = !!(temp & LIS2DW_FIFO_SAMPLE_OVERRUN);

    fifo_data->count = temp & LIS2DW_FIFO_SAMPLE_COUNT;

    for(int i = 0; i < fifo_data->count; i++) {
        fifo_data->readings[i] = lis2dw_get_raw_reading();
    }

    return overrun;
#else
    (void) fifo_data;
    return false;
#endif
}

void lis2dw_clear_fifo(void) {
#ifdef I2C_SERCOM
    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_FIFO_CTRL, LIS2DW_FIFO_CTRL_MODE_OFF);
    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_FIFO_CTRL, LIS2DW_FIFO_CTRL_MODE_COLLECT_AND_STOP | LIS2DW_FIFO_CTRL_FTH);
#endif
}

void lis2dw_enable_sleep(void) {
#ifdef I2C_SERCOM
    uint8_t configuration = watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_WAKE_UP_THS);
    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_WAKE_UP_THS, configuration | LIS2DW_WAKE_UP_THS_VAL_SLEEP_ON);
#endif
}

void lis2dw_disable_sleep(void) {
#ifdef I2C_SERCOM
    uint8_t configuration = watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_WAKE_UP_THS);
    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_WAKE_UP_THS, configuration & ~LIS2DW_WAKE_UP_THS_VAL_SLEEP_ON);
#endif
}

void lis2dw_enable_stationary_motion_detection(void) {
#ifdef I2C_SERCOM
    uint8_t configuration = watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_WAKE_UP_DUR);
    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_WAKE_UP_DUR, configuration | LIS2DW_WAKE_UP_DUR_STATIONARY);
#endif
}

void lis2dw_disable_stationary_motion_detection(void) {
#ifdef I2C_SERCOM
    uint8_t configuration = watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_WAKE_UP_DUR);
    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_WAKE_UP_DUR, configuration & ~LIS2DW_WAKE_UP_DUR_STATIONARY);
#endif
}

void lis2dw_configure_wakeup_threshold(uint8_t threshold) {
#ifdef I2C_SERCOM
    uint8_t configuration = watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_WAKE_UP_THS) & 0b11000000;
    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_WAKE_UP_THS, configuration | threshold);
#else
    (void)threshold;
#endif
}

void lis2dw_configure_6d_threshold(uint8_t threshold) {
#ifdef I2C_SERCOM
    uint8_t configuration = watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_TAP_THS_X) & 0b01100000;
    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_TAP_THS_X, configuration | ((threshold & 0b11) << 5));
#else
    (void)threshold;
#endif
}

void lis2dw_configure_tap_threshold(uint8_t threshold_x, uint8_t threshold_y, uint8_t threshold_z, uint8_t axes_to_enable) {
#ifdef I2C_SERCOM
    (void) threshold_x;
    (void) threshold_y;
    uint8_t configuration;
    // if (axes_to_enable & LIS2DW_REG_TAP_THS_Z_X_AXIS_ENABLE);   // X axis tap not implemented
    // if (axes_to_enable & LIS2DW_REG_TAP_THS_Z_Y_AXIS_ENABLE);   // Y axis tap not implemented
    // tap enable bitmask is the high bits of LIS2DW_REG_TAP_THS_Z
    configuration = axes_to_enable & 0b00100000; // NOTE: should be 0b11100000 to allow use of all three axes, but we're not using X or Y.
    if (axes_to_enable & LIS2DW_REG_TAP_THS_Z_Z_AXIS_ENABLE) {
        // mask out high bits if set
        configuration |= (threshold_z & 0b00011111);
    }
    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_TAP_THS_Z, configuration);
#else
    (void)threshold_x;
    (void)threshold_y;
    (void)threshold_z;
    (void)axes_to_enable;
#endif
}

void lis2dw_configure_tap_duration(uint8_t latency, uint8_t quiet, uint8_t shock) {
#ifdef I2C_SERCOM
    uint8_t configuration = (latency << 4) | ((quiet & 0b11) << 2) | (shock & 0b11);
    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_INT1_DUR, configuration);
#else
    (void)latency;
    (void)quiet;
    (void)shock;
#endif
}

void lis2dw_configure_int1(uint8_t sources) {
#ifdef I2C_SERCOM
    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL4_INT1, sources);
#else
    (void)sources;
#endif
}

void lis2dw_configure_int2(uint8_t sources) {
#ifdef I2C_SERCOM
    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL5_INT2, sources);
#else
    (void)sources;
#endif
}

void lis2dw_enable_interrupts(void) {
#ifdef I2C_SERCOM
    uint8_t configuration = watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL7);
    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL7, configuration | LIS2DW_CTRL7_VAL_INTERRUPTS_ENABLE);
#endif
}

void lis2dw_disable_interrupts(void) {
#ifdef I2C_SERCOM
    uint8_t configuration = watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL7);
    watch_i2c_write8(LIS2DW_ADDRESS, LIS2DW_REG_CTRL7, configuration & ~LIS2DW_CTRL7_VAL_INTERRUPTS_ENABLE);
#endif
}

lis2dw_wakeup_source_t lis2dw_get_wakeup_source() {
#ifdef I2C_SERCOM
    return (lis2dw_wakeup_source_t) watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_WAKE_UP_SRC);
#else
    return 0;
#endif
}

lis2dw_interrupt_source_t lis2dw_get_interrupt_source(void) {
#ifdef I2C_SERCOM
    #if __EMSCRIPTEN__
        return (lis2dw_interrupt_source_t) EM_ASM_INT({
            return window.LIS2DW_INTERRUPT_SRC || 0; // Fallback to 0 if not defined
        });
    #else
    return (lis2dw_interrupt_source_t) watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_ALL_INT_SRC);
    #endif
#else
    return 0;
#endif
}

uint8_t lis2dw_get_wakeup_threshold(void) {
#ifdef I2C_SERCOM
    return watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_WAKE_UP_THS) & 0b00111111;
#else
    return 0;
#endif
}
