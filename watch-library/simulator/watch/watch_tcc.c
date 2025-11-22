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

#include "watch_tcc.h"
#include "watch_main_loop.h"

#include <emscripten.h>
#include <emscripten/html5.h>

static volatile bool buzzer_enabled = false;
static uint32_t buzzer_period;

void cb_watch_buzzer_seq(void *userData);
void cb_watch_buzzer_raw_source(void *userData);

static uint16_t _seq_position;
static int8_t _tone_ticks, _repeat_counter;
static volatile long _em_interval_id = 0;
static int8_t *_sequence;
static watch_buzzer_raw_source_t _raw_source;
static void* _userdata;
static uint8_t _volume;
static void (*_cb_finished)(void);
static watch_cb_t _cb_start_global = NULL;
static watch_cb_t _cb_stop_global = NULL;
static volatile bool _buzzer_is_active = false;

static inline void _em_interval_stop() {
    emscripten_clear_interval(_em_interval_id);
    _em_interval_id = 0;
}

void watch_buzzer_play_sequence(int8_t *note_sequence, void (*callback_on_end)(void)) {
    watch_buzzer_play_sequence_with_volume(note_sequence, callback_on_end, WATCH_BUZZER_VOLUME_LOUD);
}

void watch_buzzer_play_sequence_with_volume(int8_t *note_sequence, void (*callback_on_end)(void), watch_buzzer_volume_t volume) {
    watch_buzzer_abort_sequence();

    // prepare buzzer
    watch_enable_buzzer();
    watch_set_buzzer_off();

    _buzzer_is_active = true;

    if (_cb_start_global) {
        _cb_start_global();
    }

    _sequence = note_sequence;
    _cb_finished = callback_on_end;
    _volume = volume == WATCH_BUZZER_VOLUME_SOFT ? 5 : 25;
    _seq_position = 0;
    _tone_ticks = 0;
    _repeat_counter = -1;
    // initiate 64 hz callback
    _em_interval_id = emscripten_set_interval(cb_watch_buzzer_seq, (double)(1000/64), (void *)NULL);
}

void cb_watch_buzzer_seq(void *userData) {
    // callback for reading the note sequence
    (void) userData;
    if (_tone_ticks == 0) {
        if (_sequence[_seq_position] < 0 && _sequence[_seq_position + 1]) {
            // repeat indicator found
            if (_repeat_counter == -1) {
                // first encounter: load repeat counter
                _repeat_counter = _sequence[_seq_position + 1];
            } else _repeat_counter--;
            if (_repeat_counter > 0)
                // rewind
                if (_seq_position > _sequence[_seq_position] * -2)
                    _seq_position += _sequence[_seq_position] * 2;
                else
                    _seq_position = 0;
            else {
                // continue
                _seq_position += 2;
                _repeat_counter = -1;
            }
        }
        if (_sequence[_seq_position] && _sequence[_seq_position + 1]) {
            // read note
            watch_buzzer_note_t note = _sequence[_seq_position];
            if (note == BUZZER_NOTE_REST) {
                watch_set_buzzer_off();
            } else {
                watch_set_buzzer_period_and_duty_cycle(NotePeriods[note], _volume);
                watch_set_buzzer_on();
            }
            // set duration ticks and move to next tone
            _tone_ticks = _sequence[_seq_position + 1] - 1;
            _seq_position += 2;
        } else {
            // end the sequence
            watch_buzzer_abort_sequence();
        }
    } else _tone_ticks--;
}

void watch_buzzer_play_raw_source(watch_buzzer_raw_source_t raw_source, void* userdata, watch_cb_t callback_on_end) {
    watch_buzzer_play_raw_source_with_volume(raw_source, userdata, callback_on_end, WATCH_BUZZER_VOLUME_LOUD);
}

void watch_buzzer_play_raw_source_with_volume(watch_buzzer_raw_source_t raw_source, void* userdata, watch_cb_t callback_on_end, watch_buzzer_volume_t volume) {
    watch_buzzer_abort_sequence();

    // prepare buzzer
    watch_enable_buzzer();
    watch_set_buzzer_off();

    _buzzer_is_active = true;

    if (_cb_start_global) {
        _cb_start_global();
    }

    _raw_source = raw_source;
    _userdata = userdata;
    _cb_finished = callback_on_end;
    _volume = volume == WATCH_BUZZER_VOLUME_SOFT ? 5 : 25;
    _seq_position = 0;
    _tone_ticks = 0;

    // initiate 64 hz callback
    _em_interval_id = emscripten_set_interval(cb_watch_buzzer_raw_source, (double)(1000/64), (void *)NULL);
}

void cb_watch_buzzer_raw_source(void *userData) {
    // callback for reading the note sequence
    (void) userData;
    uint16_t period;
    uint16_t duration;
    bool done;

    if (_tone_ticks == 0) {
        done = _raw_source(_seq_position, _userdata, &period, &duration);

        if (done || duration == 0) {
            // end the sequence
            watch_buzzer_abort_sequence();
        } else {
            if (period == WATCH_BUZZER_PERIOD_REST) {
                watch_set_buzzer_off();
            } else {
                watch_set_buzzer_period_and_duty_cycle(period, _volume);
                watch_set_buzzer_on();
            }

            // set duration ticks and move to next tone
            _tone_ticks = duration - 1;
            _seq_position += 1;
        }
    } else {
        _tone_ticks--;
    }
}

void watch_buzzer_abort_sequence(void) {
    // ends/aborts the sequence
    if (_em_interval_id) _em_interval_stop();

    watch_set_buzzer_off();
    watch_disable_buzzer();

    if (!_buzzer_is_active) {
        return;
    }

    _buzzer_is_active = false;

    if (_cb_stop_global) {
        _cb_stop_global();
    }

    if (_cb_finished) {
        _cb_finished();
    }
}

void watch_buzzer_register_global_callbacks(watch_cb_t cb_start, watch_cb_t cb_stop) {
    _cb_stop_global = cb_start;
    _cb_stop_global = cb_stop;
}

void watch_enable_buzzer(void) {
    watch_buzzer_abort_sequence();
    buzzer_enabled = true;
    buzzer_period = NotePeriods[BUZZER_NOTE_A4];

    EM_ASM({
        // "It's recommended to create one AudioContext and reuse it instead of initializing a new one each time."
        // https://developer.mozilla.org/en-US/docs/Web/API/AudioContext
        if (!Module['audioContext']) {
            Module['audioContext'] = new (window.AudioContext || window.webkitAudioContext)();
        }
    });
}

void watch_set_buzzer_period_and_duty_cycle(uint32_t period, uint8_t duty_cycle) {
    (void) duty_cycle;
    if (!buzzer_enabled) return;
    buzzer_period = period;
}

void watch_disable_buzzer(void) {
    buzzer_enabled = false;
    buzzer_period = NotePeriods[BUZZER_NOTE_A4];
}

void watch_set_buzzer_on(void) {
    if (!buzzer_enabled) return;

    EM_ASM({
        const audioContext = Module['audioContext'];
        if (!audioContext) return;

        if (!(audioContext._oscillator && audioContext._gain)) {
            const oscillator = audioContext.createOscillator();
            const gain = audioContext.createGain();
            oscillator.type = 'triangle';
            oscillator.connect(gain);
            gain.connect(audioContext.destination);
            oscillator.start(0);

            audioContext._oscillator = oscillator;
            audioContext._gain = gain;
        }

        audioContext._oscillator.frequency.value = 1e6/$0;
        audioContext._gain.gain.value = volumeGain;
    }, buzzer_period);
}

void watch_set_buzzer_off(void) {
    if (!buzzer_enabled) return;

    EM_ASM({
        const audioContext = Module['audioContext'];
        if (audioContext && audioContext._gain) {
            audioContext._gain.gain.value = 0;
        }
    });
}

void watch_buzzer_play_note(watch_buzzer_note_t note, uint16_t duration_ms) {
    watch_buzzer_play_note_with_volume(note, duration_ms, WATCH_BUZZER_VOLUME_LOUD);
}

void watch_buzzer_play_note_with_volume(watch_buzzer_note_t note, uint16_t duration_ms, watch_buzzer_volume_t volume) {
    static int8_t single_note_sequence[3];

    single_note_sequence[0] = note;
    // 64 ticks per second for the tc0
    // Each tick is approximately 15ms
    uint16_t duration = duration_ms / 15;
    if (duration > 127) duration = 127;
    single_note_sequence[1] = (int8_t)duration;
    single_note_sequence[2] = 0;

    watch_buzzer_play_sequence_with_volume(single_note_sequence, NULL, volume);
}

void watch_enable_leds(void) {}

void watch_disable_leds(void) {}

void watch_set_led_color_rgb(uint8_t red, uint8_t green, uint8_t blue) {
    EM_ASM({
        let filter = document.getElementById("ledcolor");
        let color_matrix = filter.children[0].values.baseVal;
        color_matrix[0].value = $0 / 255; // red
        color_matrix[6].value = $1 / 255; // green
        color_matrix[12].value = $2 / 255; // blue
        document.getElementById('light').style.opacity = Math.min(255, $0 + $1 + $2) / 255;
    }, red, green, blue);
}

void watch_set_led_red(void) {
    watch_set_led_color_rgb(255, 0, 0);
}

void watch_set_led_green(void) {
    watch_set_led_color_rgb(0, 255, 0);
}

void watch_set_led_yellow(void) {
    watch_set_led_color_rgb(255, 255, 0);
}

void watch_set_led_off(void) {
    watch_set_led_color_rgb(0, 0, 0);
}
