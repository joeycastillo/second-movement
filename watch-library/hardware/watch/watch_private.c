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

#include "watch_private.h"
#include "adc.h"
#include "tcc.h"
#include "tc.h"
#include "usb.h"
#include "system.h"

void _watch_init(void) {
    // set frequency to 4 MHz
    set_cpu_frequency(4000000);

    // disable debugger hot-plugging
    HAL_GPIO_SWCLK_pmuxdis();
    HAL_GPIO_SWCLK_off();

    // RAM should be back-biased in STANDBY
    PM->STDBYCFG.bit.BBIASHS = 1;

    // Use switching regulator for lower power consumption.
    SUPC->VREG.bit.SEL = 1;

    // per Microchip datasheet clarification DS80000782,
    // work around silicon erratum 1.7.2, which causes the microcontroller to lock up on leaving standby:
    // request that the voltage regulator run in standby, and also that it switch to PL0.
    SUPC->VREG.bit.RUNSTDBY = 1;
    SUPC->VREG.bit.STDBYPL0 = 1;
    while(!SUPC->STATUS.bit.VREGRDY); // wait for voltage regulator to become ready

    // check the battery voltage...
    uint16_t battery_voltage = watch_get_vcc_voltage();
    // ...because we can enable the more efficient low power regulator if the system voltage is > 2.5V
    // still, enable LPEFF only if the battery voltage is comfortably above this threshold.
    if (battery_voltage >= 2700) {
        SUPC->VREG.bit.LPEFF = 1;
    } else {
        SUPC->VREG.bit.LPEFF = 0;
    }

    // set up the brownout detector (low battery warning)
    NVIC_DisableIRQ(SYSTEM_IRQn);
    NVIC_ClearPendingIRQ(SYSTEM_IRQn);
    NVIC_EnableIRQ(SYSTEM_IRQn);
    SUPC->BOD33.bit.ENABLE = 0;     // BOD33 must be disabled to change its configuration
    SUPC->BOD33.bit.VMON = 0;       // Monitor VDD in active and standby mode
    SUPC->BOD33.bit.ACTCFG = 1;     // Enable sampling mode when active
    SUPC->BOD33.bit.RUNSTDBY = 1;   // Enable sampling mode in standby
    SUPC->BOD33.bit.STDBYCFG = 1;   // Run in standby
    SUPC->BOD33.bit.RUNBKUP = 0;    // Don't run in backup mode
    SUPC->BOD33.bit.PSEL = 0x9;     // Check battery level every second (we'll change this before entering sleep)
    SUPC->BOD33.bit.LEVEL = 34;     // Detect brownout at 2.6V (1.445V + level * 34mV)
    SUPC->BOD33.bit.ACTION = 0x2;   // Generate an interrupt when BOD33 is triggered
    SUPC->BOD33.bit.HYST = 0;       // Disable hysteresis
    while(!SUPC->STATUS.bit.B33SRDY); // wait for BOD33 to sync

    // Enable interrupt on BOD33 detect
    SUPC->INTENSET.bit.BOD33DET = 1;
    SUPC->BOD33.bit.ENABLE = 1;

    // External wake depends on RTC; calendar is a required module.
    _watch_rtc_init();

    // set up state
    btn_alarm_callback = NULL;
    a2_callback = NULL;
    a4_callback = NULL;
}

static inline void _watch_wait_for_entropy() {
    while (!TRNG->INTFLAG.bit.DATARDY);
}

void watch_disable_TRNG(void) {
    // per Microchip datasheet clarification DS80000782,
    // silicon erratum 1.16.1 indicates that the TRNG may leave internal components powered after being disabled.
    // the workaround is to disable the TRNG by clearing the control register, twice.
    TRNG->CTRLA.bit.ENABLE = 0;
    TRNG->CTRLA.bit.ENABLE = 0;
}

// this function is called by arc4random to get entropy for random number generation.
int getentropy(void *buf, size_t buflen);

// let's use the SAM L22's true random number generator to seed the PRNG!
int getentropy(void *buf, size_t buflen) {
    MCLK->APBCMASK.bit.TRNG_ = 1;
    TRNG->CTRLA.bit.ENABLE = 1;

    size_t i = 0;
    while(i < buflen / 4) {
        _watch_wait_for_entropy();
        ((uint32_t *)buf)[i++] = TRNG->DATA.reg;
    }

    // but what if they asked for an awkward number of bytes?
    if (buflen % 4) {
        // all good: let's fill in one, two or three bytes at the end of the buffer.
        _watch_wait_for_entropy();
        uint32_t last_little_bit = TRNG->DATA.reg;
        for(size_t j = 0; j <= (buflen % 4); j++) {
            ((uint8_t *)buf)[i * 4 + j] = (last_little_bit >> (j * 8)) & 0xFF;
        }
    }

    watch_disable_TRNG();
    MCLK->APBCMASK.bit.TRNG_ = 0;

    return 0;
}


void _watch_enable_usb(void) {
    set_cpu_frequency(8000000);
    usb_init();
    usb_enable();
}

void watch_reset_to_bootloader(void) {
    volatile uint32_t *dbl_tap_ptr = ((volatile uint32_t *)(HSRAM_ADDR + HSRAM_SIZE - 4));
    *dbl_tap_ptr = 0xf01669ef; // from the UF2 bootloaer: uf2.h line 255
    NVIC_SystemReset();
}
