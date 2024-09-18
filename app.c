#include <stdio.h>
#include "app.h"
#include "watch.h"
#include "watch_private.h"
#include "delay.h"
#include "usb.h"
#include "tusb.h"
#include "watch_usb_cdc.h"

void yield(void) {
    tud_task();
    cdc_task();
}

void app_init(void) {
    // perform watch initialization first!
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
    watch_enable_adc();
    watch_enable_display();
}

bool app_loop(void) {
    uint16_t vcc = watch_get_vcc_voltage();
    char buf[7];
    snprintf(buf, 7, "%6d", vcc);
    watch_display_main_line(buf);
    printf("VCC: %d\n", vcc);

    if (usb_is_enabled()) {
        yield();
    }

    return false;
}