#include "count_steps.h"
#include "stdint.h"
#include "stdio.h"  //using this for printing debug outputs
#include <stdlib.h>

#define DEBUG_PRINT false

#define SIMPLE_THRESHOLD               2000  // Magnitudes at or above this threshold are considered a step, but can change if USE_WINDOW_AVG is true
#define SIMPLE_THRESHOLD_MULT           1.3 // Multiplier for the moving average threshold adjustment. It was seen in some testing that 50% higher than the average worked well.
#define SIMPLE_SAMP_IGNORE_STEP         3   // After detecting a step, ignore this many samples to avoid double counting
#define USE_WINDOW_AVG                  true   // If true, the step detection threshold will be adjusted based on a moving average of the signal magnitude
#define AVG_WINDOW_SIZE_SHIFT           7  // The size of the moving average window. We are using bitshifting to keep the math efficient since we run once a second
#define AVG_WINDOW_SIZE                 ((1 << AVG_WINDOW_SIZE_SHIFT) - 1)
#define MAX_FIFO_SIZE_SIMPLE            13
#define MAX_SIMPLE_STEPS                (MAX_FIFO_SIZE_SIMPLE / SIMPLE_SAMP_IGNORE_STEP)


static uint32_t step_counter_threshold = SIMPLE_THRESHOLD;

/* Approximate l2 norm */
uint32_t count_steps_approx_l2_norm(lis2dw_reading_t reading)
{
    /* Absolute values */
    uint32_t ax = abs(reading.x);
    uint32_t ay = abs(reading.y);
    uint32_t az = abs(reading.z);

    /* *INDENT-OFF* */
    /* Sort values: ax >= ay >= az */
    if (ax < ay) { uint32_t t = ax; ax = ay; ay = t; }
    if (ay < az) { uint32_t t = ay; ay = az; az = t; }
    if (ax < ay) { uint32_t t = ax; ax = ay; ay = t; }
    /* *INDENT-ON* */

    /* Approximate sqrt(x^2 + y^2 + z^2) */
    /* alpha ≈ 0.9375 (15/16), beta ≈ 0.375 (3/8) */
    return ax + ((15 * ay) >> 4) + ((3 * az) >> 3);
}


uint8_t count_steps_simple(lis2dw_fifo_t *fifo_data) {
    uint8_t new_steps = 0;
#if USE_WINDOW_AVG
    uint8_t samples_processed = 0;
    uint32_t samples_sum = 0;
#endif
#if DEBUG_PRINT
    printf("fifo counts: %d\r\n", fifo_data->count);
#endif
    for (uint8_t i = 0; i < fifo_data->count; i++) {
        if (i >= MAX_FIFO_SIZE_SIMPLE) break;
        uint32_t magnitude = count_steps_approx_l2_norm(fifo_data->readings[i]) >> 3;
#if DEBUG_PRINT
        printf("%lu; ", magnitude);
#endif
        if (magnitude == 0) {
          // We ignore readings where everything is zero, because that's almost definetly a misread
          continue;
        }
        if (magnitude >= step_counter_threshold) {
            new_steps += 1;
            i += SIMPLE_SAMP_IGNORE_STEP;
        }
#if USE_WINDOW_AVG
        samples_processed += 1;
        samples_sum += magnitude;
#endif
    }
#if USE_WINDOW_AVG
    if (samples_processed > 0) {
        samples_sum /= samples_processed;
        samples_sum *= SIMPLE_THRESHOLD_MULT;
        step_counter_threshold = ((step_counter_threshold * AVG_WINDOW_SIZE) + samples_sum) >> AVG_WINDOW_SIZE_SHIFT;
    }
#if DEBUG_PRINT
        printf("\r\nThreshold: %lu\r\n", step_counter_threshold);
        printf("New Steps: %d\r\n; ", new_steps);
#endif
#endif
    if (new_steps > MAX_SIMPLE_STEPS) new_steps = MAX_SIMPLE_STEPS;
    return new_steps;
}



// STEPCOUNT_CONFIGURABLE is for use with https://github.com/gfwilliams/step-count
#define ACCELFILTER_TAP_NUM 7

typedef struct {
  int8_t history[ACCELFILTER_TAP_NUM];
  unsigned int last_index;
} AccelFilter;

static const int8_t filter_taps[ACCELFILTER_TAP_NUM] = {
    -11, -15, 44, 68, 44, -15, -11
};


static void AccelFilter_init(AccelFilter* f) {
  int i;
  for(i = 0; i < ACCELFILTER_TAP_NUM; ++i)
    f->history[i] = 0;
  f->last_index = 0;
}

static void AccelFilter_put(AccelFilter* f, int8_t input) {
  f->history[f->last_index++] = input;
  if(f->last_index == ACCELFILTER_TAP_NUM)
    f->last_index = 0;
}

static int AccelFilter_get(AccelFilter* f) {
  int acc = 0;
  int index = f->last_index, i;
  for(i = 0; i < ACCELFILTER_TAP_NUM; ++i) {
    index = index != 0 ? index-1 : ACCELFILTER_TAP_NUM-1;
    acc += (int)f->history[index] * (int)filter_taps[i];
  };
  return acc >> 2;
}

AccelFilter accelFilter;

/* =============================================================
*  DC Filter
*/
#define NSAMPLE 12 //Exponential Moving Average DC removal filter alpha = 1/NSAMPLE

static int DCFilter_sample_avg_total = 8192*NSAMPLE;

static int DCFilter(int sample) {
    DCFilter_sample_avg_total += (sample - DCFilter_sample_avg_total/NSAMPLE);
    return sample - DCFilter_sample_avg_total/NSAMPLE;
}


// ===============================================================

// These were calculated based on contributed data
// using https://github.com/gfwilliams/step-count
#define STEPCOUNTERTHRESHOLD_DEFAULT  600

// These are the ranges of values we test over
// when calculating the best data offline
#define STEPCOUNTERTHRESHOLD_MIN 0
#define STEPCOUNTERTHRESHOLD_MAX 2000
#define STEPCOUNTERTHRESHOLD_STEP 100

// This is a bit of a hack to allow us to configure
// variables which would otherwise have been compiler defines
#ifdef STEPCOUNT_CONFIGURABLE
int stepCounterThreshold = STEPCOUNTERTHRESHOLD_DEFAULT;
// These are handy values used for graphing
int accScaled;
#else
#define stepCounterThreshold STEPCOUNTERTHRESHOLD_DEFAULT
#endif

// ===============================================================

/** stepHistory allows us to check for repeated step counts.
Rather than registering each instantaneous step, we now only
measure steps if there were at least 3 steps (including the
current one) in 3 seconds

For each step it contains the number of iterations ago it occurred. 255 is the maximum
*/

int16_t accFiltered; // last accel reading, after running through filter
int16_t accFilteredHist[2]; // last 2 accel readings, 1=newest

// ===============================================================

/**
 * (4) State Machine
 *
 * The state machine ensure all steps are checked that they fall
 * between T_MIN_STEP and T_MAX_STEP. The 2v9.90 firmare uses X steps
 * in Y seconds but this just enforces that the step X steps ago was
 * within 6 seconds (75 samples).  It is possible to have 4 steps
 * within 1 second and then not get the 5th until T5 seconds.  This
 * could mean that the F/W would would be letting through 2 batches
 * of steps that actually would not meet the threshold as the step at
 * T5 could be the last.  The F/W version also does not give back the
 * X steps detected whilst it is waiting for X steps in Y seconds.
 * After 100 cycles of the algorithm this would amount to 500 steps
 * which is a 5% error over 10K steps.  In practice the number of
 * passes through the step machine from STEP_1 state to STEPPING
 * state can be as high as 500 events.  So using the state machine
 * approach avoids this source of error.
 *
 */

typedef enum {
  S_STILL = 0,       // just created state m/c no steps yet
  S_STEP_1 = 1,      // first step recorded
  S_STEP_22N = 2,    // counting 2-X steps
  S_STEPPING = 3,    // we've had X steps in X seconds
} StepState;

// In periods of 12.5Hz
#define T_MIN_STEP 4 // ~333ms
#define T_MAX_STEP 16 // ~1300ms
#define X_STEPS 6 // steps in a row needed
#define RAW_THRESHOLD 15
#define N_ACTIVE_SAMPLES 3

StepState stepState;
unsigned char holdSteps; // how many steps are we holding back?
unsigned char stepLength; // how many poll intervals since the last step?
int active_sample_count = 0;
bool gate_open = false;        // start closed
// ===============================================================

// Init step count
void count_steps_espruino_init() {
  AccelFilter_init(&accelFilter);
  DCFilter_sample_avg_total = 8192*NSAMPLE;
  accFiltered = 0;
  accFilteredHist[0] = 0;
  accFilteredHist[1] = 0;
  stepState = S_STILL;
  holdSteps = 0;
  stepLength = 0;
}

static int stepcount_had_step() {
  StepState st = stepState;

  switch (st) {
  case S_STILL:
    stepState = S_STEP_1;
    holdSteps = 1;
    return 0;

  case S_STEP_1:
    holdSteps = 1;
    // we got a step within expected period
    if (stepLength <= T_MAX_STEP && stepLength >= T_MIN_STEP) {
      //stepDebug("  S_STEP_1 -> S_STEP_22N");
      stepState = S_STEP_22N;
      holdSteps = 2;
    } else {
      // we stay in STEP_1 state
      //stepDebug("  S_STEP_1 -> S_STEP_1");
      //reject_count++;
    }
    return 0;

  case S_STEP_22N:
    // we got a step within expected time range
    if (stepLength <= T_MAX_STEP && stepLength >= T_MIN_STEP) {
      holdSteps++;

      if (holdSteps >= X_STEPS) {
        stepState = S_STEPPING;
        //pass_count++;  // we are going to STEPPING STATE
        //stepDebug("  S_STEP_22N -> S_STEPPING");
        return X_STEPS;
      }

      //stepDebug("  S_STEP_22N -> S_STEP_22N");
    } else {
      // we did not get the step in time, back to STEP_1
      stepState = S_STEP_1;
      //stepDebug("  S_STEP_22N -> S_STEP_1");
      //reject_count++;
    }
    return 0;

  case S_STEPPING:
    // we got a step within the expected window
    if (stepLength <= T_MAX_STEP && stepLength >= T_MIN_STEP) {
      stepState = S_STEPPING;
      //stepDebug("  S_STEPPING -> S_STEPPING");
      return 1;
    } else {
      // we did not get the step in time, back to STEP_1
      stepState = S_STEP_1;
      //stepDebug("  S_STEPPING -> S_STEP_1");
      //reject_count++;
    }
    return 0;
  }

  // should never get here
  //stepDebug("  ERROR");
  return 0;
}

// do calculations
uint8_t count_steps_espruino_sample(uint32_t accMag) {
  // scale to fit and clip
  //int v = (accMag-8192)>>5;
  int v = DCFilter(accMag)>>5;
#if DEBUG_PRINT
  printf("v %d\r\n",v);
  if (v>127 || v<-128) printf("Out of range %d\r\n", v);
#endif
  if (v>127) v = 127;
  if (v<-128) v = -128;
#ifdef STEPCOUNT_CONFIGURABLE
  accScaled = v;
#endif

  // do filtering
  AccelFilter_put(&accelFilter, v);
  accFilteredHist[0] = accFilteredHist[1];
  accFilteredHist[1] = accFiltered;
  int a = AccelFilter_get(&accelFilter);
  if (a>32767) a=32767;
  if (a<-32768) a=32768;
  accFiltered = a;

  if (v > RAW_THRESHOLD || v < -1*RAW_THRESHOLD) {
    if (active_sample_count < N_ACTIVE_SAMPLES)
      active_sample_count++;
    if (active_sample_count == N_ACTIVE_SAMPLES)
      gate_open = true;
  } else {
    if (active_sample_count > 0)
      active_sample_count--;
    if (active_sample_count == 0)
      gate_open = false;
  }

  // increment step count history counters
  if (stepLength<255)
    stepLength++;

  int stepsCounted = 0;
  // check for a peak in the last sample - in which case call the state machine
  if (gate_open == true && accFilteredHist[1] > accFilteredHist[0] && accFilteredHist[1] > accFiltered) {
    // We now have something resembling a step!
    stepsCounted = stepcount_had_step();
    stepLength = 0;
  }

  return stepsCounted;
}


uint8_t count_steps_espruino(lis2dw_fifo_t *fifo_data) {
    uint8_t new_steps = 0;
    // Fifo data is 12 bit @ 4G, or 1.952mg/LSB. The 3 LSB are also zero and need to be bitshifted.
    // So 1g * (1LSB / 1.952) = 512. 512 << 3 = 4096
    // This code wants 8192 LSB=1g, we just need to bitshift left once to get them to match
    // This can be done by shifting the output of count_steps_approx_l2_norm left once

    for (uint8_t i = 0; i < fifo_data->count; i++) {
        uint32_t magnitude = count_steps_approx_l2_norm(fifo_data->readings[i]);
        if (magnitude == 0) {
          // We ignore readings where everything is zero, because that's almost definetly a misread
          continue;
        }
        new_steps += count_steps_espruino_sample(magnitude << 1);
    }

    if (new_steps > MAX_SIMPLE_STEPS) new_steps = MAX_SIMPLE_STEPS;
    return new_steps;
}


