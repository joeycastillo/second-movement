/*
 * MIT License
 *
 * Copyright (c) 2026 Diego Perez
 *
 * Homebase Table Implementation
 */

#include "homebase.h"
#include "homebase_table.h"

#ifdef PHASE_ENGINE_ENABLED

const homebase_entry_t* homebase_get_entry(uint16_t day_of_year) {
    // Clamp to valid range (1-365)
    if (day_of_year < 1) day_of_year = 1;
    if (day_of_year > 365) day_of_year = 365;
    
    // Array is 0-indexed, day_of_year is 1-indexed
    return &homebase_table[day_of_year - 1];
}

const homebase_metadata_t* homebase_get_metadata(void) {
    return &homebase_metadata;
}

#endif // PHASE_ENGINE_ENABLED
