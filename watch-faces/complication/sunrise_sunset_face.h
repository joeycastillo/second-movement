





/*
 * MIT License
 *
 * Copyright (c) 2022 Joey Castillo
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

#ifndef SUNRISE_SUNSET_FACE_H_
#define SUNRISE_SUNSET_FACE_H_

/*
 * SUNRISE & SUNSET FACE
 *
 * The Sunrise/Sunset face is designed to display the next sunrise or sunset
 * for a given location. It also functions as an interface for setting the
 * location register, which other watch faces can use for various purposes.
 *
 * Refer to the wiki for usage instructions:
 *  https://www.sensorwatch.net/docs/watchfaces/complication/#sunrisesunset
 */

#include "movement.h"
#include "zones.h"

typedef enum {
    SUNRISE_SUNSET_FACE_RISE_SET_TIMES = 0,
    SUNRISE_SUNSET_FACE_CITIES,
    SUNRISE_SUNSET_FACE_SETTING_LAT,
    SUNRISE_SUNSET_FACE_SETTING_LONG,
    SUNRISE_SUNSET_FACE_PAGES_COUNT,
} sunrise_sunset_pages_t;

typedef struct {
    uint8_t sign: 1;    // 0-1
    uint8_t hundreds: 5;    // 0-1, ignored for latitude
    uint8_t tens: 5;        // 0-18 (wraps at 10 on clasic LCD, 18 on custom LCD)
    uint8_t ones: 4;        // 0-9 (must wrap at 10)
    uint8_t tenths: 4;      // 0-9 (must wrap at 10)
    uint8_t hundredths: 4;  // 0-9 (must wrap at 10)
} sunrise_sunset_lat_lon_settings_t;

typedef struct {
    char name[6];
    int16_t latitude;
    int16_t longitude;
    int8_t region;
    uint8_t timezone;
} sunrise_sunset_long_lat_presets_t;

typedef struct {
    sunrise_sunset_pages_t page;
    uint8_t rise_index;
    uint8_t active_digit;
    bool location_changed;
    watch_date_time_t rise_set_expires;
    sunrise_sunset_lat_lon_settings_t working_latitude;
    sunrise_sunset_lat_lon_settings_t working_longitude;
    uint8_t longLatToUse;
    uint8_t city_idx;
    uint8_t set_city_idx;
    bool quick_ticks_running;
    bool curr_tz_has_cities;
} sunrise_sunset_state_t;

void sunrise_sunset_face_setup(uint8_t watch_face_index, void ** context_ptr);
void sunrise_sunset_face_activate(void *context);
bool sunrise_sunset_face_loop(movement_event_t event, void *context);
void sunrise_sunset_face_resign(void *context);

#define sunrise_sunset_face ((const watch_face_t){ \
    sunrise_sunset_face_setup, \
    sunrise_sunset_face_activate, \
    sunrise_sunset_face_loop, \
    sunrise_sunset_face_resign, \
    NULL, \
})

typedef struct {
    char name[3];
    int16_t latitude;
    int16_t longitude;
    uint8_t timezone;
} sunrise_sunset_alt_location_presets_t;

static const sunrise_sunset_alt_location_presets_t sunriseSunsetAltLocationPresets[] =
{
    { .name = "  "},  // Default, the long and lat get replaced by what's set in the watch
//    { .name = "Ny", .latitude = 4072, .longitude = -7401, .timezone = UTZ_NEW_YORK },  // New York City, NY
//    { .name = "LA", .latitude = 3405, .longitude = -11824, .timezone = UTZ_LOS_ANGELES },  // Los Angeles, CA
//    { .name = "dE", .latitude = 4221, .longitude = -8305, .timezone = UTZ_NEW_YORK },  // Detroit, MI
};

// Data came from here: https://worldpopulationreview.com/cities
// All cities have a 2025 population of at least 1000000 and removed all cities that are within 500km of a more populated city that shares a timezone. 190 cities total.
static const sunrise_sunset_long_lat_presets_t sunriseSunsetLongLatPresets[] =
{
    // North America
    { .name = "Calgar", .latitude = 5105, .longitude = -11406, .region = 0, .timezone = UTZ_DENVER },  // Calgary, Canada (Region: North America, 2025 Population: 1687900)
    { .name = "Cancun", .latitude = 2115, .longitude = -8684, .region = 0, .timezone = UTZ_CHICAGO  },  // Cancun, Mexico (Region: North America, 2025 Population: 1065400)
    { .name = "Chicag", .latitude = 4188, .longitude = -8762, .region = 0, .timezone = UTZ_CHICAGO },  // Chicago, United States (Region: North America, 2025 Population: 2611867)
    { .name = "Chihua", .latitude = 2850, .longitude = -10600, .region = 0, .timezone = UTZ_CHICAGO },  // Chihuahua, Mexico (Region: North America, 2025 Population: 1153540)
    { .name = "Ciudad", .latitude = 3174, .longitude = -10649, .region = 0, .timezone = UTZ_DENVER },  // Ciudad Juarez, Mexico (Region: North America, 2025 Population: 1625980)
    { .name = "Detroi", .latitude = 4233, .longitude = -8304, .region = 0, .timezone = UTZ_NEW_YORK },  // Detroit, United States (Region: North America, 2025 Population: 645705)
    { .name = "Guatem", .latitude = 1464, .longitude = -9051, .region = 0, .timezone = UTZ_CHICAGO },  // Guatemala City, Guatemala (Region: North America, 2025 Population: 3229740)
    { .name = "Havana", .latitude = 2314, .longitude = -8236, .region = 0, .timezone = UTZ_NEW_YORK  },  // Havana, Cuba (Region: North America, 2025 Population: 2156350)
    { .name = "Housto", .latitude = 2976, .longitude = -9537, .region = 0, .timezone = UTZ_CHICAGO },  // Houston, United States (Region: North America, 2025 Population: 2324082)
    { .name = "Jackso", .latitude = 3033, .longitude = -8166, .region = 0, .timezone = UTZ_NEW_YORK },  // Jacksonville, United States (Region: North America, 2025 Population: 1008485)
    { .name = "Los An", .latitude = 3405, .longitude = -11824, .region = 0, .timezone = UTZ_LOS_ANGELES },  // Los Angeles, United States (Region: North America, 2025 Population: 3770958)
    { .name = "Managu", .latitude = 1216, .longitude = -8627, .region = 0, .timezone = UTZ_CHICAGO },  // Managua, Nicaragua (Region: North America, 2025 Population: 1120900)
    { .name = "Merida", .latitude = 2097, .longitude = -8962, .region = 0, .timezone = UTZ_CHICAGO },  // Merida, Mexico (Region: North America, 2025 Population: 1258230)
    { .name = "Mexico", .latitude = 1932, .longitude = -9915, .region = 0, .timezone = UTZ_CHICAGO },  // Mexico City, Mexico (Region: North America, 2025 Population: 22752400)
    { .name = "Monter", .latitude = 2568, .longitude = -10032, .region = 0, .timezone = UTZ_CHICAGO },  // Monterrey, Mexico (Region: North America, 2025 Population: 5272360)
    { .name = "Montre", .latitude = 4550, .longitude = -7357, .region = 0, .timezone = UTZ_NEW_YORK },  // Montreal, Canada (Region: North America, 2025 Population: 4377310)
    { .name = "New Yo", .latitude = 4071, .longitude = -7401, .region = 0, .timezone = UTZ_NEW_YORK },  // New York City, United States (Region: North America, 2025 Population: 7936530)
    { .name = "Panama", .latitude = 897, .longitude = -7953, .region = 0, .timezone = UTZ_CHICAGO  },  // Panama City, Panama (Region: North America, 2025 Population: 2054540)
    { .name = "Phoeni", .latitude = 3345, .longitude = -11207, .region = 0, .timezone = UTZ_PHOENIX },  // Phoenix, United States (Region: North America, 2025 Population: 1675144)
    { .name = "Port-a", .latitude = 1855, .longitude = -7234, .region = 0, .timezone = UTZ_NEW_YORK },  // Port-au-Prince, Haiti (Region: North America, 2025 Population: 3133080)
    { .name = "Raleig", .latitude = 3578, .longitude = -7864, .region = 0, .timezone = UTZ_NEW_YORK },  // Raleigh, United States (Region: North America, 2025 Population: 493589)
    { .name = "Santo ", .latitude = 1847, .longitude = -6989, .region = 0, .timezone = UTZ_CHICAGO  },  // Santo Domingo, Dominican Republic (Region: North America, 2025 Population: 3648110)
    { .name = "Toront", .latitude = 4365, .longitude = -7938, .region = 0, .timezone = UTZ_NEW_YORK },  // Toronto, Canada (Region: North America, 2025 Population: 6491290)
    { .name = "Vancou", .latitude = 4926, .longitude = -12311, .region = 0, .timezone = UTZ_LOS_ANGELES },  // Vancouver, Canada (Region: North America, 2025 Population: 2707920)
    // Asia
    { .name = "Almaty", .latitude = 4324, .longitude = 7695, .region = 1, .timezone = UTZ_KOLKATA  },  // Almaty, Kazakhstan (Region: Asia, 2025 Population: 2042040)
    { .name = "Astana", .latitude = 5113, .longitude = 7143, .region = 1, .timezone = UTZ_KOLKATA  },  // Astana, Kazakhstan (Region: Asia, 2025 Population: 1352560)
    { .name = "Baghda", .latitude = 3331, .longitude = 4439, .region = 1, .timezone = UTZ_MOSCOW },  // Baghdad, Iraq (Region: Asia, 2025 Population: 8141120)
    { .name = "Baku", .latitude = 4038, .longitude = 4983, .region = 1, .timezone = UTZ_DUBAI },  // Baku, Azerbaijan (Region: Asia, 2025 Population: 2496500)
    { .name = "Bangal", .latitude = 1298, .longitude = 7759, .region = 1, .timezone = UTZ_KOLKATA },  // Bangalore, India (Region: Asia, 2025 Population: 14395400)
    { .name = "Bangko", .latitude = 1375, .longitude = 10049, .region = 1, .timezone = UTZ_BANGKOK },  // Bangkok, Thailand (Region: Asia, 2025 Population: 11391700)
    { .name = "Baotou", .latitude = 4062, .longitude = 10994, .region = 1, .timezone = UTZ_SINGAPORE },  // Baotou, China (Region: Asia, 2025 Population: 2425700)
    { .name = "Batam", .latitude = 110, .longitude = 10404, .region = 1, .timezone = UTZ_BANGKOK },  // Batam, Indonesia (Region: Asia, 2025 Population: 1858910)
    { .name = "Beijin", .latitude = 4019, .longitude = 11641, .region = 1, .timezone = UTZ_SINGAPORE },  // Beijing, China (Region: Asia, 2025 Population: 22596500)
    { .name = "Beirut", .latitude = 3389, .longitude = 3550, .region = 1, .timezone = UTZ_JERUSALEM  },  // Beirut, Lebanon (Region: Asia, 2025 Population: 2379330)
    { .name = "Chiang", .latitude = 1879, .longitude = 9899, .region = 1, .timezone = UTZ_BANGKOK },  // Chiang Mai, Thailand (Region: Asia, 2025 Population: 1244190)
    { .name = "Chongq", .latitude = 3006, .longitude = 10787, .region = 1, .timezone = UTZ_SINGAPORE },  // Chongqing, China (Region: Asia, 2025 Population: 18171200)
    { .name = "Da Nan", .latitude = 1607, .longitude = 10821, .region = 1, .timezone = UTZ_BANGKOK },  // Da Nang, Vietnam (Region: Asia, 2025 Population: 1286000)
    { .name = "Damasc", .latitude = 3351, .longitude = 3631, .region = 1, .timezone = UTZ_MOSCOW },  // Damascus, Syria (Region: Asia, 2025 Population: 2799960)
    { .name = "Daqing", .latitude = 4632, .longitude = 12456, .region = 1, .timezone = UTZ_SINGAPORE },  // Daqing, China (Region: Asia, 2025 Population: 2085470)
    { .name = "Davao ", .latitude = 706, .longitude = 12561, .region = 1, .timezone = UTZ_SINGAPORE },  // Davao City, Philippines (Region: Asia, 2025 Population: 2033990)
    { .name = "Delhi", .latitude = 2863, .longitude = 7722, .region = 1, .timezone = UTZ_KOLKATA },  // Delhi, India (Region: Asia, 2025 Population: 34665600)
    { .name = "Denpas", .latitude = -865, .longitude = 11522, .region = 1, .timezone = UTZ_SINGAPORE },  // Denpasar, Indonesia (Region: Asia, 2025 Population: 1097310)
    { .name = "Dhaka", .latitude = 2376, .longitude = 9039, .region = 1, .timezone = UTZ_KOLKATA  },  // Dhaka, Bangladesh (Region: Asia, 2025 Population: 24652900)
    { .name = "Diyarb", .latitude = 3792, .longitude = 4024, .region = 1, .timezone = UTZ_MOSCOW },  // Diyarbakir, Turkey (Region: Asia, 2025 Population: 1128360)
    { .name = "Dubai", .latitude = 2507, .longitude = 5519, .region = 1, .timezone = UTZ_DUBAI },  // Dubai, United Arab Emirates (Region: Asia, 2025 Population: 3094640)
    { .name = "Fukuok", .latitude = 3363, .longitude = 13062, .region = 1, .timezone = UTZ_TOKYO },  // Fukuoka, Japan (Region: Asia, 2025 Population: 5465920)
    { .name = "Guangz", .latitude = 2313, .longitude = 11326, .region = 1, .timezone = UTZ_SINGAPORE },  // Guangzhou, China (Region: Asia, 2025 Population: 14878700)
    { .name = "Guwaha", .latitude = 2618, .longitude = 9175, .region = 1, .timezone = UTZ_KOLKATA },  // Guwahati, India (Region: Asia, 2025 Population: 1224170)
    { .name = "Haerbi", .latitude = 3611, .longitude = 12039, .region = 1, .timezone = UTZ_SINGAPORE },  // Haerbin, China (Region: Asia, 2025 Population: 7066860)
    { .name = "Hanoi", .latitude = 2103, .longitude = 10585, .region = 1, .timezone = UTZ_BANGKOK },  // Hanoi, Vietnam (Region: Asia, 2025 Population: 5602200)
    { .name = "Ho Chi", .latitude = 1082, .longitude = 10663, .region = 1, .timezone = UTZ_BANGKOK },  // Ho Chi Minh City, Vietnam (Region: Asia, 2025 Population: 9816320)
    { .name = "Indore", .latitude = 2272, .longitude = 7587, .region = 1, .timezone = UTZ_KOLKATA },  // Indore, India (Region: Asia, 2025 Population: 3482830)
    { .name = "Istanb", .latitude = 4101, .longitude = 2898, .region = 1, .timezone = UTZ_MOSCOW },  // Istanbul, Turkey (Region: Asia, 2025 Population: 16236700)
    { .name = "Jakart", .latitude = -618, .longitude = 10683, .region = 1, .timezone = UTZ_BANGKOK },  // Jakarta, Indonesia (Region: Asia, 2025 Population: 11634100)
    { .name = "Jiddah", .latitude = 2155, .longitude = 3917, .region = 1, .timezone = UTZ_MOSCOW },  // Jiddah, Saudi Arabia (Region: Asia, 2025 Population: 5021600)
    { .name = "Jixi H", .latitude = 4530, .longitude = 13098, .region = 1, .timezone = UTZ_SINGAPORE },  // Jixi Heilongjiang, China (Region: Asia, 2025 Population: 1024970)
    { .name = "Kabul", .latitude = 3453, .longitude = 6919, .region = 1, .timezone = UTZ_KATHMANDU  },  // Kabul, Afghanistan (Region: Asia, 2025 Population: 4877020)
    { .name = "Karach", .latitude = 2485, .longitude = 6702, .region = 1, .timezone = UTZ_KOLKATA  },  // Karachi, Pakistan (Region: Asia, 2025 Population: 18076800)
    { .name = "Kathma", .latitude = 2771, .longitude = 8532, .region = 1, .timezone = UTZ_KATHMANDU },  // Kathmandu, Nepal (Region: Asia, 2025 Population: 1672900)
    { .name = "Kolkat", .latitude = 2257, .longitude = 8836, .region = 1, .timezone = UTZ_KOLKATA },  // Kolkata, India (Region: Asia, 2025 Population: 15845200)
    { .name = "Kuala ", .latitude = 315, .longitude = 10170, .region = 1, .timezone = UTZ_SINGAPORE },  // Kuala Lumpur, Malaysia (Region: Asia, 2025 Population: 9000280)
    { .name = "Kunmin", .latitude = 2504, .longitude = 10271, .region = 1, .timezone = UTZ_SINGAPORE },  // Kunming, China (Region: Asia, 2025 Population: 4955680)
    { .name = "Kuwait", .latitude = 2938, .longitude = 4797, .region = 1, .timezone = UTZ_MOSCOW },  // Kuwait City, Kuwait (Region: Asia, 2025 Population: 3405000)
    { .name = "Lahore", .latitude = 3157, .longitude = 7431, .region = 1, .timezone = UTZ_KOLKATA  },  // Lahore, Pakistan (Region: Asia, 2025 Population: 14825800)
    { .name = "Lanzho", .latitude = 3647, .longitude = 10373, .region = 1, .timezone = UTZ_SINGAPORE },  // Lanzhou, China (Region: Asia, 2025 Population: 3430880)
    { .name = "Makass", .latitude = -513, .longitude = 11941, .region = 1, .timezone = UTZ_SINGAPORE },  // Makassar, Indonesia (Region: Asia, 2025 Population: 1737390)
    { .name = "Mandal", .latitude = 2196, .longitude = 9609, .region = 1, .timezone = UTZ_YANGON },  // Mandalay, Myanmar (Region: Asia, 2025 Population: 1594300)
    { .name = "Manila", .latitude = 1459, .longitude = 12098, .region = 1, .timezone = UTZ_SINGAPORE },  // Manila, Philippines (Region: Asia, 2025 Population: 15230600)
    { .name = "Mashha", .latitude = 3630, .longitude = 5961, .region = 1, .timezone = UTZ_TEHRAN },  // Mashhad, Iran (Region: Asia, 2025 Population: 3460660)
    { .name = "Medan", .latitude = 359, .longitude = 9867, .region = 1, .timezone = UTZ_BANGKOK },  // Medan, Indonesia (Region: Asia, 2025 Population: 2521590)
    { .name = "Mumbai", .latitude = 1905, .longitude = 7287, .region = 1, .timezone = UTZ_KOLKATA },  // Mumbai, India (Region: Asia, 2025 Population: 22089000)
    { .name = "Nannin", .latitude = 2282, .longitude = 10836, .region = 1, .timezone = UTZ_SINGAPORE },  // Nanning, China (Region: Asia, 2025 Population: 4383600)
    { .name = "New Ta", .latitude = 2501, .longitude = 12147, .region = 1, .timezone = UTZ_SINGAPORE },  // New Taipei, Taiwan (Region: Asia, 2025 Population: 4563850)
    { .name = "Quetta", .latitude = 3019, .longitude = 6700, .region = 1, .timezone = UTZ_KOLKATA  },  // Quetta, Pakistan (Region: Asia, 2025 Population: 1253110)
    { .name = "Riyadh", .latitude = 2333, .longitude = 4533, .region = 1, .timezone = UTZ_MOSCOW },  // Riyadh, Saudi Arabia (Region: Asia, 2025 Population: 7952860)
    { .name = "Samari", .latitude = -50, .longitude = 11714, .region = 1, .timezone = UTZ_SINGAPORE },  // Samarinda, Indonesia (Region: Asia, 2025 Population: 1154760)
    { .name = "Sanaa", .latitude = 1535, .longitude = 4420, .region = 1, .timezone = UTZ_MOSCOW },  // Sanaa, Yemen (Region: Asia, 2025 Population: 3527430)
    { .name = "Sappor", .latitude = 4306, .longitude = 14135, .region = 1, .timezone = UTZ_TOKYO },  // Sapporo, Japan (Region: Asia, 2025 Population: 2653580)
    { .name = "Seoul", .latitude = 3757, .longitude = 12698, .region = 1, .timezone = UTZ_TOKYO },  // Seoul, South Korea (Region: Asia, 2025 Population: 10025800)
    { .name = "Shangh", .latitude = 3123, .longitude = 12147, .region = 1, .timezone = UTZ_SINGAPORE },  // Shanghai, China (Region: Asia, 2025 Population: 30482100)
    { .name = "Shenya", .latitude = 4180, .longitude = 12343, .region = 1, .timezone = UTZ_SINGAPORE },  // Shenyang, China (Region: Asia, 2025 Population: 7974270)
    { .name = "Shiraz", .latitude = 2961, .longitude = 5254, .region = 1, .timezone = UTZ_TEHRAN },  // Shiraz, Iran (Region: Asia, 2025 Population: 1763900)
    { .name = "Srinag", .latitude = 3407, .longitude = 7482, .region = 1, .timezone = UTZ_KOLKATA },  // Srinagar, India (Region: Asia, 2025 Population: 1777610)
    { .name = "Suraba", .latitude = -725, .longitude = 11274, .region = 1, .timezone = UTZ_BANGKOK },  // Surabaya, Indonesia (Region: Asia, 2025 Population: 3137620)
    { .name = "Tabriz", .latitude = 3807, .longitude = 4630, .region = 1, .timezone = UTZ_TEHRAN },  // Tabriz, Iran (Region: Asia, 2025 Population: 1695670)
    { .name = "Tashke", .latitude = 4131, .longitude = 6928, .region = 1, .timezone = UTZ_KOLKATA  },  // Tashkent, Uzbekistan (Region: Asia, 2025 Population: 2665080)
    { .name = "Tehran", .latitude = 3569, .longitude = 5139, .region = 1, .timezone = UTZ_TEHRAN },  // Tehran, Iran (Region: Asia, 2025 Population: 9729740)
    { .name = "Tel Av", .latitude = 3209, .longitude = 3478, .region = 1, .timezone = UTZ_JERUSALEM },  // Tel Aviv, Israel (Region: Asia, 2025 Population: 4568530)
    { .name = "Thiruv", .latitude = 849, .longitude = 7695, .region = 1, .timezone = UTZ_KOLKATA },  // Thiruvananthapuram, India (Region: Asia, 2025 Population: 3072530)
    { .name = "Tokyo", .latitude = 3568, .longitude = 13976, .region = 1, .timezone = UTZ_TOKYO },  // Tokyo, Japan (Region: Asia, 2025 Population: 37036200)
    { .name = "Ulaanb", .latitude = 4792, .longitude = 10692, .region = 1, .timezone = UTZ_SINGAPORE },  // Ulaanbaatar, Mongolia (Region: Asia, 2025 Population: 1724890)
    { .name = "Urumqi", .latitude = 4382, .longitude = 8761, .region = 1, .timezone = UTZ_KOLKATA  },  // Urumqi, China (Region: Asia, 2025 Population: 5132170)
    { .name = "Varana", .latitude = 2534, .longitude = 8301, .region = 1, .timezone = UTZ_KOLKATA },  // Varanasi, India (Region: Asia, 2025 Population: 1826010)
    { .name = "Visakh", .latitude = 1769, .longitude = 8329, .region = 1, .timezone = UTZ_KOLKATA },  // Visakhapatnam, India (Region: Asia, 2025 Population: 2440420)
    { .name = "Wuhan", .latitude = 3060, .longitude = 11430, .region = 1, .timezone = UTZ_SINGAPORE },  // Wuhan, China (Region: Asia, 2025 Population: 8986480)
    { .name = "Xinxia", .latitude = 3531, .longitude = 11405, .region = 1, .timezone = UTZ_SINGAPORE },  // Xinxiang, China (Region: Asia, 2025 Population: 1180750)
    { .name = "Yangon", .latitude = 1680, .longitude = 9616, .region = 1, .timezone = UTZ_YANGON },  // Yangon, Myanmar (Region: Asia, 2025 Population: 5813190)
    // Europe
    { .name = "Athens", .latitude = 3798, .longitude = 2373, .region = 2, .timezone = UTZ_HELSINKI  },  // Athens, Greece (Region: Europe, 2025 Population: 3155320)
    { .name = "Barcel", .latitude = 4138, .longitude = 218, .region = 2, .timezone = UTZ_BERLIN  },  // Barcelona, Spain (Region: Europe, 2025 Population: 5733250)
    { .name = "Berlin", .latitude = 5252, .longitude = 1340, .region = 2, .timezone = UTZ_BERLIN  },  // Berlin, Germany (Region: Europe, 2025 Population: 3580190)
    { .name = "Buchar", .latitude = 4444, .longitude = 2610, .region = 2, .timezone = UTZ_HELSINKI  },  // Bucharest, Romania (Region: Europe, 2025 Population: 1758700)
    { .name = "Glasgo", .latitude = 5586, .longitude = -425, .region = 2, .timezone = UTZ_LONDON },  // Glasgow, United Kingdom (Region: Europe, 2025 Population: 1718940)
    { .name = "Helsin", .latitude = 6017, .longitude = 2494, .region = 2, .timezone = UTZ_HELSINKI  },  // Helsinki, Finland (Region: Europe, 2025 Population: 1354890)
    { .name = "Kiev", .latitude = 5045, .longitude = 3052, .region = 2, .timezone = UTZ_HELSINKI  },  // Kiev, Ukraine (Region: Europe, 2025 Population: 3018160)
    { .name = "Lisbon", .latitude = 3871, .longitude = -914, .region = 2, .timezone = UTZ_LONDON },  // Lisbon, Portugal (Region: Europe, 2025 Population: 3028270)
    { .name = "London", .latitude = 5151, .longitude = -13, .region = 2, .timezone = UTZ_LONDON },  // London, United Kingdom (Region: Europe, 2025 Population: 9840740)
    { .name = "Madrid", .latitude = 4042, .longitude = -370, .region = 2, .timezone = UTZ_BERLIN  },  // Madrid, Spain (Region: Europe, 2025 Population: 6810530)
    { .name = "Minsk", .latitude = 5390, .longitude = 2756, .region = 2, .timezone = UTZ_MOSCOW },  // Minsk, Belarus (Region: Europe, 2025 Population: 2070930)
    { .name = "Paris", .latitude = 4885, .longitude = 235, .region = 2, .timezone = UTZ_BERLIN  },  // Paris, France (Region: Europe, 2025 Population: 11346800)
    { .name = "Rome", .latitude = 4189, .longitude = 1248, .region = 2, .timezone = UTZ_BERLIN  },  // Rome, Italy (Region: Europe, 2025 Population: 4347100)
    { .name = "Stockh", .latitude = 5933, .longitude = 1807, .region = 2, .timezone = UTZ_BERLIN  },  // Stockholm, Sweden (Region: Europe, 2025 Population: 1737760)
    { .name = "Turin", .latitude = 4507, .longitude = 768, .region = 2, .timezone = UTZ_BERLIN  },  // Turin, Italy (Region: Europe, 2025 Population: 1809850)
    { .name = "Vienna", .latitude = 4821, .longitude = 1637, .region = 2, .timezone = UTZ_BERLIN  },  // Vienna, Austria (Region: Europe, 2025 Population: 2005500)
    { .name = "Warsaw", .latitude = 5223, .longitude = 2107, .region = 2, .timezone = UTZ_BERLIN  },  // Warsaw, Poland (Region: Europe, 2025 Population: 1800230)
    // Africa
    { .name = "Abidja", .latitude = 532, .longitude = -402, .region = 3, .timezone = UTZ_UTC  },  // Abidjan, Ivory Coast (Region: Africa, 2025 Population: 6056880)
    { .name = "Addis ", .latitude = 904, .longitude = 3875, .region = 3, .timezone = UTZ_MOSCOW },  // Addis Ababa, Ethiopia (Region: Africa, 2025 Population: 5956680)
    { .name = "Algier", .latitude = 3677, .longitude = 306, .region = 3, .timezone = UTZ_LAGOS },  // Algiers, Algeria (Region: Africa, 2025 Population: 3004130)
    { .name = "Antana", .latitude = -1891, .longitude = 4753, .region = 3, .timezone = UTZ_MOSCOW },  // Antananarivo, Madagascar (Region: Africa, 2025 Population: 4228980)
    { .name = "Asmara", .latitude = 1534, .longitude = 3893, .region = 3, .timezone = UTZ_MOSCOW },  // Asmara, Eritrea (Region: Africa, 2025 Population: 1152180)
    { .name = "Bamako", .latitude = 1265, .longitude = -800, .region = 3, .timezone = UTZ_UTC  },  // Bamako, Mali (Region: Africa, 2025 Population: 3180340)
    { .name = "Bangui", .latitude = 436, .longitude = 1858, .region = 3, .timezone = UTZ_LAGOS },  // Bangui, Central African Republic (Region: Africa, 2025 Population: 1016150)
    { .name = "Bukavu", .latitude = -251, .longitude = 2886, .region = 3, .timezone = UTZ_MAPUTO  },  // Bukavu, DR Congo (Region: Africa, 2025 Population: 1369430)
    { .name = "Cairo", .latitude = 3004, .longitude = 3124, .region = 3, .timezone = UTZ_CAIRO },  // Cairo, Egypt (Region: Africa, 2025 Population: 23074200)
    { .name = "Cape T", .latitude = -3393, .longitude = 1842, .region = 3, .timezone = UTZ_MAPUTO  },  // Cape Town, South Africa (Region: Africa, 2025 Population: 5063580)
    { .name = "Casabl", .latitude = 3359, .longitude = -762, .region = 3, .timezone = UTZ_LONDON  },  // Casablanca, Morocco (Region: Africa, 2025 Population: 4012310)
    { .name = "Conakr", .latitude = 952, .longitude = -1370, .region = 3, .timezone = UTZ_UTC  },  // Conakry, Guinea (Region: Africa, 2025 Population: 2251590)
    { .name = "Dakar", .latitude = 1469, .longitude = -1745, .region = 3, .timezone = UTZ_UTC  },  // Dakar, Senegal (Region: Africa, 2025 Population: 3658640)
    { .name = "Dar es", .latitude = -682, .longitude = 3928, .region = 3, .timezone = UTZ_MOSCOW },  // Dar es Salaam, Tanzania (Region: Africa, 2025 Population: 8561520)
    { .name = "Hargey", .latitude = 956, .longitude = 4406, .region = 3, .timezone = UTZ_MOSCOW },  // Hargeysa, Somalia (Region: Africa, 2025 Population: 1227620)
    { .name = "Johann", .latitude = -2620, .longitude = 2805, .region = 3, .timezone = UTZ_MAPUTO  },  // Johannesburg, South Africa (Region: Africa, 2025 Population: 6444580)
    { .name = "Kampal", .latitude = 32, .longitude = 3258, .region = 3, .timezone = UTZ_MOSCOW },  // Kampala, Uganda (Region: Africa, 2025 Population: 4265160)
    { .name = "Kano", .latitude = 1199, .longitude = 852, .region = 3, .timezone = UTZ_LAGOS },  // Kano, Nigeria (Region: Africa, 2025 Population: 4645320)
    { .name = "Kharto", .latitude = 1550, .longitude = 3257, .region = 3, .timezone = UTZ_MAPUTO  },  // Khartoum, Sudan (Region: Africa, 2025 Population: 6754180)
    { .name = "Kinsha", .latitude = -430, .longitude = 1531, .region = 3, .timezone = UTZ_LAGOS },  // Kinshasa, DR Congo (Region: Africa, 2025 Population: 17778500)
    { .name = "Kisang", .latitude = 52, .longitude = 2521, .region = 3, .timezone = UTZ_MAPUTO  },  // Kisangani, DR Congo (Region: Africa, 2025 Population: 1546690)
    { .name = "Lagos", .latitude = 646, .longitude = 339, .region = 3, .timezone = UTZ_LAGOS },  // Lagos, Nigeria (Region: Africa, 2025 Population: 17156400)
    { .name = "Lilong", .latitude = -1399, .longitude = 3377, .region = 3, .timezone = UTZ_MAPUTO  },  // Lilongwe, Malawi (Region: Africa, 2025 Population: 1393010)
    { .name = "Lome", .latitude = 613, .longitude = 122, .region = 3, .timezone = UTZ_UTC  },  // Lome, Togo (Region: Africa, 2025 Population: 2108740)
    { .name = "Luanda", .latitude = -883, .longitude = 1324, .region = 3, .timezone = UTZ_LAGOS },  // Luanda, Angola (Region: Africa, 2025 Population: 10027900)
    { .name = "Lubang", .latitude = -1492, .longitude = 1349, .region = 3, .timezone = UTZ_LAGOS },  // Lubango, Angola (Region: Africa, 2025 Population: 1047810)
    { .name = "Lusaka", .latitude = -1542, .longitude = 2828, .region = 3, .timezone = UTZ_MAPUTO  },  // Lusaka, Zambia (Region: Africa, 2025 Population: 3470870)
    { .name = "Mbuji-", .latitude = -613, .longitude = 2360, .region = 3, .timezone = UTZ_MAPUTO  },  // Mbuji-Mayi, DR Congo (Region: Africa, 2025 Population: 3158340)
    { .name = "Mogadi", .latitude = 203, .longitude = 4534, .region = 3, .timezone = UTZ_MOSCOW },  // Mogadishu, Somalia (Region: Africa, 2025 Population: 2846420)
    { .name = "N-Djam", .latitude = 1212, .longitude = 1505, .region = 3, .timezone = UTZ_LAGOS },  // N-Djamena, Chad (Region: Africa, 2025 Population: 1722780)
    { .name = "Nairob", .latitude = -129, .longitude = 3682, .region = 3, .timezone = UTZ_MOSCOW },  // Nairobi, Kenya (Region: Africa, 2025 Population: 5766990)
    { .name = "Nampul", .latitude = -1497, .longitude = 3927, .region = 3, .timezone = UTZ_MAPUTO  },  // Nampula, Mozambique (Region: Africa, 2025 Population: 1057290)
    { .name = "Niamey", .latitude = 1352, .longitude = 211, .region = 3, .timezone = UTZ_LAGOS },  // Niamey, Niger (Region: Africa, 2025 Population: 1561490)
    { .name = "Nyala", .latitude = 1228, .longitude = 2477, .region = 3, .timezone = UTZ_MAPUTO  },  // Nyala, Sudan (Region: Africa, 2025 Population: 1145590)
    { .name = "Ouagad", .latitude = 1237, .longitude = -153, .region = 3, .timezone = UTZ_UTC  },  // Ouagadougou, Burkina Faso (Region: Africa, 2025 Population: 3520820)
    { .name = "Port E", .latitude = -3396, .longitude = 2562, .region = 3, .timezone = UTZ_MAPUTO  },  // Port Elizabeth, South Africa (Region: Africa, 2025 Population: 1330500)
    { .name = "Tripol", .latitude = 3290, .longitude = 1318, .region = 3, .timezone = UTZ_MAPUTO  },  // Tripoli, Libya (Region: Africa, 2025 Population: 1203390)
    { .name = "Tunis", .latitude = 3384, .longitude = 940, .region = 3, .timezone = UTZ_LAGOS },  // Tunis, Tunisia (Region: Africa, 2025 Population: 2545030)
    { .name = "Yaound", .latitude = 387, .longitude = 1152, .region = 3, .timezone = UTZ_LAGOS },  // Yaounde, Cameroon (Region: Africa, 2025 Population: 4854260)
    // South America
    { .name = "Asunci", .latitude = -2528, .longitude = -5763, .region = 4, .timezone = UTZ_SAO_PAULO },  // Asuncion, Paraguay (Region: South America, 2025 Population: 3627220)
    { .name = "Belem", .latitude = -145, .longitude = -4847, .region = 4, .timezone = UTZ_SAO_PAULO },  // Belem, Brazil (Region: South America, 2025 Population: 2453800)
    { .name = "Bogota", .latitude = 465, .longitude = -7408, .region = 4, .timezone = UTZ_CHICAGO  },  // Bogota, Colombia (Region: South America, 2025 Population: 11795800)
    { .name = "Brasil", .latitude = -1033, .longitude = -5320, .region = 4, .timezone = UTZ_CHICAGO  },  // Brasilia, Brazil (Region: South America, 2025 Population: 4990930)
    { .name = "Buenos", .latitude = -3461, .longitude = -5839, .region = 4, .timezone = UTZ_SAO_PAULO },  // Buenos Aires, Argentina (Region: South America, 2025 Population: 15752300)
    { .name = "Caraca", .latitude = 1051, .longitude = -6691, .region = 4, .timezone = UTZ_CHICAGO  },  // Caracas, Venezuela (Region: South America, 2025 Population: 3015110)
    { .name = "Cordob", .latitude = -3142, .longitude = -6418, .region = 4, .timezone = UTZ_SAO_PAULO },  // Cordoba, Argentina (Region: South America, 2025 Population: 1640600)
    { .name = "Fortal", .latitude = -373, .longitude = -3852, .region = 4, .timezone = UTZ_SAO_PAULO },  // Fortaleza, Brazil (Region: South America, 2025 Population: 4284450)
    { .name = "Goiani", .latitude = -1668, .longitude = -4925, .region = 4, .timezone = UTZ_SAO_PAULO },  // Goiania, Brazil (Region: South America, 2025 Population: 2927080)
    { .name = "Grande", .latitude = -302, .longitude = -4400, .region = 4, .timezone = UTZ_SAO_PAULO },  // Grande Sao Luis, Brazil (Region: South America, 2025 Population: 1548210)
    { .name = "Guayaq", .latitude = -229, .longitude = -8010, .region = 4, .timezone = UTZ_CHICAGO  },  // Guayaquil, Ecuador (Region: South America, 2025 Population: 3244750)
    { .name = "La Paz", .latitude = -1650, .longitude = -6813, .region = 4, .timezone = UTZ_CHICAGO  },  // La Paz, Bolivia (Region: South America, 2025 Population: 1997370)
    { .name = "Lima", .latitude = -1205, .longitude = -7703, .region = 4, .timezone = UTZ_CHICAGO  },  // Lima, Peru (Region: South America, 2025 Population: 11517300)
    { .name = "Manaus", .latitude = -313, .longitude = -5998, .region = 4, .timezone = UTZ_CHICAGO  },  // Manaus, Brazil (Region: South America, 2025 Population: 2434640)
    { .name = "Maraca", .latitude = 1065, .longitude = -7164, .region = 4, .timezone = UTZ_CHICAGO  },  // Maracaibo, Venezuela (Region: South America, 2025 Population: 2432440)
    { .name = "Mendoz", .latitude = -3460, .longitude = -6873, .region = 4, .timezone = UTZ_SAO_PAULO },  // Mendoza, Argentina (Region: South America, 2025 Population: 1257180)
    { .name = "Porto ", .latitude = -3003, .longitude = -5123, .region = 4, .timezone = UTZ_SAO_PAULO },  // Porto Alegre, Brazil (Region: South America, 2025 Population: 4268960)
    { .name = "Recife", .latitude = -806, .longitude = -3488, .region = 4, .timezone = UTZ_SAO_PAULO },  // Recife, Brazil (Region: South America, 2025 Population: 4344050)
    { .name = "Salvad", .latitude = -1298, .longitude = -3848, .region = 4, .timezone = UTZ_SAO_PAULO },  // Salvador, Brazil (Region: South America, 2025 Population: 4029910)
    { .name = "San Mi", .latitude = -2683, .longitude = -6520, .region = 4, .timezone = UTZ_SAO_PAULO },  // San Miguel de Tucuman, Argentina (Region: South America, 2025 Population: 1051040)
    { .name = "Santa ", .latitude = -1733, .longitude = -6150, .region = 4, .timezone = UTZ_CHICAGO  },  // Santa Cruz, Bolivia (Region: South America, 2025 Population: 1891230)
    { .name = "Santia", .latitude = -3344, .longitude = -7065, .region = 4, .timezone = UTZ_SANTIAGO },  // Santiago, Chile (Region: South America, 2025 Population: 6999460)
    { .name = "Sao Pa", .latitude = -2355, .longitude = -4663, .region = 4, .timezone = UTZ_SAO_PAULO },  // Sao Paulo, Brazil (Region: South America, 2025 Population: 22990000)
    // Oceania
    { .name = "Adelai", .latitude = -3493, .longitude = 13860, .region = 5, .timezone = UTZ_ADELAIDE },  // Adelaide, Australia (Region: Oceania, 2025 Population: 1392940)
    { .name = "Auckla", .latitude = -3685, .longitude = 17476, .region = 5, .timezone = UTZ_AUCKLAND },  // Auckland, New Zealand (Region: Oceania, 2025 Population: 1711130)
    { .name = "Brisba", .latitude = -2747, .longitude = 15302, .region = 5, .timezone = UTZ_BRISBANE },  // Brisbane, Australia (Region: Oceania, 2025 Population: 2568170)
    { .name = "Melbou", .latitude = -3781, .longitude = 14496, .region = 5, .timezone = UTZ_SYDNEY },  // Melbourne, Australia (Region: Oceania, 2025 Population: 5391890)
    { .name = "Perth", .latitude = -3196, .longitude = 11586, .region = 5, .timezone = UTZ_SINGAPORE },  // Perth, Australia (Region: Oceania, 2025 Population: 2169190)
    { .name = "Sydney", .latitude = -3387, .longitude = 15121, .region = 5, .timezone = UTZ_SYDNEY },  // Sydney, Australia (Region: Oceania, 2025 Population: 5248790)
};

#endif // SUNRISE_SUNSET_FACE_H_
