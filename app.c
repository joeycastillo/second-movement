#include "app.h"
#include "watch.h"
#include "delay.h"

void app_init(void) {
}

void app_setup(void) {
    watch_enable_leds();
    watch_enable_display();
}

bool app_loop(void) {
    watch_display_main_line("123456");

    return true;
}