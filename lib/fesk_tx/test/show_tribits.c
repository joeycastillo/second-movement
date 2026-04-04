/*
 * Simple program to display FESK tribits for a given message
 */

#include <stdio.h>
#include <stdlib.h>
#include "../fesk_tx.h"

int main(void) {
    const char *message = "test";
    int8_t *sequence = NULL;
    size_t entries = 0;

    printf("Encoding message: \"%s\"\n\n", message);

    fesk_result_t result = fesk_encode(message, &sequence, &entries);

    if (result != FESK_OK) {
        printf("Error encoding: %d\n", result);
        return 1;
    }

    printf("Total entries: %zu\n", entries);
    printf("Total symbols (tribits): %zu\n\n", entries / 4);

    // Display the sequence in a readable format
    printf("Sequence breakdown:\n");
    for (size_t i = 0; i < entries; i += 4) {
        int8_t tone = sequence[i];
        int8_t tone_duration = sequence[i + 1];
        int8_t rest = sequence[i + 2];
        int8_t rest_duration = sequence[i + 3];

        // Find which tribit this tone represents
        int tribit = -1;
        for (int t = 0; t < FESK_4FSK_TONE_COUNT; t++) {
            if (fesk_tone_map_4fsk[t] == tone) {
                tribit = t;
                break;
            }
        }

        printf("Symbol %2zu: tribit=%d tone=%d duration=%d rest=%d duration=%d\n",
               i / 4, tribit, tone, tone_duration, rest, rest_duration);
    }

    printf("\nTribit sequence: ");
    for (size_t i = 0; i < entries; i += 4) {
        int8_t tone = sequence[i];
        for (int t = 0; t < FESK_4FSK_TONE_COUNT; t++) {
            if (fesk_tone_map_4fsk[t] == tone) {
                printf("%d ", t);
                break;
            }
        }
    }
    printf("\n");

    fesk_free_sequence(sequence);
    return 0;
}
