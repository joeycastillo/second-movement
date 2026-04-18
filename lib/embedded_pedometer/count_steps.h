#ifndef count_steps_h
#define count_steps_h
#include "stdint.h"
#include "lis2dw.h"

#define COUNT_STEPS_USE_ESPRUINO   true

uint32_t count_steps_approx_l2_norm(lis2dw_reading_t reading);
uint8_t count_steps_simple(lis2dw_fifo_t *fifo_data);

/// Initialise step counting
void count_steps_espruino_init(void);

/* Registers a new data point for step counting. Data is expected
 * as 12.5Hz, 8192=1g, and accMagSquared = sqrt(x*x + y*y + z*z)
 *
 * Returns the number of steps counted for this accel interval
 */

uint8_t count_steps_espruino_sample(uint32_t accMag);
uint8_t count_steps_espruino(lis2dw_fifo_t *fifo_data);
#endif /* count_steps_h */