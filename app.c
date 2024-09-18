#include "app.h"
#include "watch.h"
#include "watch_private.h"
#include "delay.h"

void app_init(void) {
}

void app_setup(void) {
    watch_enable_leds();
    watch_enable_external_interrupts();
    watch_register_interrupt_callback(HAL_GPIO_BTN_LIGHT_pin(), NULL, INTERRUPT_TRIGGER_FALLING);
    watch_register_interrupt_callback(HAL_GPIO_BTN_MODE_pin(), NULL, INTERRUPT_TRIGGER_FALLING);
    watch_register_interrupt_callback(HAL_GPIO_BTN_ALARM_pin(), NULL, INTERRUPT_TRIGGER_FALLING);
}

bool app_loop(void) {
    static bool on = false;

    if (on) {
        watch_set_led_off();
    } else {
        watch_set_led_yellow();
    }
    on = !on;

    return true;
}