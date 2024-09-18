#include "app.h"
#include "watch.h"
#include "watch_private.h"
#include "delay.h"
#include "usb.h"
#include "tusb.h"
#include "watch_usb_cdc.h"
#include "tcc.h"

uint8_t ticks = 0;
bool beep = false;

void cb_tick(void);
void cb_mode_btn_interrupt(void);
void cb_light_btn_interrupt(void);
void cb_alarm_btn_extwake(void);

void yield(void) {
    tud_task();
    cdc_task();
}

void app_init(void) {
    // initialize the watch hardware.
    _watch_init();

    // check if we are plugged into USB power.
    HAL_GPIO_VBUS_DET_in();
    HAL_GPIO_VBUS_DET_pulldown();
    if (HAL_GPIO_VBUS_DET_read()){
        /// if so, enable USB functionality.
        _watch_enable_usb();
    }
    HAL_GPIO_VBUS_DET_off();
}

void app_setup(void) {
    watch_enable_leds();
    watch_enable_external_interrupts();

    HAL_GPIO_BTN_LIGHT_in();
    HAL_GPIO_BTN_LIGHT_pulldown();
    HAL_GPIO_BTN_LIGHT_pmuxen(HAL_GPIO_PMUX_EIC);
    HAL_GPIO_BTN_MODE_in();
    HAL_GPIO_BTN_MODE_pulldown();
    HAL_GPIO_BTN_MODE_pmuxen(HAL_GPIO_PMUX_EIC);

    // Simple test sketch exercises RTC and EIC, plus LEDs, screen and buzzer.
    // Periodic callback increments the tick counter.
    watch_rtc_register_periodic_callback(cb_tick, 1);
    // Light button turns on the LED.
    watch_register_interrupt_callback(HAL_GPIO_BTN_LIGHT_pin(), cb_light_btn_interrupt, INTERRUPT_TRIGGER_RISING);
    // Mode button beeps the piezo.
    watch_register_interrupt_callback(HAL_GPIO_BTN_MODE_pin(), cb_mode_btn_interrupt, INTERRUPT_TRIGGER_RISING);
    // Alarm buttton resets the tick counter.
    watch_register_extwake_callback(HAL_GPIO_BTN_ALARM_pin(), cb_alarm_btn_extwake, true);

    watch_enable_display();
    watch_display_top_left("WA");
    watch_display_top_right(" 0");
    watch_display_main_line(" test ");
}

bool app_loop(void) {
    if (usb_is_enabled()) {
        yield();
    }

    if (beep) {
        watch_buzzer_play_note(BUZZER_NOTE_C8, 100);
        beep = false;
    }

    char buf[4];
    sprintf(buf, "%2d", ticks);
    printf("ticks: %d\n", ticks);
    watch_display_top_right(buf);

    if (ticks >= 30) {
        watch_enter_sleep_mode();
    }

    return !usb_is_enabled();
}

void cb_tick(void) {
    ticks = ticks + 1;
    watch_set_led_off();
}

void cb_light_btn_interrupt(void) {
    watch_set_led_green();
}

void cb_mode_btn_interrupt(void) {
    beep = true;
}

void cb_alarm_btn_extwake(void) {
    watch_set_led_red();
    ticks = 0;
}