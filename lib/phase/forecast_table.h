#ifndef FORECAST_TABLE_H_
#define FORECAST_TABLE_H_

#include "phase_engine.h"

#ifdef PHASE_ENGINE_ENABLED

// 7-day forecast table (populated at runtime)
extern homebase_entry_t forecast_table[7];
extern bool forecast_table_valid;
extern uint32_t forecast_last_update;

void forecast_table_init(void);
void forecast_table_update(const homebase_entry_t *data, uint8_t count);
const homebase_entry_t* forecast_get_entry(uint8_t day_offset);

#endif
#endif
