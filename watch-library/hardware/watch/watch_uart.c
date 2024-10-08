/*
 * MIT License
 *
 * Copyright (c) 2020-2024 Joey Castillo
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

#include "watch_uart.h"
#include "uart.h"
#include "usb.h"
#include <string.h>

void watch_enable_uart(const uint16_t tx_pin, const uint16_t rx_pin, uint32_t baud) {
    uart_rxpo_t rxpo = UART_RXPO_NONE;

    if (rx_pin == HAL_GPIO_A1_pin()) {
        rxpo = UART_RXPO_3;
        HAL_GPIO_A1_in();
        HAL_GPIO_A1_pmuxen(HAL_GPIO_PMUX_SERCOM);
    } else if (rx_pin == HAL_GPIO_A2_pin()) {
        rxpo = UART_RXPO_0;
        HAL_GPIO_A2_in();
        HAL_GPIO_A2_pmuxen(HAL_GPIO_PMUX_SERCOM);
    } else if (rx_pin == HAL_GPIO_A3_pin()) {
        rxpo = UART_RXPO_1;
        HAL_GPIO_A3_in();
        HAL_GPIO_A3_pmuxen(HAL_GPIO_PMUX_SERCOM);
    } else if (rx_pin == HAL_GPIO_A4_pin()) {
        rxpo = UART_RXPO_2;
        HAL_GPIO_A4_in();
        HAL_GPIO_A4_pmuxen(HAL_GPIO_PMUX_SERCOM);
    }

    if (tx_pin == HAL_GPIO_A2_pin()) {
        uart_init_instance(3, UART_TXPO_0, rxpo, baud);
        HAL_GPIO_A2_pmuxen(HAL_GPIO_PMUX_SERCOM);
    } else if (tx_pin == HAL_GPIO_A4_pin()) {
        uart_init_instance(3, UART_TXPO_2, rxpo, baud);
        HAL_GPIO_A4_pmuxen(HAL_GPIO_PMUX_SERCOM);
    } else {
        uart_init_instance(3, UART_TXPO_NONE, rxpo, baud);
    }

    uart_enable_instance(3);
}

void watch_uart_puts(uint8_t *s) {
    uart_write_instance(3, s, strlen((const char *)s));
}

size_t watch_uart_gets(uint8_t *data, size_t max_length) {
    return uart_read_instance(3, data, max_length);
}

void irq_handler_sercom3(void);
void irq_handler_sercom3(void) {
    uart_irq_handler(3);
}