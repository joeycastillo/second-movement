# Anomaly Detection & Notification Specification

> **Goal:** Alert wearer when metrics deviate significantly from baseline, indicating misalignment with natural cycles.

---

## Detection Logic

### Baseline Reference
Most metrics use **50 as neutral baseline** (midpoint of 0-100 range):
- SD (Sleep Debt): 50 = moderate debt
- EM (Emotional): 50 = neutral mood
- WK (Wake Momentum): 50 = halfway ramped
- Energy: 50 = moderate capacity
- Comfort: 50 = moderate environmental alignment

**Exception:** JL (Jet Lag) uses 0 as baseline (no disruption)

### Anomaly Thresholds

**Trigger conditions:**
```c
// High anomaly (metric too high)
if (metric >= 70 && abs(metric - 50) >= 20) {
    trigger_anomaly(ANOMALY_HIGH, metric_id);
}

// Low anomaly (metric too low)
if (metric <= 30 && abs(metric - 50) >= 20) {
    trigger_anomaly(ANOMALY_LOW, metric_id);
}
```

**Hysteresis:** Once anomaly triggered, requires metric to return within ±15 of baseline (35-65 range) before clearing.

**Rate limiting:** Maximum 1 notification per metric per hour (prevent notification spam).

---

## Notification Behavior

### Audio Notification

**Tone:** Soft, subtle chime (respects `button_sound` setting)

**Pattern:**
- **High anomaly:** Two ascending beeps (C6, E6) — "alert, check up"
- **Low anomaly:** Two descending beeps (E6, C6) — "alert, check down"

**Volume:** Uses `WATCH_BUZZER_VOLUME_SOFT` regardless of button_volume setting (non-intrusive).

**When:** Only if watch is NOT in low-power mode.

### Visual Notification

**LCD Indicator:**
- **High anomaly:** Display `+` in top-right corner
- **Low anomaly:** Display `-` in top-right corner
- **Duration:** 3 seconds, then clear

**Only if:** Watch is NOT in low-power mode (display is awake).

---

## Implementation

### Data Structure

```c
typedef enum {
    ANOMALY_NONE = 0,
    ANOMALY_HIGH,   // Metric >= 70, deviation >= 20
    ANOMALY_LOW     // Metric <= 30, deviation >= 20
} anomaly_type_t;

typedef struct {
    uint8_t metric_id;           // 0-5 (SD, EM, WK, Energy, JL, Comfort)
    anomaly_type_t type;         // High, low, or none
    uint32_t last_notify_time;   // RTC timestamp of last notification
    bool active;                 // Anomaly currently active
} anomaly_state_t;

// Global anomaly tracker (6 metrics)
anomaly_state_t anomaly_tracker[6];
```

### Detection Function

```c
void detect_anomalies(const metrics_snapshot_t *metrics, uint32_t current_time) {
    uint8_t metric_values[6] = {
        metrics->sd,
        metrics->em,
        metrics->wk,
        metrics->energy,
        metrics->jl,       // JL uses 0 as baseline
        metrics->comfort
    };
    
    for (uint8_t i = 0; i < 6; i++) {
        uint8_t value = metric_values[i];
        uint8_t baseline = (i == 4) ? 0 : 50;  // JL uses 0, others use 50
        int16_t deviation = abs(value - baseline);
        
        anomaly_type_t new_type = ANOMALY_NONE;
        
        if (value >= 70 && deviation >= 20) {
            new_type = ANOMALY_HIGH;
        } else if (value <= 30 && deviation >= 20) {
            new_type = ANOMALY_LOW;
        }
        
        // Check if anomaly state changed
        if (new_type != ANOMALY_NONE && !anomaly_tracker[i].active) {
            // New anomaly detected
            // Check rate limit (1 hour)
            if (current_time - anomaly_tracker[i].last_notify_time >= 3600) {
                notify_anomaly(i, new_type);
                anomaly_tracker[i].last_notify_time = current_time;
            }
            anomaly_tracker[i].active = true;
            anomaly_tracker[i].type = new_type;
        } else if (new_type == ANOMALY_NONE && anomaly_tracker[i].active) {
            // Anomaly cleared (hysteresis: back to 35-65 range)
            if (value >= 35 && value <= 65) {
                anomaly_tracker[i].active = false;
                anomaly_tracker[i].type = ANOMALY_NONE;
            }
        }
    }
}
```

### Notification Function

```c
void notify_anomaly(uint8_t metric_id, anomaly_type_t type) {
    // Only notify if NOT in low-power mode
    if (movement_state.le_mode_active) {
        return;
    }
    
    // Audio notification (soft chime)
    if (movement_button_should_sound()) {
        int8_t tone_sequence[7];
        if (type == ANOMALY_HIGH) {
            // Two ascending beeps: C6, E6
            tone_sequence[0] = BUZZER_NOTE_C6;
            tone_sequence[1] = 3;
            tone_sequence[2] = BUZZER_NOTE_REST;
            tone_sequence[3] = 2;
            tone_sequence[4] = BUZZER_NOTE_E6;
            tone_sequence[5] = 3;
            tone_sequence[6] = 0;  // End
        } else {
            // Two descending beeps: E6, C6
            tone_sequence[0] = BUZZER_NOTE_E6;
            tone_sequence[1] = 3;
            tone_sequence[2] = BUZZER_NOTE_REST;
            tone_sequence[3] = 2;
            tone_sequence[4] = BUZZER_NOTE_C6;
            tone_sequence[5] = 3;
            tone_sequence[6] = 0;  // End
        }
        
        // Play sequence at soft volume
        watch_buzzer_play_sequence_with_volume(tone_sequence, NULL, WATCH_BUZZER_VOLUME_SOFT);
    }
    
    // Visual notification (+/- in top-right)
    // TODO: Implement LCD indicator display
    // - Set indicator_char = (type == ANOMALY_HIGH) ? '+' : '-'
    // - Set indicator_timeout = 3 seconds
    // - Clear after timeout
}
```

---

## Integration Points

**Where to call `detect_anomalies()`:**
- Inside `_movement_handle_top_of_minute()` after `metrics_update()`
- Runs every 15 minutes when metric tick occurs
- Uses RTC timestamp for rate limiting

**Power budget:**
- Detection: <0.1 µA (simple integer comparisons)
- Notification: ~50 µA for 1 second (buzzer + display) = ~0.014 µAh per event
- Assuming 1-2 anomalies per day = negligible impact

---

## User Control

**Settings:**
- `anomaly_notifications_enabled` (bool, default true)
- Controlled via settings face (future enhancement)
- Stored in movement_settings_t

**Override:**
- When in low-power mode: NO notifications (screen off, save power)
- When button_sound disabled: NO audio, visual only
- When timeout occurs: Clear any active indicators

---

## Testing Scenarios

**High Sleep Debt (SD = 85):**
- Trigger: After 3 nights of <6h sleep
- Notification: Two ascending beeps + `+` indicator
- User action: Check SD metric on Emergence face, prioritize rest

**Low Energy (NRG = 25):**
- Trigger: High sleep debt + low phase score + sedentary
- Notification: Two descending beeps + `-` indicator
- User action: Check Energy metric on Active face, consider break/nap

**High Comfort (CMF = 90):**
- Trigger: Perfect temperature + ideal light levels
- Notification: Two ascending beeps + `+` indicator
- User interpretation: Conditions are ideal, good time for activity

**Rate limiting test:**
- SD stays at 85 for 6 hours
- Notification at hour 0, hour 3, hour 6 (once per hour max)
- Prevents spam from persistent anomalies
