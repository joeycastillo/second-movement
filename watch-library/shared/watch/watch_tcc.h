/*
 * MIT License
 *
 * Copyright (c) 2020 Joey Castillo
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

#pragma once

////< @file watch_buzzer.h

#include "watch.h"

/// @brief An enum for controlling the volume of the buzzer.
typedef enum {
    WATCH_BUZZER_VOLUME_SOFT = 0,
    WATCH_BUZZER_VOLUME_LOUD
} watch_buzzer_volume_t;

/// @brief 87 notes for use with watch_buzzer_play_note
typedef enum {
    BUZZER_NOTE_A1,              ///< 55.00 Hz
    BUZZER_NOTE_A1SHARP_B1FLAT,  ///< 58.27 Hz
    BUZZER_NOTE_B1,              ///< 61.74 Hz
    BUZZER_NOTE_C2,              ///< 65.41 Hz
    BUZZER_NOTE_C2SHARP_D2FLAT,  ///< 69.30 Hz
    BUZZER_NOTE_D2,              ///< 73.42 Hz
    BUZZER_NOTE_D2SHARP_E2FLAT,  ///< 77.78 Hz
    BUZZER_NOTE_E2,              ///< 82.41 Hz
    BUZZER_NOTE_F2,              ///< 87.31 Hz
    BUZZER_NOTE_F2SHARP_G2FLAT,  ///< 92.50 Hz
    BUZZER_NOTE_G2,              ///< 98.00 Hz
    BUZZER_NOTE_G2SHARP_A2FLAT,  ///< 103.83 Hz
    BUZZER_NOTE_A2,              ///< 110.00 Hz
    BUZZER_NOTE_A2SHARP_B2FLAT,  ///< 116.54 Hz
    BUZZER_NOTE_B2,              ///< 123.47 Hz
    BUZZER_NOTE_C3,              ///< 130.81 Hz
    BUZZER_NOTE_C3SHARP_D3FLAT,  ///< 138.59 Hz
    BUZZER_NOTE_D3,              ///< 146.83 Hz
    BUZZER_NOTE_D3SHARP_E3FLAT,  ///< 155.56 Hz
    BUZZER_NOTE_E3,              ///< 164.81 Hz
    BUZZER_NOTE_F3,              ///< 174.61 Hz
    BUZZER_NOTE_F3SHARP_G3FLAT,  ///< 185.00 Hz
    BUZZER_NOTE_G3,              ///< 196.00 Hz
    BUZZER_NOTE_G3SHARP_A3FLAT,  ///< 207.65 Hz
    BUZZER_NOTE_A3,              ///< 220.00 Hz
    BUZZER_NOTE_A3SHARP_B3FLAT,  ///< 233.08 Hz
    BUZZER_NOTE_B3,              ///< 246.94 Hz
    BUZZER_NOTE_C4,              ///< 261.63 Hz
    BUZZER_NOTE_C4SHARP_D4FLAT,  ///< 277.18 Hz
    BUZZER_NOTE_D4,              ///< 293.66 Hz
    BUZZER_NOTE_D4SHARP_E4FLAT,  ///< 311.13 Hz
    BUZZER_NOTE_E4,              ///< 329.63 Hz
    BUZZER_NOTE_F4,              ///< 349.23 Hz
    BUZZER_NOTE_F4SHARP_G4FLAT,  ///< 369.99 Hz
    BUZZER_NOTE_G4,              ///< 392.00 Hz
    BUZZER_NOTE_G4SHARP_A4FLAT,  ///< 415.30 Hz
    BUZZER_NOTE_A4,              ///< 440.00 Hz
    BUZZER_NOTE_A4SHARP_B4FLAT,  ///< 466.16 Hz
    BUZZER_NOTE_B4,              ///< 493.88 Hz
    BUZZER_NOTE_C5,              ///< 523.25 Hz
    BUZZER_NOTE_C5SHARP_D5FLAT,  ///< 554.37 Hz
    BUZZER_NOTE_D5,              ///< 587.33 Hz
    BUZZER_NOTE_D5SHARP_E5FLAT,  ///< 622.25 Hz
    BUZZER_NOTE_E5,              ///< 659.25 Hz
    BUZZER_NOTE_F5,              ///< 698.46 Hz
    BUZZER_NOTE_F5SHARP_G5FLAT,  ///< 739.99 Hz
    BUZZER_NOTE_G5,              ///< 783.99 Hz
    BUZZER_NOTE_G5SHARP_A5FLAT,  ///< 830.61 Hz
    BUZZER_NOTE_A5,              ///< 880.00 Hz
    BUZZER_NOTE_A5SHARP_B5FLAT,  ///< 932.33 Hz
    BUZZER_NOTE_B5,              ///< 987.77 Hz
    BUZZER_NOTE_C6,              ///< 1046.50 Hz
    BUZZER_NOTE_C6SHARP_D6FLAT,  ///< 1108.73 Hz
    BUZZER_NOTE_D6,              ///< 1174.66 Hz
    BUZZER_NOTE_D6SHARP_E6FLAT,  ///< 1244.51 Hz
    BUZZER_NOTE_E6,              ///< 1318.51 Hz
    BUZZER_NOTE_F6,              ///< 1396.91 Hz
    BUZZER_NOTE_F6SHARP_G6FLAT,  ///< 1479.98 Hz
    BUZZER_NOTE_G6,              ///< 1567.98 Hz
    BUZZER_NOTE_G6SHARP_A6FLAT,  ///< 1661.22 Hz
    BUZZER_NOTE_A6,              ///< 1760.00 Hz
    BUZZER_NOTE_A6SHARP_B6FLAT,  ///< 1864.66 Hz
    BUZZER_NOTE_B6,              ///< 1975.53 Hz
    BUZZER_NOTE_C7,              ///< 2093.00 Hz
    BUZZER_NOTE_C7SHARP_D7FLAT,  ///< 2217.46 Hz
    BUZZER_NOTE_D7,              ///< 2349.32 Hz
    BUZZER_NOTE_D7SHARP_E7FLAT,  ///< 2489.02 Hz
    BUZZER_NOTE_E7,              ///< 2637.02 Hz
    BUZZER_NOTE_F7,              ///< 2793.83 Hz
    BUZZER_NOTE_F7SHARP_G7FLAT,  ///< 2959.96 Hz
    BUZZER_NOTE_G7,              ///< 3135.96 Hz
    BUZZER_NOTE_G7SHARP_A7FLAT,  ///< 3322.44 Hz
    BUZZER_NOTE_A7,              ///< 3520.00 Hz
    BUZZER_NOTE_A7SHARP_B7FLAT,  ///< 3729.31 Hz
    BUZZER_NOTE_B7,              ///< 3951.07 Hz
    BUZZER_NOTE_C8,              ///< 4186.01 Hz
    BUZZER_NOTE_C8SHARP_D8FLAT,  ///< 4434.92 Hz
    BUZZER_NOTE_D8,              ///< 4698.63 Hz
    BUZZER_NOTE_D8SHARP_E8FLAT,  ///< 4978.03 Hz
    BUZZER_NOTE_E8,              ///< 5274.04 Hz
    BUZZER_NOTE_F8,              ///< 5587.65 Hz
    BUZZER_NOTE_F8SHARP_G8FLAT,  ///< 5919.91 Hz
    BUZZER_NOTE_G8,              ///< 6271.93 Hz
    BUZZER_NOTE_G8SHARP_A8FLAT,  ///< 6644.88 Hz
    BUZZER_NOTE_A8,              ///< 7040.00 Hz
    BUZZER_NOTE_A8SHARP_B8FLAT,  ///< 7458.62 Hz
    BUZZER_NOTE_B8,              ///< 7902.13 Hz
    BUZZER_NOTE_REST             ///< no sound
} watch_buzzer_note_t;

#define WATCH_BUZZER_PERIOD_REST 0

typedef bool (*watch_buzzer_raw_source_t)(uint16_t position, void* userdata, uint16_t* period, uint16_t* duration);

/** @addtogroup tcc Buzzer and LED Control (via the TCC peripheral)
  * @brief This section covers functions related to Timer Counter for Control peripheral, which drives the piezo buzzer
  *        embedded in the F-91W's back plate as well as the LED that backlights the display.
  */
/// @{
/** @brief Enables the TCC peripheral, which drives the buzzer.
  */
void watch_enable_buzzer(void);

/** @brief Sets the period of the buzzer.
  * @param period The period of a single cycle for the TCC peripheral. You can determine the period for
  *               a desired frequency with the following formula: period = 1000000 / freq
  * @param duty The duty cycle of the buzzer, from 0 to 50 (percent ON time). Do not use values over 50.
  * @note Unless you _really_ know what you're doing, use 25% for the duty cycle. If you're just aiming
  *       to play a tone at a given volume, use watch_buzzer_play_note_with_volume instead.
  */
void watch_set_buzzer_period_and_duty_cycle(uint32_t period, uint8_t duty);

/** @brief Disables the TCC peripheral that drives the buzzer (if LED not active).
  */
void watch_disable_buzzer(void);

/** @brief Turns the buzzer output on. It will emit a continuous sound at the given frequency.
  * @note The TCC peripheral that drives the buzzer does run in standby mode; if you wish for buzzer
  *       output to continue, you don't need to prevent your app from going to sleep.
  */
void watch_set_buzzer_on(void);

/** @brief Turns the buzzer output off.
  */
void watch_set_buzzer_off(void);

/** @brief Plays the given note for a set duration at the loudest possible volume.
  * @param note The note you wish to play, or BUZZER_NOTE_REST to disable output for the given duration.
  * @param duration_ms The duration of the note.
  */
void watch_buzzer_play_note(watch_buzzer_note_t note, uint16_t duration_ms);

/** @brief Plays the given note for a set duration.
  * @param note The note you wish to play, or BUZZER_NOTE_REST to disable output for the given duration.
  * @param duration_ms The duration of the note.
  * @param volume either WATCH_BUZZER_VOLUME_SOFT or WATCH_BUZZER_VOLUME_LOUD
  */
void watch_buzzer_play_note_with_volume(watch_buzzer_note_t note, uint16_t duration_ms, watch_buzzer_volume_t volume);

/// @brief An array of periods for all the notes on a piano, corresponding to the names in watch_buzzer_note_t.
extern const uint16_t NotePeriods[108];

/** @brief Plays the given sequence of notes in a non-blocking way.
  * @param note_sequence A pointer to the sequence of buzzer note & duration tuples, ending with a zero. A simple
  *        RLE logic is implemented: a negative number instead of a buzzer note means that the sequence
  *        is rewound by the given number of notes. The byte following a negative number determines the number
  *        of loops. I.e. if you want to repeat the last three notes of the sequence one time, you should provide 
  *        the tuple -3, 1. The repeated notes must not contain any other repeat markers, or you will end up with 
  *        an eternal loop.
  * @param callback_on_end A pointer to a callback function to be invoked when the sequence has finished playing.
  * @note This function plays the sequence asynchronously, so the UI will not be blocked. 
  *       Hint: It is not possible to play the lowest note BUZZER_NOTE_A1 (55.00 Hz). The note is represented by a 
  *       zero byte, which is used here as the end-of-sequence marker. But hey, a frequency that low cannot be
  *       played properly by the watch's buzzer, anyway.
  */
void watch_buzzer_play_sequence(int8_t *note_sequence, void (*callback_on_end)(void));

/** @brief Plays the given sequence of notes in a non-blocking way.
  * @param note_sequence A pointer to the sequence of buzzer note & duration tuples, ending with a zero. A simple
  *        RLE logic is implemented: a negative number instead of a buzzer note means that the sequence
  *        is rewound by the given number of notes. The byte following a negative number determines the number
  *        of loops. I.e. if you want to repeat the last three notes of the sequence one time, you should provide 
  *        the tuple -3, 1. The repeated notes must not contain any other repeat markers, or you will end up with 
  *        an eternal loop.
  * @param callback_on_end A pointer to a callback function to be invoked when the sequence has finished playing.
  * @param volume either WATCH_BUZZER_VOLUME_SOFT or WATCH_BUZZER_VOLUME_LOUD
  */
void watch_buzzer_play_sequence_with_volume(int8_t *note_sequence, void (*callback_on_end)(void), watch_buzzer_volume_t volume);

/** @brief Plays the given raw buzzer source function in a non-blocking way.
  *
  *  @details This function plays audio data generated by a raw source callback function,
  *           allowing for precise control over buzzer timing and frequency. The raw source
  *           function is called repeatedly to generate audio samples, each containing a
  *           period and duration for the buzzer tone.
  *           Useful for applications such as chirpy, so that they won't need to allocate a
  *           long note sequence, and we will also take care of all the timing logic.
  *
  * @param raw_source Pointer to the callback function that generates raw buzzer data.
  *                   The function should take a position parameter and return true if
  *                   more data is available, false if end of sequence is reached.
  *                   Parameters:
  *                   - position: Current position in the audio sequence (0-based)
  *                   - userdata: User-provided data passed through to the callback
  *                   - period: Pointer to store the period (in microseconds) for the tone
  *                   - duration: Pointer to store the duration (in microseconds) for the tone
  * @param userdata Pointer to user data that will be passed to the raw_source callback
  * @param callback_on_end A pointer to a callback function to be invoked when the sequence has finished playing.
  */
void watch_buzzer_play_raw_source(watch_buzzer_raw_source_t raw_source, void* userdata, watch_cb_t callback_on_end);

/** @brief Plays the given raw buzzer source function in a non-blocking way.
  *
  *  @details This function plays audio data generated by a raw source callback function,
  *           allowing for precise control over buzzer timing and frequency. The raw source
  *           function is called repeatedly to generate audio samples, each containing a
  *           period and duration for the buzzer tone.
  *           Useful for applications such as chirpy, so that they won't need to allocate a
  *           long note sequence, and we will also take care of all the timing logic.
  *
  * @param raw_source Pointer to the callback function that generates raw buzzer data.
  *                   The function should take a position parameter and return true if
  *                   more data is available, false if end of sequence is reached.
  *                   Parameters:
  *                   - position: Current position in the audio sequence (0-based)
  *                   - userdata: User-provided data passed through to the callback
  *                   - period: Pointer to store the period (in microseconds) for the tone
  *                   - duration: Pointer to store the duration (in microseconds) for the tone
  * @param userdata Pointer to user data that will be passed to the raw_source callback
  * @param callback_on_end A pointer to a callback function to be invoked when the sequence has finished playing.
  * @param volume either WATCH_BUZZER_VOLUME_SOFT or WATCH_BUZZER_VOLUME_LOUD
  */
void watch_buzzer_play_raw_source_with_volume(watch_buzzer_raw_source_t raw_source, void* userdata, watch_cb_t callback_on_end, watch_buzzer_volume_t volume);

/** @brief Aborts a playing sequence.
  */
void watch_buzzer_abort_sequence(void);

void watch_buzzer_register_global_callbacks(watch_cb_t cb_start, watch_cb_t cb_stop);

#ifndef __EMSCRIPTEN__
void irq_handler_tc0(void);
#endif

/** @addtogroup led LED Control
  * @brief This section covers functions related to the bi-color red/green LED mounted behind the LCD.
  * @details The SAM L22 is an exceedingly power efficient chip, whereas the LED's are relatively power-
  *          hungry. The green LED, at full power, consumes more power than the whole chip in active mode,
  *          and the red LED consumes about twelve times as much power! The LED's should thus be used only
  *          sparingly in order to preserve battery life.
  * @note Some watches use a red/blue LED instead of a red/green LED. You will be able to determine this
  *       easily when you double tap the reset button: if the pulsing bootloader LED is red, you have a
  *       red/green edition; if it is blue, you have a red/blue edition. For red/blue watches, build your
  *       project with the command `make COLOR=BLUE`, and the watch library will automatically swap the pins
  *       so that watch_set_led_red sets the red LED, and watch_set_led_green sets the blue one.
  */
/// @{
/** @brief Enables the TCC peripheral, which drives the LEDs.
  */
void watch_enable_leds(void);

/** @brief Disables the TCC peripheral that drives the LEDs (if buzzer not active).
  */
void watch_disable_leds(void);

/** @brief Sets the LED to a custom color by modulating each output's duty cycle.
  * @param red The red value from 0-255.
  * @param green The green value from 0-255. If your watch has a red/blue LED, this will be the blue value.
  */
void watch_set_led_color(uint8_t red, uint8_t green);

/** @brief On boards with an RGB LED, sets the LED to a custom color by modulating each output's duty cycle.
  * @param red The red value from 0-255.
  * @param green The green value from 0-255.
  * @param blue The blue value from 0-255.
  */
void watch_set_led_color_rgb(uint8_t red, uint8_t green, uint8_t blue);

/** @brief Sets the red LED to full brightness, and turns the green LED off.
  * @details Of the two LED's in the RG bi-color LED, the red LED is the less power-efficient one (~4.5 mA).
  */
void watch_set_led_red(void);

/** @brief Sets the green LED to full brightness, and turns the red LED off.
  * @details Of the two LED's in the RG bi-color LED, the green LED is the more power-efficient one (~0.44 mA).
  * @note If your watch has a red/blue LED, this method will set the LED to blue.
  */
void watch_set_led_green(void);

/** @brief Sets both red and green LEDs to full brightness.
  * @details The total current draw between the two LED's in this mode will be ~5 mA, which is more than the
  *          watch draws in any other mode. Take care not to drain the battery.
  * @note If your watch has a red/blue LED, this method will set the LED to pink.
  */
void watch_set_led_yellow(void);

/** @brief Turns both the red and the green LEDs off. */
void watch_set_led_off(void);

/// @brief An array of periods for all the notes on a piano, corresponding to the names in watch_buzzer_note_t.
extern const uint16_t NotePeriods[108];

/// @}
