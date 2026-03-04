#include "forecast_table.h"
#include <string.h>

#ifdef PHASE_ENGINE_ENABLED

homebase_entry_t forecast_table[7];
bool forecast_table_valid = false;
uint32_t forecast_last_update = 0;

void forecast_table_init(void) {
    memset(forecast_table, 0, sizeof(forecast_table));
    forecast_table_valid = false;
    forecast_last_update = 0;
}

void forecast_table_update(const homebase_entry_t *data, uint8_t count) {
    if (count > 7) count = 7;
    memcpy(forecast_table, data, count * sizeof(homebase_entry_t));
    forecast_table_valid = (count == 7);
}

const homebase_entry_t* forecast_get_entry(uint8_t day_offset) {
    if (day_offset >= 7 || !forecast_table_valid) {
        return NULL;
    }
    return &forecast_table[day_offset];
}

#endif
