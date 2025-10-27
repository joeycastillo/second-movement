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
#include "zones.h"

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
    uint8_t set_city_idx;
    uint8_t active_digit;
    location_lat_lon_settings_t working_latitude;
    location_lat_lon_settings_t working_longitude;
    bool quick_ticks_running;
    bool curr_tz_has_no_cities;
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
    int8_t region;
    uint8_t timezone;
} location_long_lat_presets_t;

// Data came from here: https://worldpopulationreview.com/cities
// Removed all cities that are within 750km of a more populated city. 113 cities total.
static const location_long_lat_presets_t locationLongLatPresets[] =
{
    // North America
    { .name = "Chicago", .latitude = 4188, .longitude = -8762, .region = 0 , .timezone = UTZ_CHICAGO },  // Chicago, United States (2025 Population: 2611867)
    { .name = "Ciudad Juarez", .latitude = 3174, .longitude = -10649, .region = 0 , .timezone = UTZ_DENVER },  // Ciudad Juarez, Mexico (2025 Population: 1625980)
    { .name = "Culiacan", .latitude = 2480, .longitude = -10739, .region = 0 , .timezone = UTZ_PHOENIX },  // Culiacan, Mexico (2025 Population: 918494)
    { .name = "Detroit", .latitude = 4233, .longitude = -8304, .region = 0 , .timezone = UTZ_NEW_YORK },  // Detroit, United States (2025 Population: 645705)
    { .name = "Edmonton", .latitude = 5355, .longitude = -11349, .region = 0 , .timezone = UTZ_DENVER },  // Edmonton, Canada (2025 Population: 1588600)
    { .name = "Guatemala City", .latitude = 1464, .longitude = -9051, .region = 0 , .timezone = UTZ_CHICAGO },  // Guatemala City, Guatemala (2025 Population: 3229740)
    { .name = "Havana", .latitude = 2314, .longitude = -8236, .region = 0 , .timezone = UTZ_NEW_YORK  },  // Havana, Cuba (2025 Population: 2156350)
    { .name = "Houston", .latitude = 2976, .longitude = -9537, .region = 0 , .timezone = UTZ_CHICAGO },  // Houston, United States (2025 Population: 2324082)
    { .name = "Jacksonville", .latitude = 3033, .longitude = -8166, .region = 0 , .timezone = UTZ_NEW_YORK },  // Jacksonville, United States (2025 Population: 1008485)
    { .name = "Los Angeles", .latitude = 3405, .longitude = -11824, .region = 0 , .timezone = UTZ_LOS_ANGELES },  // Los Angeles, United States (2025 Population: 3770958)
    { .name = "Mexico City", .latitude = 1932, .longitude = -9915, .region = 0 , .timezone = UTZ_CHICAGO },  // Mexico City, Mexico (2025 Population: 22752400)
    { .name = "New York City", .latitude = 4071, .longitude = -7401, .region = 0 , .timezone = UTZ_NEW_YORK },  // New York City, United States (2025 Population: 7936530)
    { .name = "Panama City", .latitude = 897, .longitude = -7953, .region = 0 , .timezone = UTZ_CHICAGO  },  // Panama City, Panama (2025 Population: 2054540)
    { .name = "Raleigh", .latitude = 3578, .longitude = -7864, .region = 0 , .timezone = UTZ_NEW_YORK },  // Raleigh, United States (2025 Population: 493589)
    { .name = "Santo Domingo", .latitude = 1847, .longitude = -6989, .region = 0 , .timezone = UTZ_CHICAGO  },  // Santo Domingo, Dominican Republic (2025 Population: 3648110)
    { .name = "Vancouver", .latitude = 4926, .longitude = -12311, .region = 0 , .timezone = UTZ_LOS_ANGELES },  // Vancouver, Canada (2025 Population: 2707920)
    { .name = "Winnipeg", .latitude = 4990, .longitude = -9714, .region = 0 , .timezone = UTZ_CHICAGO },  // Winnipeg, Canada (2025 Population: 857367)
    // Asia
    { .name = "Aleppo", .latitude = 3620, .longitude = 3716, .region = 1 , .timezone = UTZ_MOSCOW },  // Aleppo, Syria (2025 Population: 2438480)
    { .name = "Astana", .latitude = 5113, .longitude = 7143, .region = 1 , .timezone = UTZ_KOLKATA  },  // Astana, Kazakhstan (2025 Population: 1352560)
    { .name = "Bangalore", .latitude = 1298, .longitude = 7759, .region = 1 , .timezone = UTZ_KOLKATA },  // Bangalore, India (2025 Population: 14395400)
    { .name = "Bangkok", .latitude = 1375, .longitude = 10049, .region = 1 , .timezone = UTZ_BANGKOK },  // Bangkok, Thailand (2025 Population: 11391700)
    { .name = "Beijing", .latitude = 4019, .longitude = 11641, .region = 1 , .timezone = UTZ_SINGAPORE },  // Beijing, China (2025 Population: 22596500)
    { .name = "Bien Hoa", .latitude = 1095, .longitude = 10683, .region = 1 , .timezone = UTZ_BANGKOK },  // Bien Hoa, Vietnam (2025 Population: 1174750)
    { .name = "Chongqing", .latitude = 3006, .longitude = 10787, .region = 1 , .timezone = UTZ_SINGAPORE },  // Chongqing, China (2025 Population: 18171200)
    { .name = "Daqing", .latitude = 4632, .longitude = 12456, .region = 1 , .timezone = UTZ_SINGAPORE },  // Daqing, China (2025 Population: 2085470)
    { .name = "Davao City", .latitude = 706, .longitude = 12561, .region = 1 , .timezone = UTZ_SINGAPORE },  // Davao City, Philippines (2025 Population: 2033990)
    { .name = "Delhi", .latitude = 2863, .longitude = 7722, .region = 1 , .timezone = UTZ_KOLKATA },  // Delhi, India (2025 Population: 34665600)
    { .name = "Dhaka", .latitude = 2376, .longitude = 9039, .region = 1 , .timezone = UTZ_KOLKATA  },  // Dhaka, Bangladesh (2025 Population: 24652900)
    { .name = "Dubai", .latitude = 2507, .longitude = 5519, .region = 1 , .timezone = UTZ_DUBAI },  // Dubai, United Arab Emirates (2025 Population: 3094640)
    { .name = "Guangzhou", .latitude = 2313, .longitude = 11326, .region = 1 , .timezone = UTZ_SINGAPORE },  // Guangzhou, China (2025 Population: 14878700)
    { .name = "Hanoi", .latitude = 2103, .longitude = 10585, .region = 1 , .timezone = UTZ_BANGKOK },  // Hanoi, Vietnam (2025 Population: 5602200)
    { .name = "Istanbul", .latitude = 4101, .longitude = 2898, .region = 1 , .timezone = UTZ_MOSCOW },  // Istanbul, Turkey (2025 Population: 16236700)
    { .name = "Jakarta", .latitude = -618, .longitude = 10683, .region = 1 , .timezone = UTZ_BANGKOK },  // Jakarta, Indonesia (2025 Population: 11634100)
    { .name = "Kabul", .latitude = 3453, .longitude = 6919, .region = 1 , .timezone = UTZ_KATHMANDU  },  // Kabul, Afghanistan (2025 Population: 4877020)
    { .name = "Karachi", .latitude = 2485, .longitude = 6702, .region = 1 , .timezone = UTZ_KOLKATA  },  // Karachi, Pakistan (2025 Population: 18076800)
    { .name = "Kuala Lumpur", .latitude = 315, .longitude = 10170, .region = 1 , .timezone = UTZ_SINGAPORE },  // Kuala Lumpur, Malaysia (2025 Population: 9000280)
    { .name = "Lanzhou", .latitude = 3647, .longitude = 10373, .region = 1 , .timezone = UTZ_SINGAPORE },  // Lanzhou, China (2025 Population: 3430880)
    { .name = "Makassar", .latitude = -513, .longitude = 11941, .region = 1 , .timezone = UTZ_SINGAPORE },  // Makassar, Indonesia (2025 Population: 1737390)
    { .name = "Manila", .latitude = 1459, .longitude = 12098, .region = 1 , .timezone = UTZ_SINGAPORE },  // Manila, Philippines (2025 Population: 15230600)
    { .name = "Mumbai", .latitude = 1905, .longitude = 7287, .region = 1 , .timezone = UTZ_KOLKATA },  // Mumbai, India (2025 Population: 22089000)
    { .name = "Najaf", .latitude = 3200, .longitude = 4433, .region = 1 , .timezone = UTZ_MOSCOW },  // Najaf, Iraq (2025 Population: 1017660)
    { .name = "Riyadh", .latitude = 2333, .longitude = 4533, .region = 1 , .timezone = UTZ_MOSCOW },  // Riyadh, Saudi Arabia (2025 Population: 7952860)
    { .name = "Sanaa", .latitude = 1535, .longitude = 4420, .region = 1 , .timezone = UTZ_MOSCOW },  // Sanaa, Yemen (2025 Population: 3527430)
    { .name = "Sapporo", .latitude = 4306, .longitude = 14135, .region = 1 , .timezone = UTZ_TOKYO },  // Sapporo, Japan (2025 Population: 2653580)
    { .name = "Seoul", .latitude = 3757, .longitude = 12698, .region = 1 , .timezone = UTZ_TOKYO },  // Seoul, South Korea (2025 Population: 10025800)
    { .name = "Shanghai", .latitude = 3123, .longitude = 12147, .region = 1 , .timezone = UTZ_SINGAPORE },  // Shanghai, China (2025 Population: 30482100)
    { .name = "Taizhong", .latitude = 2416, .longitude = 12065, .region = 1 , .timezone = UTZ_SINGAPORE },  // Taizhong, Taiwan (2025 Population: 1393190)
    { .name = "Tashkent", .latitude = 4131, .longitude = 6928, .region = 1 , .timezone = UTZ_KOLKATA  },  // Tashkent, Uzbekistan (2025 Population: 2665080)
    { .name = "Tehran", .latitude = 3569, .longitude = 5139, .region = 1 , .timezone = UTZ_TEHRAN },  // Tehran, Iran (2025 Population: 9729740)
    { .name = "Tokyo", .latitude = 3568, .longitude = 13976, .region = 1 , .timezone = UTZ_TOKYO },  // Tokyo, Japan (2025 Population: 37036200)
    { .name = "Ulaanbaatar", .latitude = 4792, .longitude = 10692, .region = 1 , .timezone = UTZ_SINGAPORE },  // Ulaanbaatar, Mongolia (2025 Population: 1724890)
    { .name = "Urumqi", .latitude = 4382, .longitude = 8761, .region = 1 , .timezone = UTZ_KOLKATA  },  // Urumqi, China (2025 Population: 5132170)
    { .name = "Visakhapatnam", .latitude = 1769, .longitude = 8329, .region = 1 , .timezone = UTZ_KOLKATA },  // Visakhapatnam, India (2025 Population: 2440420)
    { .name = "Yerevan", .latitude = 4018, .longitude = 4451, .region = 1 , .timezone = UTZ_DUBAI },  // Yerevan, Armenia (2025 Population: 1100240)
    // Europe
    { .name = "Berlin", .latitude = 5252, .longitude = 1340, .region = 2 , .timezone = UTZ_BERLIN  },  // Berlin, Germany (2025 Population: 3580190)
    { .name = "Glasgow", .latitude = 5586, .longitude = -425, .region = 2 , .timezone = UTZ_LONDON },  // Glasgow, United Kingdom (2025 Population: 1718940)
    { .name = "Madrid", .latitude = 4042, .longitude = -370, .region = 2 , .timezone = UTZ_BERLIN  },  // Madrid, Spain (2025 Population: 6810530)
    { .name = "Paris", .latitude = 4885, .longitude = 235, .region = 2 , .timezone = UTZ_BERLIN  },  // Paris, France (2025 Population: 11346800)
    { .name = "Rome", .latitude = 4189, .longitude = 1248, .region = 2 , .timezone = UTZ_BERLIN  },  // Rome, Italy (2025 Population: 4347100)
    { .name = "Stockholm", .latitude = 5933, .longitude = 1807, .region = 2 , .timezone = UTZ_BERLIN  },  // Stockholm, Sweden (2025 Population: 1737760)
    // Africa
    { .name = "Abidjan", .latitude = 532, .longitude = -402, .region = 3 , .timezone = UTZ_UTC  },  // Abidjan, Ivory Coast (2025 Population: 6056880)
    { .name = "Addis Ababa", .latitude = 904, .longitude = 3875, .region = 3 , .timezone = UTZ_MOSCOW },  // Addis Ababa, Ethiopia (2025 Population: 5956680)
    { .name = "Antananarivo", .latitude = -1891, .longitude = 4753, .region = 3 , .timezone = UTZ_MOSCOW },  // Antananarivo, Madagascar (2025 Population: 4228980)
    { .name = "Banghazi", .latitude = 3212, .longitude = 2009, .region = 3 , .timezone = UTZ_MAPUTO  },  // Banghazi, Libya (2025 Population: 881897)
    { .name = "Bangui", .latitude = 436, .longitude = 1858, .region = 3 , .timezone = UTZ_LAGOS },  // Bangui, Central African Republic (2025 Population: 1016150)
    { .name = "Buffalo City", .latitude = -3298, .longitude = 2768, .region = 3 , .timezone = UTZ_MAPUTO  },  // Buffalo City, South Africa (2025 Population: 756507)
    { .name = "Cairo", .latitude = 3004, .longitude = 3124, .region = 3 , .timezone = UTZ_CAIRO },  // Cairo, Egypt (2025 Population: 23074200)
    { .name = "Cape Town", .latitude = -3393, .longitude = 1842, .region = 3 , .timezone = UTZ_MAPUTO  },  // Cape Town, South Africa (2025 Population: 5063580)
    { .name = "Casablanca", .latitude = 3359, .longitude = -762, .region = 3 , .timezone = UTZ_LONDON  },  // Casablanca, Morocco (2025 Population: 4012310)
    { .name = "Dakar", .latitude = 1469, .longitude = -1745, .region = 3 , .timezone = UTZ_UTC  },  // Dakar, Senegal (2025 Population: 3658640)
    { .name = "Dar es Salaam", .latitude = -682, .longitude = 3928, .region = 3 , .timezone = UTZ_MOSCOW },  // Dar es Salaam, Tanzania (2025 Population: 8561520)
    { .name = "Johannesburg", .latitude = -2620, .longitude = 2805, .region = 3 , .timezone = UTZ_MAPUTO  },  // Johannesburg, South Africa (2025 Population: 6444580)
    { .name = "Kampala", .latitude = 32, .longitude = 3258, .region = 3 , .timezone = UTZ_MOSCOW },  // Kampala, Uganda (2025 Population: 4265160)
    { .name = "Kano", .latitude = 1199, .longitude = 852, .region = 3 , .timezone = UTZ_LAGOS },  // Kano, Nigeria (2025 Population: 4645320)
    { .name = "Khartoum", .latitude = 1550, .longitude = 3257, .region = 3 , .timezone = UTZ_MAPUTO  },  // Khartoum, Sudan (2025 Population: 6754180)
    { .name = "Kinshasa", .latitude = -430, .longitude = 1531, .region = 3 , .timezone = UTZ_LAGOS },  // Kinshasa, DR Congo (2025 Population: 17778500)
    { .name = "Kisangani", .latitude = 52, .longitude = 2521, .region = 3 , .timezone = UTZ_MAPUTO  },  // Kisangani, DR Congo (2025 Population: 1546690)
    { .name = "Lagos", .latitude = 646, .longitude = 339, .region = 3 , .timezone = UTZ_LAGOS },  // Lagos, Nigeria (2025 Population: 17156400)
    { .name = "Lubango", .latitude = -1492, .longitude = 1349, .region = 3 , .timezone = UTZ_LAGOS },  // Lubango, Angola (2025 Population: 1047810)
    { .name = "Lusaka", .latitude = -1542, .longitude = 2828, .region = 3 , .timezone = UTZ_MAPUTO  },  // Lusaka, Zambia (2025 Population: 3470870)
    { .name = "Mbuji-Mayi", .latitude = -613, .longitude = 2360, .region = 3 , .timezone = UTZ_MAPUTO  },  // Mbuji-Mayi, DR Congo (2025 Population: 3158340)
    { .name = "Mogadishu", .latitude = 203, .longitude = 4534, .region = 3 , .timezone = UTZ_MOSCOW },  // Mogadishu, Somalia (2025 Population: 2846420)
    { .name = "Monrovia", .latitude = 633, .longitude = -1080, .region = 3 , .timezone = UTZ_UTC  },  // Monrovia, Liberia (2025 Population: 1794650)
    { .name = "Nampula", .latitude = -1497, .longitude = 3927, .region = 3 , .timezone = UTZ_MAPUTO  },  // Nampula, Mozambique (2025 Population: 1057290)
    { .name = "Nyala", .latitude = 1228, .longitude = 2477, .region = 3 , .timezone = UTZ_MAPUTO  },  // Nyala, Sudan (2025 Population: 1145590)
    { .name = "Ouagadougou", .latitude = 1237, .longitude = -153, .region = 3 , .timezone = UTZ_UTC  },  // Ouagadougou, Burkina Faso (2025 Population: 3520820)
    { .name = "Tunis", .latitude = 3384, .longitude = 940, .region = 3 , .timezone = UTZ_LAGOS },  // Tunis, Tunisia (2025 Population: 2545030)
    { .name = "Yaounde", .latitude = 387, .longitude = 1152, .region = 3 , .timezone = UTZ_LAGOS },  // Yaounde, Cameroon (2025 Population: 4854260)
    // South America
    { .name = "Asuncion", .latitude = -2528, .longitude = -5763, .region = 4 , .timezone = UTZ_SAO_PAULO },  // Asuncion, Paraguay (2025 Population: 3627220)
    { .name = "Belem", .latitude = -145, .longitude = -4847, .region = 4 , .timezone = UTZ_SAO_PAULO },  // Belem, Brazil (2025 Population: 2453800)
    { .name = "Bogota", .latitude = 465, .longitude = -7408, .region = 4 , .timezone = UTZ_CHICAGO  },  // Bogota, Colombia (2025 Population: 11795800)
    { .name = "Brasilia", .latitude = -1033, .longitude = -5320, .region = 4 , .timezone = UTZ_CHICAGO  },  // Brasilia, Brazil (2025 Population: 4990930)
    { .name = "Buenos Aires", .latitude = -3461, .longitude = -5839, .region = 4 , .timezone = UTZ_SAO_PAULO },  // Buenos Aires, Argentina (2025 Population: 15752300)
    { .name = "Caracas", .latitude = 1051, .longitude = -6691, .region = 4 , .timezone = UTZ_CHICAGO  },  // Caracas, Venezuela (2025 Population: 3015110)
    { .name = "Goiania", .latitude = -1668, .longitude = -4925, .region = 4 , .timezone = UTZ_SAO_PAULO },  // Goiania, Brazil (2025 Population: 2927080)
    { .name = "Guayaquil", .latitude = -229, .longitude = -8010, .region = 4 , .timezone = UTZ_CHICAGO  },  // Guayaquil, Ecuador (2025 Population: 3244750)
    { .name = "La Paz", .latitude = -1650, .longitude = -6813, .region = 4 , .timezone = UTZ_CHICAGO  },  // La Paz, Bolivia (2025 Population: 1997370)
    { .name = "Lima", .latitude = -1205, .longitude = -7703, .region = 4 , .timezone = UTZ_CHICAGO  },  // Lima, Peru (2025 Population: 11517300)
    { .name = "Manaus", .latitude = -313, .longitude = -5998, .region = 4 , .timezone = UTZ_CHICAGO  },  // Manaus, Brazil (2025 Population: 2434640)
    { .name = "Porto Alegre", .latitude = -3003, .longitude = -5123, .region = 4 , .timezone = UTZ_SAO_PAULO },  // Porto Alegre, Brazil (2025 Population: 4268960)
    { .name = "Recife", .latitude = -806, .longitude = -3488, .region = 4 , .timezone = UTZ_SAO_PAULO },  // Recife, Brazil (2025 Population: 4344050)
    { .name = "San Miguel de Tucuman", .latitude = -2683, .longitude = -6520, .region = 4 , .timezone = UTZ_SAO_PAULO },  // San Miguel de Tucuman, Argentina (2025 Population: 1051040)
    { .name = "Santiago", .latitude = -3344, .longitude = -7065, .region = 4 , .timezone = UTZ_SANTIAGO },  // Santiago, Chile (2025 Population: 6999460)
    { .name = "Sao Paulo", .latitude = -2355, .longitude = -4663, .region = 4 , .timezone = UTZ_SAO_PAULO },  // Sao Paulo, Brazil (2025 Population: 22990000)
    // Oceania
    { .name = "Auckland", .latitude = -3685, .longitude = 17476, .region = 5 , .timezone = UTZ_AUCKLAND },  // Auckland, New Zealand (2025 Population: 1711130)
    { .name = "Brisbane", .latitude = -2747, .longitude = 15302, .region = 5 , .timezone = UTZ_BRISBANE },  // Brisbane, Australia (2025 Population: 2568170)
    { .name = "Melbourne", .latitude = -3781, .longitude = 14496, .region = 5 , .timezone = UTZ_SYDNEY },  // Melbourne, Australia (2025 Population: 5391890)
    { .name = "Perth", .latitude = -3196, .longitude = 11586, .region = 5 , .timezone = UTZ_SINGAPORE },  // Perth, Australia (2025 Population: 2169190)
};

#endif // LOCATION_FACE_H_
