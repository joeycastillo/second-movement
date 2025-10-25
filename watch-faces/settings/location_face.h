/*
 * MIT License
 *
 * Copyright (c) 2025 David Volovskiy
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

#ifndef LOCATION_FACE_H_
#define LOCATION_FACE_H_

#include "movement.h"

typedef enum {
    LOCATION_FACE_CITIES = 0,
    LOCATION_FACE_SETTING_LAT,
    LOCATION_FACE_SETTING_LONG,
    LOCATION_FACE_PAGES_COUNT,
} location_face_pages_t;

typedef struct {
    uint8_t sign: 1;    // 0-1
    uint8_t hundreds: 5;    // 0-1, ignored for latitude
    uint8_t tens: 5;        // 0-18 (wraps at 10 on clasic LCD, 18 on custom LCD)
    uint8_t ones: 4;        // 0-9 (must wrap at 10)
    uint8_t tenths: 4;      // 0-9 (must wrap at 10)
    uint8_t hundredths: 4;  // 0-9 (must wrap at 10)
} location_lat_lon_settings_t;

typedef struct {
    location_face_pages_t page;
    uint8_t city_idx;
    uint8_t active_digit;
    location_lat_lon_settings_t working_latitude;
    location_lat_lon_settings_t working_longitude;
} location_state_t;

void location_face_setup(uint8_t watch_face_index, void ** context_ptr);
void location_face_activate(void *context);
bool location_face_loop(movement_event_t event, void *context);
void location_face_resign(void *context);

#define location_face ((const watch_face_t){ \
    location_face_setup, \
    location_face_activate, \
    location_face_loop, \
    location_face_resign, \
    NULL, \
})

typedef struct {
    char name[26];
    int16_t latitude;
    int16_t longitude;
} location_long_lat_presets_t;

// Data came from here: https://worldpopulationreview.com/cities
// Removed all cities that are within 1000km of a more populated city
static const location_long_lat_presets_t locationLongLatPresets[] =
{
    // North America
    { .name = "Mexico City", .latitude = 1932, .longitude = -9915 },  // Mexico City, Mexico (2025 Population: 22752400)
    { .name = "New York City", .latitude = 4071, .longitude = -7401 },  // New York City, United States (2025 Population: 7936530)
    { .name = "Los Angeles", .latitude = 3405, .longitude = -11824 },  // Los Angeles, United States (2025 Population: 3770958)
    { .name = "Santo Domingo", .latitude = 1847, .longitude = -6989 },  // Santo Domingo, Dominican Republic (2025 Population: 3648110)
    { .name = "Guatemala City", .latitude = 1464, .longitude = -9051 },  // Guatemala City, Guatemala (2025 Population: 3229740)
    { .name = "Vancouver", .latitude = 4926, .longitude = -12311 },  // Vancouver, Canada (2025 Population: 2707920)
    { .name = "Chicago", .latitude = 4188, .longitude = -8762 },  // Chicago, United States (2025 Population: 2611867)
    { .name = "Houston", .latitude = 2976, .longitude = -9537 },  // Houston, United States (2025 Population: 2324082)
    { .name = "Havana", .latitude = 2314, .longitude = -8236 },  // Havana, Cuba (2025 Population: 2156350)
    { .name = "Ciudad Juarez", .latitude = 3174, .longitude = -10649 },  // Ciudad Juarez, Mexico (2025 Population: 1625980)
    { .name = "Winnipeg", .latitude = 4990, .longitude = -9714 },  // Winnipeg, Canada (2025 Population: 857367)
    // Asia
    { .name = "Tokyo", .latitude = 3568, .longitude = 13976 },  // Tokyo, Japan (2025 Population: 37036200)
    { .name = "Delhi", .latitude = 2863, .longitude = 7722 },  // Delhi, India (2025 Population: 34665600)
    { .name = "Shanghai", .latitude = 3123, .longitude = 12147 },  // Shanghai, China (2025 Population: 30482100)
    { .name = "Dhaka", .latitude = 2376, .longitude = 9039 },  // Dhaka, Bangladesh (2025 Population: 24652900)
    { .name = "Beijing", .latitude = 4019, .longitude = 11641 },  // Beijing, China (2025 Population: 22596500)
    { .name = "Mumbai", .latitude = 1905, .longitude = 7287 },  // Mumbai, India (2025 Population: 22089000)
    { .name = "Chongqing", .latitude = 3006, .longitude = 10787 },  // Chongqing, China (2025 Population: 18171200)
    { .name = "Istanbul", .latitude = 4101, .longitude = 2898 },  // Istanbul, Turkey (2025 Population: 16236700)
    { .name = "Manila", .latitude = 1459, .longitude = 12098 },  // Manila, Philippines (2025 Population: 15230600)
    { .name = "Shenzhen", .latitude = 2254, .longitude = 11405 },  // Shenzhen, China (2025 Population: 13545400)
    { .name = "Chennai", .latitude = 1308, .longitude = 8027 },  // Chennai, India (2025 Population: 12336000)
    { .name = "Jakarta", .latitude = -618, .longitude = 10683 },  // Jakarta, Indonesia (2025 Population: 11634100)
    { .name = "Bangkok", .latitude = 1375, .longitude = 10049 },  // Bangkok, Thailand (2025 Population: 11391700)
    { .name = "Tehran", .latitude = 3569, .longitude = 5139 },  // Tehran, Iran (2025 Population: 9729740)
    { .name = "Kuala Lumpur", .latitude = 315, .longitude = 10170 },  // Kuala Lumpur, Malaysia (2025 Population: 9000280)
    { .name = "Riyadh", .latitude = 2333, .longitude = 4533 },  // Riyadh, Saudi Arabia (2025 Population: 7952860)
    { .name = "Urumqi", .latitude = 4382, .longitude = 8761 },  // Urumqi, China (2025 Population: 5132170)
    { .name = "Kabul", .latitude = 3453, .longitude = 6919 },  // Kabul, Afghanistan (2025 Population: 4877020)
    { .name = "Dubai", .latitude = 2507, .longitude = 5519 },  // Dubai, United Arab Emirates (2025 Population: 3094640)
    { .name = "Makassar", .latitude = -513, .longitude = 11941 },  // Makassar, Indonesia (2025 Population: 1737390)
    { .name = "Ulaanbaatar", .latitude = 4792, .longitude = 10692 },  // Ulaanbaatar, Mongolia (2025 Population: 1724890)
    { .name = "Aden", .latitude = 1279, .longitude = 4503 },  // Aden, Yemen (2025 Population: 1154410)
    { .name = "Bishkek", .latitude = 4288, .longitude = 7460 },  // Bishkek, Kyrgyzstan (2025 Population: 1151220)
    { .name = "Diyarbakir", .latitude = 3792, .longitude = 4024 },  // Diyarbakir, Turkey (2025 Population: 1128360)
    { .name = "Jixi Heilongjiang", .latitude = 4530, .longitude = 13098 },  // Jixi Heilongjiang, China (2025 Population: 1024970)
    // Europe
    { .name = "Paris", .latitude = 4885, .longitude = 235 },  // Paris, France (2025 Population: 11346800)
    { .name = "Madrid", .latitude = 4042, .longitude = -370 },  // Madrid, Spain (2025 Population: 6810530)
    { .name = "Rome", .latitude = 4189, .longitude = 1248 },  // Rome, Italy (2025 Population: 4347100)
    { .name = "Warsaw", .latitude = 5223, .longitude = 2107 },  // Warsaw, Poland (2025 Population: 1800230)
    { .name = "Oslo", .latitude = 5991, .longitude = 1074 },  // Oslo, Norway (2025 Population: 1115580)
    // Africa
    { .name = "Cairo", .latitude = 3004, .longitude = 3124 },  // Cairo, Egypt (2025 Population: 23074200)
    { .name = "Luanda", .latitude = -883, .longitude = 1324 },  // Luanda, Angola (2025 Population: 10027900)
    { .name = "Dar es Salaam", .latitude = -682, .longitude = 3928 },  // Dar es Salaam, Tanzania (2025 Population: 8561520)
    { .name = "Khartoum", .latitude = 1550, .longitude = 3257 },  // Khartoum, Sudan (2025 Population: 6754180)
    { .name = "Johannesburg", .latitude = -2620, .longitude = 2805 },  // Johannesburg, South Africa (2025 Population: 6444580)
    { .name = "Cape Town", .latitude = -3393, .longitude = 1842 },  // Cape Town, South Africa (2025 Population: 5063580)
    { .name = "Yaounde", .latitude = 387, .longitude = 1152 },  // Yaounde, Cameroon (2025 Population: 4854260)
    { .name = "Kampala", .latitude = 32, .longitude = 3258 },  // Kampala, Uganda (2025 Population: 4265160)
    { .name = "Antananarivo", .latitude = -1891, .longitude = 4753 },  // Antananarivo, Madagascar (2025 Population: 4228980)
    { .name = "Dakar", .latitude = 1469, .longitude = -1745 },  // Dakar, Senegal (2025 Population: 3658640)
    { .name = "Ouagadougou", .latitude = 1237, .longitude = -153 },  // Ouagadougou, Burkina Faso (2025 Population: 3520820)
    { .name = "Lusaka", .latitude = -1542, .longitude = 2828 },  // Lusaka, Zambia (2025 Population: 3470870)
    { .name = "Mbuji-Mayi", .latitude = -613, .longitude = 2360 },  // Mbuji-Mayi, DR Congo (2025 Population: 3158340)
    { .name = "Mogadishu", .latitude = 203, .longitude = 4534 },  // Mogadishu, Somalia (2025 Population: 2846420)
    { .name = "Monrovia", .latitude = 633, .longitude = -1080 },  // Monrovia, Liberia (2025 Population: 1794650)
    { .name = "Tripoli", .latitude = 3290, .longitude = 1318 },  // Tripoli, Libya (2025 Population: 1203390)
    { .name = "Marrakech", .latitude = 3163, .longitude = -799 },  // Marrakech, Morocco (2025 Population: 1085330)
    // South America
    { .name = "Sao Paulo", .latitude = -2355, .longitude = -4663 },  // Sao Paulo, Brazil (2025 Population: 22990000)
    { .name = "Buenos Aires", .latitude = -3461, .longitude = -5839 },  // Buenos Aires, Argentina (2025 Population: 15752300)
    { .name = "Bogota", .latitude = 465, .longitude = -7408 },  // Bogota, Colombia (2025 Population: 11795800)
    { .name = "Lima", .latitude = -1205, .longitude = -7703 },  // Lima, Peru (2025 Population: 11517300)
    { .name = "Santiago", .latitude = -3344, .longitude = -7065 },  // Santiago, Chile (2025 Population: 6999460)
    { .name = "Brasilia", .latitude = -1033, .longitude = -5320 },  // Brasilia, Brazil (2025 Population: 4990930)
    { .name = "Recife", .latitude = -806, .longitude = -3488 },  // Recife, Brazil (2025 Population: 4344050)
    { .name = "Asuncion", .latitude = -2528, .longitude = -5763 },  // Asuncion, Paraguay (2025 Population: 3627220)
    { .name = "Guayaquil", .latitude = -229, .longitude = -8010 },  // Guayaquil, Ecuador (2025 Population: 3244750)
    { .name = "Belem", .latitude = -145, .longitude = -4847 },  // Belem, Brazil (2025 Population: 2453800)
    { .name = "Manaus", .latitude = -313, .longitude = -5998 },  // Manaus, Brazil (2025 Population: 2434640)
    { .name = "La Paz", .latitude = -1650, .longitude = -6813 },  // La Paz, Bolivia (2025 Population: 1997370)
    { .name = "Ciudad Guayana", .latitude = 832, .longitude = -6269 },  // Ciudad Guayana, Venezuela (2025 Population: 991388)
    // Oceania
    { .name = "Melbourne", .latitude = -3781, .longitude = 14496 },  // Melbourne, Australia (2025 Population: 5391890)
    { .name = "Brisbane", .latitude = -2747, .longitude = 15302 },  // Brisbane, Australia (2025 Population: 2568170)
    { .name = "Perth", .latitude = -3196, .longitude = 11586 },  // Perth, Australia (2025 Population: 2169190)
    { .name = "Auckland", .latitude = -3685, .longitude = 17476 },  // Auckland, New Zealand (2025 Population: 1711130)
};

#endif // LOCATION_FACE_H_
