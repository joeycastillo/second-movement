# Optical Comms Time Sync - Code Review Findings

**Review Date:** 2026-02-18  
**Reviewer:** Code Review Agent  
**Repo:** second-movement

---

## Executive Summary

**Status:** ⚠️ **CRITICAL BUG FOUND** - Baud rate mismatch prevents communication

The optical comms implementation has a fundamental timing mismatch between the companion app transmitter and the watch receiver that will prevent successful communication. Several other aspects of the implementation are correct and well-designed.

---

## 🔴 CRITICAL ISSUE: Baud Rate Mismatch

### Problem
The companion app transmits at **10 bps** while the watch receiver expects **16 bps**.

### Details

**Companion App (TX):**
```javascript
async function flashBit(bit) {
    const h = 50; // 50ms half-bit => 100ms/bit => 10bps
    // ... transmits at 100ms per bit
}
```
- Each bit period: 100ms (50ms first half + 50ms second half)
- Baud rate: 1000ms ÷ 100ms = **10 bps**

**Watch RX:**
```c
// Manchester at 16 bps / 64 Hz: midpoint transitions occur every 4 ticks (one per bit period).
#define SYNC_TICKS_PER_EDGE 4            // Expected ticks between consecutive sync transitions

// Poll at 64 Hz
movement_request_tick_frequency(64);

// decode_phase counts 0..3 within each 4-tick bit period.
state->rx_state.decode_phase = (state->rx_state.decode_phase + 1) & 3;
```
- Tick rate: 64 Hz → 15.625ms per tick
- Bit period: 4 ticks = 4 × 15.625ms = 62.5ms
- Baud rate: 1000ms ÷ 62.5ms = **16 bps**

### Impact
The receiver will sample too frequently for the slower transmitter signal:
- RX expects transitions every 4 ticks (62.5ms)
- TX provides transitions every 6.4 ticks (100ms)
- Sync detection will timeout (edges not spaced as expected)
- Even if sync somehow succeeds, bit decoding will fail due to phase drift

### Recommended Fix
**Option 1 (preferred):** Speed up TX to match RX
```javascript
const h = 31.25; // 31.25ms half-bit => 62.5ms/bit => 16bps
```

**Option 2:** Slow down RX to match TX
```c
#define SYNC_TICKS_PER_EDGE 6    // 6.4 ticks ≈ 100ms @ 64 Hz
// But note: 6 ticks = 93.75ms, 7 ticks = 109.375ms
// This introduces rounding error; TX at 16 bps is cleaner
```

**Option 3:** Use a different tick rate
- RX at 100 Hz would give 10ms ticks
- 10 ticks per bit = 100ms → matches TX at 10 bps
- But requires more CPU wake-ups

---

## ✅ CORRECT IMPLEMENTATIONS

### 1. Packet Generation (Companion App)

**Timestamp Encoding:** ✅ Correct
```javascript
const now = Math.floor(Date.now() / 1000);  // Unix timestamp in seconds
const packet = [
    0x06,  // LEN
    0x01,  // TYPE_TIME_SYNC
    (now >>  0) & 0xFF,  // timestamp byte 0 (LSB)
    (now >>  8) & 0xFF,  // timestamp byte 1
    (now >> 16) & 0xFF,  // timestamp byte 2
    (now >> 24) & 0xFF,  // timestamp byte 3 (MSB)
    (tzOffset >> 0) & 0xFF,  // tz_offset byte 0 (LSB)
    (tzOffset >> 8) & 0xFF,  // tz_offset byte 1 (MSB)
];
```
- Uses `Date.now()` correctly
- Encodes as 4-byte little-endian ✅
- Timezone offset as 2-byte little-endian signed ✅

**CRC-8:** ✅ Correct
```javascript
function crc8(data) {
    let crc = 0;
    for (const byte of data) {
        crc ^= byte;
        for (let i = 0; i < 8; i++) {
            crc = (crc & 0x01) ? (crc >> 1) ^ 0x8C : crc >> 1;  // CRC-8/MAXIM
        }
    }
    return crc;
}
```
Matches receiver implementation exactly (CRC-8/MAXIM polynomial 0x31 reversed).

### 2. Manchester Encoding/Decoding

**TX Convention:** ✅ Correct
```javascript
// 0 = HIGH->LOW (white->black)
// 1 = LOW->HIGH (black->white)
```

**RX Convention:** ✅ Matches
```c
static int8_t decode_manchester_bit(bool first_half, bool second_half) {
    if (!first_half && second_half) {
        return 1;  // LOW -> HIGH = 1
    } else if (first_half && !second_half) {
        return 0;  // HIGH -> LOW = 0
    }
    return -1;  // No transition (framing error)
}
```

Both use IEEE 802.3 Manchester convention consistently.

### 3. Packet Format Alignment

**TX sends:** `SYNC(0xAA) + LEN + TYPE + DATA[LEN] + CRC8`  
**RX expects:** Same structure ✅

Both define:
- Sync byte: `0xAA` (10101010 pattern for bit clock recovery)
- Length field includes only the data payload (not LEN, TYPE, or CRC)
- Packet structure: `[LEN][TYPE][DATA...][CRC8]`

### 4. Timestamp Extraction

**RX Decoding:** ✅ Correct little-endian extraction
```c
uint32_t timestamp =
    ((uint32_t)state->rx_state.rx_buffer[2] <<  0) |
    ((uint32_t)state->rx_state.rx_buffer[3] <<  8) |
    ((uint32_t)state->rx_state.rx_buffer[4] << 16) |
    ((uint32_t)state->rx_state.rx_buffer[5] << 24);
```
Matches TX byte order exactly.

### 5. Timezone Handling

**TX sends timezone offset:**
```javascript
const tzOffset = -new Date().getTimezoneOffset(); // minutes from UTC
```
For AKST (UTC-9): `tzOffset = -(-540) = 540` (positive 540 minutes = +9 hours... wait, that's wrong)

Actually, let me recalculate:
- `getTimezoneOffset()` for AKST returns `540` (minutes *behind* UTC)
- `tzOffset = -540` → encoded as signed int16 little-endian

**RX correctly ignores it:**
```c
int16_t tz_offset = ...;
(void)tz_offset;  // retained for future use / logging
watch_rtc_set_unix_time(timestamp);  // pass raw UTC, NOT adjusted
```

**Analysis:** ✅ Correct behavior
- The comment correctly states that `watch_rtc_set_unix_time()` expects raw UTC
- Applying `tz_offset` here would cause double-correction (RTC handles local time internally)
- The timezone offset is correctly transmitted but not applied

### 6. Epoch Range Validation

**RX validation:** ✅ Correct
```c
if (timestamp < 1577836800UL || timestamp > 4133980799UL) {
    state->mode = COMMS_MODE_RX_ERROR;
    break;
}
```

- `1577836800` = 2020-01-01 00:00:00 UTC ✓
- `4133980799` = 2100-12-31 23:59:59 UTC ✓

This replaces a previous buggy check that tested derived hour/minute values (which could never fail).

### 7. Sync Detection State Machine

**RX sync detection:** ✅ Well-designed
```c
#define SYNC_TICKS_PER_EDGE 4        // Expected spacing
#define SYNC_TICKS_TOLERANCE 1       // +/- jitter allowance
#define SYNC_EDGES_NEEDED 6          // Consecutive edges required
```

The implementation correctly:
- Counts edges with consistent spacing (4 ± 1 ticks)
- Requires 6 consecutive good edges before locking phase
- Resets edge count on bad spacing
- Times out after 10 seconds if no sync found

---

## ⚠️ MINOR ISSUES

### 1. Timezone Offset Sign Confusion

The TX code has a confusing comment:
```javascript
const tzOffset = -new Date().getTimezoneOffset(); // minutes from UTC
```

For AKST (UTC-9):
- `getTimezoneOffset()` returns `540` (minutes *behind* UTC, positive value)
- `tzOffset = -540` (negative value)

The sign inversion is correct (converts "minutes behind" to "offset from UTC"), but the variable name and comment could be clearer. This doesn't affect functionality since the RX ignores the value, but it's a potential source of confusion.

**Recommendation:** Clarify the comment:
```javascript
// getTimezoneOffset() is positive for zones behind UTC (e.g., 540 for AKST/UTC-9)
// Invert sign to get offset from UTC (e.g., -540 = -9 hours)
const tzOffset = -new Date().getTimezoneOffset();
```

### 2. CRC Position in Comments

The companion app comment says:
```javascript
// Packet: LEN(0x06) + TYPE(0x01) + timestamp(4 LE) + timezone(2 LE) + CRC
```

But the actual packet structure is:
```
[0x06] [0x01] [ts0] [ts1] [ts2] [ts3] [tz0] [tz1] [CRC]
 LEN    TYPE   ----timestamp----  --tz_offset--  CRC8
```

The CRC is appended *after* the packet array is built via `packet.push(crc8(packet))`. The comment correctly lists the fields but might benefit from explicitly showing the CRC is calculated over all preceding bytes.

---

## 📊 INTEGRATION SUMMARY

| Component | Status | Notes |
|-----------|--------|-------|
| **Baud Rate** | ❌ FAIL | TX: 10 bps, RX: 16 bps (mismatch) |
| **Manchester Encoding** | ✅ PASS | Convention matches (LOW→HIGH=1, HIGH→LOW=0) |
| **Packet Format** | ✅ PASS | Structure aligned (SYNC + LEN + TYPE + DATA + CRC) |
| **Timestamp Encoding** | ✅ PASS | 4-byte little-endian, both sides agree |
| **Timezone Encoding** | ✅ PASS | 2-byte little-endian signed, correctly ignored by RX |
| **Timezone Handling** | ✅ PASS | Raw UTC passed to RTC (no double-correction) |
| **Epoch Validation** | ✅ PASS | Range [2020-01-01, 2100-12-31] correctly enforced |
| **Sync Pattern** | ✅ PASS | Both use 0xAA sync byte |
| **CRC** | ✅ PASS | CRC-8/MAXIM polynomial matches |

---

## 🔧 REQUIRED FIX

**To make this system work, fix the baud rate mismatch:**

**Companion App (index.html), line ~860:**
```javascript
async function flashBit(bit) {
    const h = 31.25;  // CHANGED: 31.25ms half-bit => 62.5ms/bit => 16bps (matches watch RX)
    if (bit === 0) {
        txFlashArea.style.background = '#FFFFFF';
        txFlashArea.classList.add('light-phase');
        await sleep(h);
        txFlashArea.style.background = '#000000';
        txFlashArea.classList.remove('light-phase');
        await sleep(h);
    } else {
        txFlashArea.style.background = '#000000';
        txFlashArea.classList.remove('light-phase');
        await sleep(h);
        txFlashArea.style.background = '#FFFFFF';
        txFlashArea.classList.add('light-phase');
        await sleep(h);
    }
}
```

**Also update the timing estimate:**
```javascript
const totalBytes       = packet.length + 1; // +1 for SYNC
const estimatedSeconds = Math.ceil(totalBytes * 0.8);  // CHANGED: was 0.8, now accurate for 16bps
// 9 bytes * 8 bits * 62.5ms = 4.5s
```

Change to:
```javascript
const estimatedSeconds = Math.ceil(totalBytes * 8 * 0.0625);  // 8 bits/byte * 0.0625s/bit
```

---

## ✅ CONCLUSION

The implementation demonstrates good understanding of:
- Manchester encoding for clock recovery
- Little-endian multi-byte encoding
- CRC checksums for data integrity
- Edge-based sync detection with jitter tolerance
- Proper UTC timestamp handling

However, the **baud rate mismatch is a showstopper bug** that will prevent any communication. Once fixed, the system should work correctly.

**Estimated fix effort:** 5 minutes (change one constant, test)

---

**Next Steps:**
1. Apply the baud rate fix to `companion-app/index.html`
2. Test with actual hardware
3. Verify sync lock occurs within 1-2 seconds
4. Confirm timestamp is set correctly on the watch

