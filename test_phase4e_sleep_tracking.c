/*
 * Phase 4E Sleep Tracking Test
 * 
 * Tests the sleep tracking and telemetry functions to verify correctness.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#define PHASE_ENGINE_ENABLED
#include "lib/phase/sleep_data.h"

void test_sleep_state_encoding() {
    printf("Testing sleep state encoding...\n");
    
    sleep_telemetry_state_t state;
    sleep_data_init(&state);
    
    // Test setting and getting sleep states
    sleep_data_set_state_at_epoch(&state, 0, DEEP_SLEEP);
    sleep_data_set_state_at_epoch(&state, 1, LIGHT_SLEEP);
    sleep_data_set_state_at_epoch(&state, 2, RESTLESS);
    sleep_data_set_state_at_epoch(&state, 3, WAKE);
    
    assert(sleep_data_get_state_at_epoch(&state, 0) == DEEP_SLEEP);
    assert(sleep_data_get_state_at_epoch(&state, 1) == LIGHT_SLEEP);
    assert(sleep_data_get_state_at_epoch(&state, 2) == RESTLESS);
    assert(sleep_data_get_state_at_epoch(&state, 3) == WAKE);
    
    printf("  ✓ Sleep state encoding works correctly\n");
}

void test_epoch_recording() {
    printf("Testing epoch recording...\n");
    
    sleep_telemetry_state_t state;
    sleep_data_init(&state);
    
    // Record some epochs with varying movement
    sleep_data_record_epoch(&state, 0);  // Deep sleep
    sleep_data_record_epoch(&state, 1);  // Deep sleep
    sleep_data_record_epoch(&state, 3);  // Light sleep
    sleep_data_record_epoch(&state, 10); // Restless
    sleep_data_record_epoch(&state, 20); // Wake
    
    assert(state.sleep_states.total_epochs == 5);
    assert(sleep_data_get_state_at_epoch(&state, 0) == DEEP_SLEEP);
    assert(sleep_data_get_state_at_epoch(&state, 2) == LIGHT_SLEEP);
    assert(sleep_data_get_state_at_epoch(&state, 3) == RESTLESS);
    assert(sleep_data_get_state_at_epoch(&state, 4) == WAKE);
    
    printf("  ✓ Epoch recording works correctly\n");
}

void test_wake_event_detection() {
    printf("Testing wake event detection...\n");
    
    sleep_telemetry_state_t state;
    sleep_data_init(&state);
    
    // Simulate 8 hours of sleep with a wake event
    // Skip first 40 epochs (20 minutes)
    for (int i = 0; i < 40; i++) {
        sleep_data_record_epoch(&state, 0);  // Deep sleep
    }
    
    // 30 minutes of deep sleep
    for (int i = 0; i < 60; i++) {
        sleep_data_record_epoch(&state, 0);  // Deep sleep
    }
    
    // 10 minutes of wake (20 epochs) - this should be detected as wake event
    for (int i = 0; i < 20; i++) {
        sleep_data_record_epoch(&state, 20);  // Wake
    }
    
    // More deep sleep
    for (int i = 0; i < 100; i++) {
        sleep_data_record_epoch(&state, 0);  // Deep sleep
    }
    
    sleep_data_detect_wake_events(&state);
    
    assert(state.wake_events.wake_count == 1);
    assert(state.wake_events.total_wake_min == 10);
    
    printf("  ✓ Wake event detection works correctly\n");
    printf("    Wake count: %d\n", state.wake_events.wake_count);
    printf("    Total wake minutes: %d\n", state.wake_events.total_wake_min);
}

void test_restlessness_calculation() {
    printf("Testing restlessness calculation...\n");
    
    sleep_telemetry_state_t state;
    sleep_data_init(&state);
    
    // Simulate perfect sleep (all deep sleep)
    for (int i = 0; i < 100; i++) {
        sleep_data_record_epoch(&state, 0);
    }
    
    sleep_data_detect_wake_events(&state);
    uint8_t restlessness_perfect = sleep_data_calc_restlessness(&state);
    
    printf("  Perfect sleep restlessness: %d (should be 0)\n", restlessness_perfect);
    assert(restlessness_perfect == 0);
    
    // Simulate very restless sleep
    sleep_data_init(&state);
    for (int i = 0; i < 100; i++) {
        sleep_data_record_epoch(&state, 20);  // All wake
    }
    
    sleep_data_detect_wake_events(&state);
    uint8_t restlessness_bad = sleep_data_calc_restlessness(&state);
    
    printf("  Very restless sleep restlessness: %d (should be high)\n", restlessness_bad);
    assert(restlessness_bad > 50);
    
    printf("  ✓ Restlessness calculation works correctly\n");
}

void test_telemetry_accumulation() {
    printf("Testing telemetry accumulation...\n");
    
    sleep_telemetry_state_t state;
    sleep_data_init(&state);
    
    // Accumulate telemetry for hour 0
    sleep_data_accumulate_telemetry(&state,
        0,                      // hour
        1,                      // current_zone (Momentum)
        0,                      // prev_zone (Emergence)
        DOMINANT_EM,            // dominant_metric
        false,                  // anomaly_fired
        30,                     // light_minutes
        15,                     // motion_interrupts
        3700,                   // battery_mv (3.7V)
        45,                     // sd_now
        50,                     // sd_prev
        60,                     // em_now
        55,                     // em_prev
        70,                     // wk_now
        65,                     // wk_prev
        80,                     // comfort_now
        78                      // comfort_prev
    );
    
    hourly_telemetry_t *sample = &state.telemetry.hours[0];
    
    assert((sample->flags & 0x01) == 1);  // Zone transition
    assert(((sample->flags >> 1) & 0x03) == DOMINANT_EM);  // Dominant metric
    assert(sample->light_exposure_min == 30);
    assert(sample->movement_count == 15);
    
    printf("  ✓ Telemetry accumulation works correctly\n");
    printf("    Zone transition: %s\n", (sample->flags & 0x01) ? "yes" : "no");
    printf("    Dominant metric: %d\n", (sample->flags >> 1) & 0x03);
    printf("    Light exposure: %d min\n", sample->light_exposure_min);
    printf("    Movement count: %d\n", sample->movement_count);
}

void test_ram_usage() {
    printf("Testing RAM usage...\n");
    
    size_t total_size = sizeof(sleep_telemetry_state_t);
    
    printf("  Total RAM usage: %zu bytes\n", total_size);
    printf("    sleep_state_data_t: %zu bytes\n", sizeof(sleep_state_data_t));
    printf("    wake_event_data_t: %zu bytes\n", sizeof(wake_event_data_t));
    printf("    restlessness_data_t: %zu bytes\n", sizeof(restlessness_data_t));
    printf("    daily_telemetry_t: %zu bytes\n", sizeof(daily_telemetry_t));
    printf("    sleep_state_thresholds_t: %zu bytes\n", sizeof(sleep_state_thresholds_t));
    
    assert(total_size <= 500);  // Should be ~471 bytes per plan (492 actual due to padding)
    printf("  ✓ RAM usage within budget (<%d bytes)\n", 500);
}

int main() {
    printf("=== Phase 4E Sleep Tracking Tests ===\n\n");
    
    test_sleep_state_encoding();
    test_epoch_recording();
    test_wake_event_detection();
    test_restlessness_calculation();
    test_telemetry_accumulation();
    test_ram_usage();
    
    printf("\n=== All tests passed! ===\n");
    
    return 0;
}
