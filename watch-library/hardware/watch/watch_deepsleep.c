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

#include "watch_deepsleep.h"
#include "app.h"
#include "watch.h"
#include "watch_private.h"

void sleep(const uint8_t mode) {
    PM->SLEEPCFG.bit.SLEEPMODE = mode;

    // wait for the mode set to actually take, per SLEEPCFG note in data sheet:
    // "A small latency happens between the store instruction and actual writing
    // of the SLEEPCFG register due to bridges. Software has to make sure the
    // SLEEPCFG register reads the wanted value before issuing WFI instruction."
    while(PM->SLEEPCFG.bit.SLEEPMODE != mode);

    __DSB();
	__WFI();
}

void watch_register_extwake_callback(uint8_t pin, watch_cb_t callback, bool level) {
    uint32_t config = RTC->MODE2.TAMPCTRL.reg;

    if (pin == HAL_GPIO_BTN_ALARM_pin()) {
        HAL_GPIO_BTN_ALARM_in();
        HAL_GPIO_BTN_ALARM_pulldown();
        HAL_GPIO_BTN_ALARM_pmuxen(HAL_GPIO_PMUX_RTC);
        btn_alarm_callback = callback;
        config &= ~(3 << RTC_TAMPCTRL_IN2ACT_Pos);
        config &= ~(1 << RTC_TAMPCTRL_TAMLVL2_Pos);
        config |= 1 << RTC_TAMPCTRL_IN2ACT_Pos;
        if (level) config |= 1 << RTC_TAMPCTRL_TAMLVL2_Pos;
    } else if (pin == HAL_GPIO_A2_pin()) {
        HAL_GPIO_A2_in();
        HAL_GPIO_A2_pmuxen(HAL_GPIO_PMUX_RTC);
        a2_callback = callback;
        config &= ~(3 << RTC_TAMPCTRL_IN1ACT_Pos);
        config &= ~(1 << RTC_TAMPCTRL_TAMLVL1_Pos);
        config |= 1 << RTC_TAMPCTRL_IN1ACT_Pos;
        if (level) config |= 1 << RTC_TAMPCTRL_TAMLVL1_Pos;
    } else if (pin == HAL_GPIO_A4_pin()) {
        HAL_GPIO_A4_in();
        HAL_GPIO_A4_pmuxen(HAL_GPIO_PMUX_RTC);
        a4_callback = callback;
        config &= ~(3 << RTC_TAMPCTRL_IN0ACT_Pos);
        config &= ~(1 << RTC_TAMPCTRL_TAMLVL0_Pos);
        config |= 1 << RTC_TAMPCTRL_IN0ACT_Pos;
        if (level) config |= 1 << RTC_TAMPCTRL_TAMLVL0_Pos;
    }

    // disable the RTC
    RTC->MODE2.CTRLA.bit.ENABLE = 0;
    while (RTC->MODE2.SYNCBUSY.bit.ENABLE); // wait for RTC to be disabled

    // update the configuration
    RTC->MODE2.TAMPCTRL.reg = config;

    // re-enable the RTC
    RTC->MODE2.CTRLA.bit.ENABLE = 1;

    NVIC_ClearPendingIRQ(RTC_IRQn);
    NVIC_EnableIRQ(RTC_IRQn);
    RTC->MODE2.INTENSET.reg = RTC_MODE2_INTENSET_TAMPER;
}

void watch_disable_extwake_interrupt(uint8_t pin) {
    uint32_t config = RTC->MODE2.TAMPCTRL.reg;

    if (pin == HAL_GPIO_BTN_ALARM_pin()) {
        btn_alarm_callback = NULL;
        config &= ~(3 << RTC_TAMPCTRL_IN2ACT_Pos);
    } else if (pin == HAL_GPIO_A2_pin()) {
        a2_callback = NULL;
        config &= ~(3 << RTC_TAMPCTRL_IN1ACT_Pos);
    }
    else if (pin == HAL_GPIO_A4_pin()) {
        a4_callback = NULL;
        config &= ~(3 << RTC_TAMPCTRL_IN0ACT_Pos);
    }

    // disable the RTC
    RTC->MODE2.CTRLA.bit.ENABLE = 0;
    while (RTC->MODE2.SYNCBUSY.bit.ENABLE); // wait for RTC to be disabled

    // update the configuration
    RTC->MODE2.TAMPCTRL.reg = config;

    // re-enable the RTC
    RTC->MODE2.CTRLA.bit.ENABLE = 1;
}

void watch_store_backup_data(uint32_t data, uint8_t reg) {
    if (reg < 8) {
        RTC->MODE0.BKUP[reg].reg = data;
    }
}

uint32_t watch_get_backup_data(uint8_t reg) {
    if (reg < 8) {
        return RTC->MODE0.BKUP[reg].reg;
    }

    return 0;
}

static void _watch_disable_all_pins_except_rtc(void) {
    uint32_t config = RTC->MODE0.TAMPCTRL.reg;
    uint32_t portb_pins_to_disable = 0xFFFFFFFF;

    /// FIXME: Watch library shouldn't be responsible for this, but Movement uses PB00 and PB03 for activity and orientation tracking.
    ///        As such, we need to keep them on.
    portb_pins_to_disable &= 0xFFFFFFF6;
    // if there's an action set on RTC/IN[0], leave PB00 configured
    if (config & RTC_TAMPCTRL_IN0ACT_Msk) portb_pins_to_disable &= 0xFFFFFFFE;
    // same with RTC/IN[1] and PB02
    if (config & RTC_TAMPCTRL_IN1ACT_Msk) portb_pins_to_disable &= 0xFFFFFFFB;

    // port A: that last B is to always keep PA02 configured as-is; that's our ALARM button.
    PORT->Group[0].DIRCLR.reg = 0xFFFFFFFB;
    // WRCONFIG can only set half the pins at a time, so we need two writes. This sets pins 0-15.
	PORT->Group[0].WRCONFIG.reg = PORT_WRCONFIG_WRPINCFG | 0xFFFB;
    // ...and adding the HWSEL flag configures 16-31.
	PORT->Group[0].WRCONFIG.reg = PORT_WRCONFIG_HWSEL | PORT_WRCONFIG_WRPINCFG | 0xffff;

    // port B: disable all pins we didn't save above.
    PORT->Group[1].DIRCLR.reg = portb_pins_to_disable;
	PORT->Group[1].WRCONFIG.reg = PORT_WRCONFIG_WRPINCFG | (portb_pins_to_disable & 0xFFFF);
	PORT->Group[1].WRCONFIG.reg = PORT_WRCONFIG_HWSEL | PORT_WRCONFIG_WRPINCFG | (portb_pins_to_disable >> 16);
}

static void _watch_disable_all_peripherals_except_slcd(void) {
    _watch_disable_tcc();
    watch_disable_adc();
    watch_disable_external_interrupts();

    /// TODO: Actually disable all these peripherals? Disabling I2C seems to have no impact fwiw.
    // watch_disable_i2c();
    // SERCOM3->USART.CTRLA.reg &= ~SERCOM_USART_CTRLA_ENABLE;
    // MCLK->APBCMASK.reg &= ~MCLK_APBCMASK_SERCOM3;
}

void watch_enter_sleep_mode(void) {
    // disable all other peripherals
    _watch_disable_all_peripherals_except_slcd();

    // disable tick interrupt
    watch_rtc_disable_all_periodic_callbacks();

    // disable brownout detector interrupt, which could inadvertently wake us up.
    SUPC->INTENCLR.bit.BOD33DET = 1;

    // disable all pins
    _watch_disable_all_pins_except_rtc();

    // enter standby (4); we basically hang out here until an interrupt wakes us.
    sleep(4);

    // and we awake! re-enable the brownout detector
    SUPC->INTENSET.bit.BOD33DET = 1;

    // call app_setup so the app can re-enable everything we disabled.
    app_setup();
}

void watch_enter_backup_mode(void) {
    watch_rtc_disable_all_periodic_callbacks();
    _watch_disable_all_pins_except_rtc();

    // go into backup sleep mode (5). when we exit, the reset controller will take over.
    sleep(5);
}
