#include <stdio.h>
#include "app.h"
#include "watch.h"
#include "delay.h"

void app_init(void) {
}

void app_setup(void) {
    watch_enable_adc();
    watch_enable_display();
}

bool app_loop(void) {
    uint16_t vcc = watch_get_vcc_voltage();
    char buf[7];
    snprintf(buf, 7, "%6d", vcc);
    watch_display_main_line(buf);

    return false;
}