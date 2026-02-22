/*
 * Phase Export Structure Size Validation Test
 * Compile: gcc -o test_phase_export_size test_phase_export_size.c
 * Run: ./test_phase_export_size
 */

#include <stdio.h>
#include <stdint.h>

// Phase Engine hourly sample (6 bytes)
typedef struct {
    uint8_t hour : 5;          // 0-23
    uint8_t zone : 2;          // 0-3 (Emergence/Momentum/Active/Descent)
    uint8_t reserved : 1;      // Future use
    
    uint8_t phase_score;       // 0-100
    uint8_t sd;                // Sleep Debt (offset by 60: 0-180 → -60 to +120)
    uint8_t em;                // Emotional 0-100
    uint8_t wk;                // Wake Momentum 0-100
    uint8_t comfort;           // Comfort 0-100
} phase_hourly_sample_t;

// Phase Engine export data (prepended to circadian export)
typedef struct {
    uint8_t version;           // Protocol version (0x01)
    uint8_t sample_count;      // Number of samples (0-24)
    phase_hourly_sample_t samples[24];  // Hourly samples
} phase_export_t;  // Total: 2 + (6 × 24) = 146 bytes

int main() {
    printf("Structure Size Validation\n");
    printf("=========================\n\n");
    
    printf("phase_hourly_sample_t: %zu bytes (expected: 6)\n", sizeof(phase_hourly_sample_t));
    printf("phase_export_t: %zu bytes (expected: 146)\n", sizeof(phase_export_t));
    printf("\n");
    
    // Verify sizes
    int errors = 0;
    
    if (sizeof(phase_hourly_sample_t) != 6) {
        printf("ERROR: phase_hourly_sample_t is %zu bytes, expected 6!\n", 
               sizeof(phase_hourly_sample_t));
        printf("       Bitfield packing may be causing alignment issues.\n");
        errors++;
    } else {
        printf("✓ phase_hourly_sample_t size correct (6 bytes)\n");
    }
    
    if (sizeof(phase_export_t) != 146) {
        printf("ERROR: phase_export_t is %zu bytes, expected 146!\n", 
               sizeof(phase_export_t));
        printf("       Expected: 2 + (24 × 6) = 146 bytes\n");
        errors++;
    } else {
        printf("✓ phase_export_t size correct (146 bytes)\n");
    }
    
    printf("\n");
    
    // Test buffer sizes
    printf("Buffer Size Calculations\n");
    printf("========================\n\n");
    printf("Phase data: 146 bytes\n");
    printf("Circadian data: 112 bytes\n");
    printf("Total binary: 258 bytes (rounded to 256 buffer)\n");
    printf("Hex-encoded: 512 characters + NUL = 513 bytes\n");
    printf("\n");
    
    // TX time calculation
    printf("TX Time Estimate\n");
    printf("================\n\n");
    int hex_chars = 258 * 2;  // 516 hex characters
    int total_bits = hex_chars * 6;  // Each hex char = 6 bits in 4FSK
    int total_seconds = (total_bits + 25) / 26;  // 26 bps, round up
    
    printf("Hex characters: %d\n", hex_chars);
    printf("Total bits: %d\n", total_bits);
    printf("TX time @ 26 bps: %d seconds (~%.1f minutes)\n", 
           total_seconds, total_seconds / 60.0);
    
    printf("\n");
    
    if (errors > 0) {
        printf("FAILED: %d error(s) found!\n", errors);
        return 1;
    } else {
        printf("SUCCESS: All sizes validated!\n");
        return 0;
    }
}
