#include "app.h"
#include "watch.h"
#include "watch_private.h"
#include "delay.h"

void app_init(void) {
    _watch_rtc_init();
}

void app_setup(void) {
    watch_enable_leds();
    watch_rtc_register_periodic_callback(NULL, 1);
}

bool app_loop(void) {
    watch_date_time date_time = watch_rtc_get_date_time();

    if (date_time.unit.second % 2 == 0) {
        watch_set_led_red();
    } else {
        watch_set_led_green();
    }

    return true;
}