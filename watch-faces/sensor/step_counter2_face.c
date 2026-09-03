/*
 * MIT License
 *
 * Copyright (c) 2025, 2026 Konrad Rieck
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

#include <stdlib.h>
#include <string.h>
#include "step_counter2_face.h"
#include "watch.h"
#include "watch_utility.h"

/* Number of settings pages */
#define NUM_SETTINGS 6

/* Default settings.
 *
 * The values follow the sample rate configured in setup() and have to be
 * adjusted if it changes. They can all be changed at runtime on the settings
 * pages of the face.
 */
#define DEFAULT_DAILY_GOAL 10000
#define DEFAULT_THRESHOLD 94
#define DEFAULT_MIN_STEP 9
#define DEFAULT_MAX_STEP 30
#define DEFAULT_MIN_STREAK 7
#define DEFAULT_MAX_RISE 19

/* Sleep duration: how long the accelerometer must be motionless before its A4
 * pin flags "still". This is the debounced "stopped moving" signal that lets
 * background counting release the watch to sleep, so it must outlast any stride
 * gap and never fire mid-walk. Raw SLEEP_DUR register value; 1 LSB = 512 / ODR.
 * At 25 Hz, 9 is ~3 minutes (at 12.5 Hz it would be ~6 minutes). */
#define SLEEP_DURATION 9

/* Motion threshold: how hard the watch must move before the accelerometer
 * flags "active" on its A4 pin. Raw wake-up threshold register value;
 * 1 LSB = 2 g / 64, so 8 is ~0.25 g. */
#define MOTION_THRESHOLD 8

/* Debug logging. Enabled by the STEP_COUNTER2_DEBUG define below; comment it out
 * to compile the logging to nothing. Each line is prefixed with a
 * "[hh:mm:ss] SC " timestamp, so gaps in the once-per-second background log
 * reveal when the watch is asleep. */
// #define STEP_COUNTER2_DEBUG
#ifdef STEP_COUNTER2_DEBUG
#define dprintf(fmt, ...) do { \
    watch_date_time_t _now = movement_get_local_date_time(); \
    printf("[%02d:%02d:%02d] SC " fmt "\n", \
           _now.unit.hour, _now.unit.minute, _now.unit.second, ##__VA_ARGS__); \
} while (0)
#else
#define dprintf(fmt, ...) do { } while (0)
#endif

static inline void _beep()
{
    if (!movement_button_should_sound())
        return;
    watch_buzzer_play_note(BUZZER_NOTE_C7, 50);
}

static void _settings_title_display(step_counter2_state_t *state, char *buf1, char *buf2)
{
    char buf[10];
    watch_display_text_with_fallback(WATCH_POSITION_TOP, buf1, buf2);
    if (watch_get_lcd_type() != WATCH_LCD_TYPE_CUSTOM) {
        snprintf(buf, sizeof(buf), "%2d", state->settings_page + 1);
        watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, buf, buf);
    }
}

static bool _settings_blink(uint8_t subsecond)
{
    if (subsecond % 2 == 0) {
        watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "      ", "      ");
        return true;
    }
    return false;
}

static void _settings_daily_goal_display(void *context, uint8_t subsecond)
{
    char buf[10];
    step_counter2_state_t *state = (step_counter2_state_t *) context;

    _settings_title_display(state, "GOAL ", "GO");
    if (_settings_blink(subsecond))
        return;

    snprintf(buf, sizeof(buf), "%5d ", state->daily_goal);
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
}

static void _settings_daily_goal_advance(void *context)
{
    step_counter2_state_t *state = (step_counter2_state_t *) context;
    /* Goals of 1000..20000 steps, in steps of 1000. */
    state->daily_goal += 1000;

    if (state->daily_goal > 20000) {
        state->daily_goal = 1000;
    }
}

static void _settings_threshold_display(void *context, uint8_t subsecond)
{
    char buf[10];
    step_counter2_state_t *state = (step_counter2_state_t *) context;

    _settings_title_display(state, "THRES", "TH");
    if (_settings_blink(subsecond))
        return;

    snprintf(buf, sizeof(buf), "%4d  ", state->threshold);
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
}

static void _settings_threshold_advance(void *context)
{
    step_counter2_state_t *state = (step_counter2_state_t *) context;
    state->threshold++;

    if (state->threshold > 120) {
        state->threshold = 80;
    }
}

static void _settings_min_step_display(void *context, uint8_t subsecond)
{
    char buf[10];
    step_counter2_state_t *state = (step_counter2_state_t *) context;

    _settings_title_display(state, "MINST", "MI");
    if (_settings_blink(subsecond))
        return;

    snprintf(buf, sizeof(buf), "%4d  ", state->min_step);
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
}

static void _settings_min_step_advance(void *context)
{
    step_counter2_state_t *state = (step_counter2_state_t *) context;
    state->min_step++;

    if (state->min_step > 20) {
        state->min_step = 0;
    }
}

static void _settings_max_step_display(void *context, uint8_t subsecond)
{
    char buf[10];
    step_counter2_state_t *state = (step_counter2_state_t *) context;

    _settings_title_display(state, "MAXST", "MA");
    if (_settings_blink(subsecond))
        return;

    snprintf(buf, sizeof(buf), "%4d  ", state->max_step);
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
}

static void _settings_max_step_advance(void *context)
{
    step_counter2_state_t *state = (step_counter2_state_t *) context;
    state->max_step++;

    if (state->max_step > 60) {
        state->max_step = state->min_step;
    }
}

static void _settings_min_streak_display(void *context, uint8_t subsecond)
{
    char buf[10];
    step_counter2_state_t *state = (step_counter2_state_t *) context;

    _settings_title_display(state, "STREK", "ST");
    if (_settings_blink(subsecond))
        return;

    snprintf(buf, sizeof(buf), "%4d  ", state->min_streak);
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
}

static void _settings_min_streak_advance(void *context)
{
    step_counter2_state_t *state = (step_counter2_state_t *) context;
    state->min_streak++;

    /* Streaks of 1..10 steps; 1 disables streak filtering. */
    if (state->min_streak > 10) {
        state->min_streak = 1;
    }
}

static void _settings_max_rise_display(void *context, uint8_t subsecond)
{
    char buf[10];
    step_counter2_state_t *state = (step_counter2_state_t *) context;

    _settings_title_display(state, "RISE", "RI");
    if (_settings_blink(subsecond))
        return;

    snprintf(buf, sizeof(buf), "%4d  ", state->max_rise);
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
}

static void _settings_max_rise_advance(void *context)
{
    step_counter2_state_t *state = (step_counter2_state_t *) context;
    state->max_rise++;

    /* Percent of the peak height. Measured medians are 10-14 for walking and
     * 16-26 for hand motion, so the useful range sits in between; 40 is loose
     * enough to pass essentially everything. */
    if (state->max_rise > 40) {
        state->max_rise = 5;
    }
}

static void _counter_display(step_counter2_state_t *state)
{
    char buf[12];
    uint8_t percent;

    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "STEPS", "SC");

    /* Display progress towards the daily goal in the date field, in tens of
     * percent, so that 10 is the goal reached. */
    percent = (state->steps * 10) / state->daily_goal;
    snprintf(buf, sizeof(buf), "%2d", percent);
    watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, buf, buf);

    /* Display step count */
    if (state->steps < 10000) {
        snprintf(buf, sizeof(buf), "%4lu  ", (unsigned long) state->steps);
    } else {
        snprintf(buf, sizeof(buf), "%6lu", (unsigned long) state->steps);
    }
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, buf, buf);
}

static void _switch_to_counter(step_counter2_state_t *state)
{
    /* Switch to counter page */
    movement_request_tick_frequency(1);
    state->page = PAGE_COUNTER;

    /* Clear display */
    watch_clear_colon();
    watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, "  ", "  ");

    _counter_display(state);
}

static void _switch_to_settings(step_counter2_state_t *state)
{
    /* Switch to settings page */
    movement_request_tick_frequency(4);
    watch_clear_colon();
    state->page = PAGE_SETTINGS;
    state->settings_page = 0;
    state->settings[state->settings_page].display(state, 0);
}

/* Reset state to default */
static void _reset_state(step_counter2_state_t *state)
{
    state->subticks = 0;
    state->steps = 0;
    state->last_step = 0;
    state->have_last_step = false;
    state->streak_len = 0;
    memset(state->mag_hist, 0, sizeof(state->mag_hist));
    state->mag_idx = 0;
}

/* Reset the count when the day rolls over. Keying on the day (not an exact
 * 00:00:00 match) survives a tick or drain that lands just after midnight. */
static void _daily_reset_check(step_counter2_state_t *state)
{
    watch_date_time_t now = movement_get_local_date_time();
    if (now.unit.day != state->last_reset_day) {
        _reset_state(state);
        state->last_reset_day = now.unit.day;
    }
}

/* Schedule one background FIFO drain for the given unix time and remember the
 * target. The chain keeps itself going: each EVENT_BACKGROUND_TASK re-arms for
 * the next second, so a 1 Hz drain runs while the face is off-screen. advise()
 * stops the chain when motion ends. */
static void _schedule_drain(step_counter2_state_t *state, uint32_t target)
{
    watch_date_time_t next = watch_utility_date_time_from_unix_time(target, 0);
    movement_schedule_background_task_for_face(state->watch_face_index, next);
    state->drain_target = target;
    state->background_armed = true;
}

/* Stop the background drain chain. */
static void _cancel_background(step_counter2_state_t *state)
{
    movement_cancel_background_task_for_face(state->watch_face_index);
    state->background_armed = false;
}

/* Approximate the vector length (L2 norm) of a reading */
static uint32_t _approx_l2_norm(lis2dw_reading_t reading)
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

/* Print lis2dw status to console. */
static void _lis2dw_print_state(void)
{
    dprintf("lis2dw: mode=%x rate=%x lp=%x bw=%x range=%x filter=%x lownoise=%x",
            lis2dw_get_mode(), lis2dw_get_data_rate(), lis2dw_get_low_power_mode(),
            lis2dw_get_bandwidth_filtering(), lis2dw_get_range(),
            lis2dw_get_filter_type(), lis2dw_get_low_noise_mode());
}

/* Steepest climb over the last STEP_COUNTER2_RISE_WIN samples.
 *
 * The wrist tells walking from housework by the shape of a peak, not by its
 * timing: an arm swing is a smooth pendulum, so the climb into a heel strike
 * stays shallow next to the peak it reaches, while the jerky movements of
 * dressing or tying laces rise much faster. Measured over the recordings, the
 * median climb is 10-14 percent of the peak height for every walking pace and
 * 16-26 percent for hand motion.
 *
 * The window is a time and not a sample count. Its optimum sits at 200 ms and
 * holds over roughly 120-280 ms, and it does not move when the other
 * parameters are varied, so it follows the sample rate alone. At 25 Hz that is
 * the 7 samples of STEP_COUNTER2_RISE_WIN. Sampled slower it stops working:
 * decimated to 12.5 Hz the separation disappears at every window size, since
 * a 200 ms rise leaves only two or three samples to measure.
 */
static uint8_t _peak_rise(step_counter2_state_t *state)
{
    /* mag_idx is the next write position, so it is also the oldest sample. */
    uint8_t prev = state->mag_hist[state->mag_idx];
    uint8_t best = 0;

    for (uint8_t k = 1; k < STEP_COUNTER2_RISE_HIST; k++) {
        uint8_t cur = state->mag_hist[(state->mag_idx + k) % STEP_COUNTER2_RISE_HIST];
        /* Rises only; the comparison also guards the unsigned subtraction. */
        if (cur > prev && (uint8_t) (cur - prev) > best)
            best = cur - prev;
        prev = cur;
    }

    return best;
}

static void _detect_steps(step_counter2_state_t *state)
{
    lis2dw_fifo_t fifo;

    lis2dw_read_fifo(&fifo);
    for (uint8_t i = 0; i < fifo.count; i++) {
        /* Calculate magnitude of acceleration */
        uint32_t mag = _approx_l2_norm(fifo.readings[i]);
        /* Clamp magnitude to 16 bits and scale down to 8 bits */
        mag = (mag > 0xffff) ? 0xffff : mag;
        mag >>= 8;

        /* Record every sample, including the ones skipped below: the rise gate
         * needs an uninterrupted history to measure the climb against. The
         * clamp is redundant while mag is derived as above, but truncating a
         * saturated peak instead of clamping it would read as a near-full-scale
         * rise and silently suppress the steps that follow it. */
        state->mag_hist[state->mag_idx] = (mag > 0xff) ? 0xff : (uint8_t) mag;
        state->mag_idx = (state->mag_idx + 1) % STEP_COUNTER2_RISE_HIST;

        /* Only threshold crossings are candidate steps */
        if (mag <= state->threshold)
            goto skip;

        /* Enforce the minimum time between steps: a crossing that arrives too
         * soon after the previous step is ignored. */
        if (state->have_last_step && state->subticks - state->last_step < state->min_step)
            goto skip;

        /* Reject peaks that rise too abruptly to be a stride. Integer form of
         * rise / mag > max_rise / 100. Until the history has filled after a
         * reset the buffer still holds zeros, which reads as a steep rise and
         * drops the first few candidates; that is the safe direction. */
        if (_peak_rise(state) * 100 > state->max_rise * mag) {
            dprintf("peak rejected (rise=%u, mag=%lu)", _peak_rise(state), (unsigned long) mag);
            goto skip;
        }

        /* Decide whether this step continues the current rhythmic streak or
         * starts a new one. A streak continues while the gap to the previous
         * step stays within max_step. */
        if (state->have_last_step && state->subticks - state->last_step <= state->max_step) {
            state->streak_len++;
        } else {
            /* Rhythm broken (or first step): a previous streak shorter than
             * min_streak was never counted, so nothing needs to be undone. */
            state->streak_len = 1;
        }
        dprintf("potential step (steps=%lu, streak=%u)", (unsigned long) state->steps, state->streak_len);

        /* Confirm steps once the streak reaches the required length. When the
         * streak first reaches min_streak, the whole streak becomes valid at once;
         * every further step in the streak is confirmed individually. This
         * matches ThresholdBoundN: streaks shorter than min_streak are dropped. */
        if (state->streak_len == state->min_streak) {
            state->steps += state->min_streak;
            dprintf("%u steps detected (steps=%lu)", state->min_streak, (unsigned long) state->steps);
        } else if (state->streak_len > state->min_streak) {
            state->steps++;
            dprintf("1 step detected (steps=%lu)", (unsigned long) state->steps);
        }

        state->have_last_step = true;
        state->last_step = state->subticks;

      skip:
        /* Increment subticks */
        state->subticks += 1;
    }
    lis2dw_clear_fifo();
}

static bool _counter_loop(movement_event_t event, void *context)
{
    step_counter2_state_t *state = (step_counter2_state_t *) context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _counter_display(state);
            break;
        case EVENT_TICK:
            /* Foreground: the 1 Hz tick drives detection and display. The
             * background drain chain is cancelled while we are on screen
             * (see activate), so exactly one drain happens per second. */
            _daily_reset_check(state);
            _detect_steps(state);
            _counter_display(state);
            break;
        case EVENT_LIGHT_LONG_PRESS:
            /* Long-press LIGHT enters settings (like the hydration face) */
            _switch_to_settings(state);
            _beep();
            break;
        case EVENT_ALARM_LONG_PRESS:
            /* Long-press ALARM resets the step count */
            _reset_state(state);
            _counter_display(state);
            _beep();
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

static bool _settings_loop(movement_event_t event, void *context)
{
    step_counter2_state_t *state = (step_counter2_state_t *) context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            state->settings[state->settings_page].display(context, event.subsecond);
            break;
        case EVENT_LIGHT_BUTTON_UP:
            /* Go to next settings page */
            state->settings_page = (state->settings_page + 1) % NUM_SETTINGS;
            state->settings[state->settings_page].display(context, event.subsecond);
            _beep();
            break;
        case EVENT_ALARM_BUTTON_UP:
            /* Advance current settings */
            state->settings[state->settings_page].advance(context);
            state->settings[state->settings_page].display(context, event.subsecond);
            break;
        case EVENT_ALARM_LONG_PRESS:
            /* Reset the current setting to its default value */
            switch (state->settings_page) {
                case STEP_COUNTER2_SETTING_DAILY_GOAL:
                    state->daily_goal = DEFAULT_DAILY_GOAL;
                    break;
                case STEP_COUNTER2_SETTING_THRESHOLD:
                    state->threshold = DEFAULT_THRESHOLD;
                    break;
                case STEP_COUNTER2_SETTING_MIN_STEP:
                    state->min_step = DEFAULT_MIN_STEP;
                    break;
                case STEP_COUNTER2_SETTING_MAX_STEP:
                    state->max_step = DEFAULT_MAX_STEP;
                    break;
                case STEP_COUNTER2_SETTING_MIN_STREAK:
                    state->min_streak = DEFAULT_MIN_STREAK;
                    break;
                case STEP_COUNTER2_SETTING_MAX_RISE:
                    state->max_rise = DEFAULT_MAX_RISE;
                    break;
            }
            state->settings[state->settings_page].display(context, event.subsecond);
            _beep();
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            /* Do nothing. */
            break;
        case EVENT_MODE_BUTTON_UP:
            /* Exit settings and return to step counter (count is preserved) */
            _switch_to_counter(state);
            _beep();
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }
    return true;
}

void step_counter2_face_setup(uint8_t watch_face_index, void **context_ptr)
{
    step_counter2_state_t *state;

    /* Initialize state */
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(step_counter2_state_t));
        memset(*context_ptr, 0, sizeof(step_counter2_state_t));
        state = (step_counter2_state_t *) *context_ptr;

        /* Default setup */
        state->daily_goal = DEFAULT_DAILY_GOAL;
        state->threshold = DEFAULT_THRESHOLD;
        state->min_step = DEFAULT_MIN_STEP;
        state->max_step = DEFAULT_MAX_STEP;
        state->min_streak = DEFAULT_MIN_STREAK;
        state->max_rise = DEFAULT_MAX_RISE;

        /* Reset state */
        _reset_state(state);
    }

    /* Initialize settings */
    state = (step_counter2_state_t *) *context_ptr;
    if (state->settings == NULL) {
        uint8_t settings_page = 0;
        state->settings = malloc(NUM_SETTINGS * sizeof(step_counter2_settings_t));
        state->settings[settings_page].display = _settings_daily_goal_display;
        state->settings[settings_page].advance = _settings_daily_goal_advance;
        settings_page++;
        state->settings[settings_page].display = _settings_threshold_display;
        state->settings[settings_page].advance = _settings_threshold_advance;
        settings_page++;
        state->settings[settings_page].display = _settings_min_step_display;
        state->settings[settings_page].advance = _settings_min_step_advance;
        settings_page++;
        state->settings[settings_page].display = _settings_max_step_display;
        state->settings[settings_page].advance = _settings_max_step_advance;
        settings_page++;
        state->settings[settings_page].display = _settings_min_streak_display;
        state->settings[settings_page].advance = _settings_min_streak_advance;
        settings_page++;
        state->settings[settings_page].display = _settings_max_rise_display;
        state->settings[settings_page].advance = _settings_max_rise_advance;
        settings_page++;
    }

    /* Remember our index so we can schedule background drains for ourselves. */
    state->watch_face_index = watch_face_index;

    /* Set up accelerometer: 25 Hz background sensing for step detection. */
    movement_set_accelerometer_background_rate(LIS2DW_DATA_RATE_25_HZ);
    lis2dw_set_bandwidth_filtering(LIS2DW_BANDWIDTH_FILTER_DIV4);

    /* Enable wake-on-motion so the watch wakes up as soon as it moves again
     * after going to sleep. Counting then resumes on wake; steps are never
     * counted while the watch is asleep. Set the motion threshold and turn the
     * feature on. This needs the wake-on-motion PR, which defines
     * MOVEMENT_HAS_WAKE_ON_MOTION in movement.h. Without it the watch does not
     * wake on motion, so movement that starts during sleep is missed until the
     * watch wakes for some other reason. */
#ifdef MOVEMENT_HAS_WAKE_ON_MOTION
    movement_set_accelerometer_motion_threshold(MOTION_THRESHOLD);
    movement_set_wake_on_motion(true);
#endif

    /* Debounce the A4 still/active signal so it only flags "still" after a few
     * minutes of no motion (see SLEEP_DURATION). advise() uses it to decide when
     * to release the watch to sleep. */
    lis2dw_configure_sleep_duration(SLEEP_DURATION);
    _lis2dw_print_state();

    /* Enable fifo and clear it. */
    lis2dw_enable_fifo();
    lis2dw_clear_fifo();

    /* Launch the background drain chain so counting runs from power-on even if
     * the face is never opened. If we boot into the foreground, activate()
     * immediately cancels it and the 1 Hz tick takes over. */
    _schedule_drain(state, movement_get_utc_timestamp() + 1);
    dprintf("setup (index=%d, armed chain)", state->watch_face_index);
}

void step_counter2_face_activate(void *context)
{
    step_counter2_state_t *state = (step_counter2_state_t *) context;

    /* Entering the foreground: the 1 Hz tick will drive detection, so stop the
     * background chain to avoid draining the FIFO twice per second. */
    state->is_foreground = true;
    _cancel_background(state);
    dprintf("activate (foreground, cancelled chain)");

    /* Switch to counter page. */
    _switch_to_counter(state);
}

bool step_counter2_face_loop(movement_event_t event, void *context)
{
    step_counter2_state_t *state = (step_counter2_state_t *) context;

    /* Background drain: delivered even when another face is on screen. Detect
     * only (we may not be in the foreground, so never touch the display), then
     * re-arm the chain for the next second. */
    if (event.event_type == EVENT_BACKGROUND_TASK) {
        /* Advance the stored target by exactly 1 for a steady 1 s spacing.
         * Do NOT recompute the target from the clock: movement_get_utc_timestamp()
         * rounds to the nearest second and can already report the next second
         * here, which pushes the spacing to 2 s. Guard against a past/equal
         * target: the scheduler's comparison is strict (target.reg > now.reg), so
         * it would silently drop it and the chain would hang. */
        uint32_t fired = state->drain_target;   /* the second this drain fires for */
        uint32_t now = movement_get_utc_timestamp();
        uint32_t target = fired + 1;
        if (target <= now)
            target = now + 1;
        _schedule_drain(state, target);

#ifdef STEP_COUNTER2_DEBUG
        /* Measure the drain cost with the 128 Hz RTC counter (~7.8 ms/tick). */
        rtc_counter_t t0 = watch_rtc_get_counter();
#endif
        _daily_reset_check(state);
        _detect_steps(state);
#ifdef STEP_COUNTER2_DEBUG
        uint32_t dur = watch_rtc_get_counter() - t0;
        int32_t tz = movement_get_current_timezone_offset();
        /* Timestamp this line from the second the drain fired for (an exact
         * value), not the wall clock, so it and next differ by exactly 1 s. */
        watch_date_time_t ft = watch_utility_date_time_from_unix_time(fired, tz);
        watch_date_time_t nt = watch_utility_date_time_from_unix_time(target, tz);
        printf("[%02d:%02d:%02d] SC scheduled drain (steps=%lu, duration=%lu.%02lus, next=%02d:%02d:%02d)\n",
               ft.unit.hour, ft.unit.minute, ft.unit.second,
               (unsigned long) state->steps, (unsigned long) (dur / 128), (unsigned long) ((dur % 128) * 100 / 128),
               nt.unit.hour, nt.unit.minute, nt.unit.second);
#endif
        return true;
    }

    switch (state->page) {
        default:
        case PAGE_COUNTER:
            return _counter_loop(event, context);
        case PAGE_SETTINGS:
            return _settings_loop(event, context);
    }
}

void step_counter2_face_resign(void *context)
{
    step_counter2_state_t *state = (step_counter2_state_t *) context;

    /* The step counter intentionally keeps owning the accelerometer (25 Hz +
     * wake-on-motion) after resign so it keeps counting in the background. Arm
     * the background drain chain to take over from the foreground tick. advise()
     * will stop it once motion ends. */
    state->is_foreground = false;
    _schedule_drain(state, movement_get_utc_timestamp() + 1);
    dprintf("resign (background, armed chain)");
}

movement_watch_face_advisory_t step_counter2_face_advise(void *context)
{
    step_counter2_state_t *state = (step_counter2_state_t *) context;
    movement_watch_face_advisory_t retval = { 0 };

    /* In the foreground the 1 Hz tick drives detection; leave the chain alone. */
    if (state->is_foreground)
        return retval;

    /* A4 outputs the accelerometer's sleep state: HIGH when still, LOW when
     * moving. Poll it once a minute and switch the background drain on motion:
     *  - moving and not armed -> start counting.
     *  - still and armed -> stop, so the watch's low-energy timeout can sleep it.
     * We arm by scheduling a task (never wants_background_task), so no FIFO drain
     * can ever fire from inside deep sleep. Renewed motion is caught by the A4
     * wake interrupt, which wakes the watch before the next poll. */
    bool moving = !HAL_GPIO_A4_read();
    if (moving && !state->background_armed) {
        _schedule_drain(state, movement_get_utc_timestamp() + 1);
    } else if (!moving && state->background_armed) {
        _cancel_background(state);
    }
    dprintf("advise (motion=%s, schedule=%s)", moving ? "active" : "still", state->background_armed ? "yes" : "no");

    return retval;
}
