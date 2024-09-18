#include "app.h"
#include "watch.h"
#include "delay.h"

void app_init(void) {
}

void app_setup(void) {
    watch_enable_leds();
}

bool app_loop(void) {
    watch_set_led_yellow();
    delay_ms(500);
    watch_set_led_off();
    delay_ms(500);

    return false;
}