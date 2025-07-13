#include "location.h"
#include "filesystem.h"

void location_persist(movement_location_t location) {
    filesystem_write_file("location.bin", (char *)&location.reg, sizeof(uint32_t));
}

movement_location_t location_load(void) {
    movement_location_t location = {0};
    filesystem_read_file("location.bin", (char *)&location.reg, sizeof(uint32_t));
    return location;
}