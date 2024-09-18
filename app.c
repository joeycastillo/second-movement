#include "app.h"
#include "watch.h"
#include "delay.h"

void app_init(void) {
}

void app_setup(void) {
    watch_enable_digital_output(HAL_GPIO_GREEN_pin());
}

bool app_loop(void) {
    watch_set_pin_level(HAL_GPIO_GREEN_pin(), true);
    delay_ms(500);
    watch_set_pin_level(HAL_GPIO_GREEN_pin(), false);
    delay_ms(500);

    return false;
}