





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
} sunrise_sunset_long_lat_presets_t;

typedef struct {
    uint8_t timezone;
    const sunrise_sunset_long_lat_presets_t *cities;
    size_t count;
} sunrise_sunset_timezone_city_group_t;

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
    int8_t set_tz_idx;
    bool quick_ticks_running;
    const sunrise_sunset_timezone_city_group_t *cities_in_tz;
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

static const sunrise_sunset_alt_location_presets_t longLatPresets[] =
{
    { .name = "  "},  // Default, the long and lat get replaced by what's set in the watch
//    { .name = "Ny", .latitude = 4072, .longitude = -7401, .timezone = UTZ_NEW_YORK },  // New York City, NY
//    { .name = "LA", .latitude = 3405, .longitude = -11824, .timezone = UTZ_LOS_ANGELES },  // Los Angeles, CA
//    { .name = "dE", .latitude = 4221, .longitude = -8305, .timezone = UTZ_NEW_YORK },  // Detroit, MI
};

// Data came from here: https://worldpopulationreview.com/cities
// All cities have a 2025 population of at least 1000000 and removed all cities that are within 500km of a more populated city that shares a timezone. 190 cities total.
static const sunrise_sunset_long_lat_presets_t UTZ_LOS_ANGELES_CITIES[] = {
    { .name = "Los An", .latitude = 3405, .longitude = -11824, .region = 0 },  // Los Angeles, United States (Region: North America, 2025 Population: 3770958)
    { .name = "Vancou", .latitude = 4926, .longitude = -12311, .region = 0 },  // Vancouver, Canada (Region: North America, 2025 Population: 2707920)
};

static const sunrise_sunset_long_lat_presets_t UTZ_DENVER_CITIES[] = {
    { .name = "Calgar", .latitude = 5105, .longitude = -11406, .region = 0 },  // Calgary, Canada (Region: North America, 2025 Population: 1687900)
    { .name = "Ciudad", .latitude = 3174, .longitude = -10649, .region = 0 },  // Ciudad Juarez, Mexico (Region: North America, 2025 Population: 1625980)
};

static const sunrise_sunset_long_lat_presets_t UTZ_PHOENIX_CITIES[] = {
    { .name = "Phoeni", .latitude = 3345, .longitude = -11207, .region = 0 },  // Phoenix, United States (Region: North America, 2025 Population: 1675144)
};

static const sunrise_sunset_long_lat_presets_t UTZ_CHICAGO_CITIES[] = {
    { .name = "Chicag", .latitude = 4188, .longitude = -8762, .region = 0 },  // Chicago, United States (Region: North America, 2025 Population: 2611867)
    { .name = "Chihua", .latitude = 2850, .longitude = -10600, .region = 0 },  // Chihuahua, Mexico (Region: North America, 2025 Population: 1153540)
    { .name = "Guatem", .latitude = 1464, .longitude = -9051, .region = 0 },  // Guatemala City, Guatemala (Region: North America, 2025 Population: 3229740)
    { .name = "Housto", .latitude = 2976, .longitude = -9537, .region = 0 },  // Houston, United States (Region: North America, 2025 Population: 2324082)
    { .name = "Managu", .latitude = 1216, .longitude = -8627, .region = 0 },  // Managua, Nicaragua (Region: North America, 2025 Population: 1120900)
    { .name = "Merida", .latitude = 2097, .longitude = -8962, .region = 0 },  // Merida, Mexico (Region: North America, 2025 Population: 1258230)
    { .name = "Mexico", .latitude = 1932, .longitude = -9915, .region = 0 },  // Mexico City, Mexico (Region: North America, 2025 Population: 22752400)
    { .name = "Monter", .latitude = 2568, .longitude = -10032, .region = 0 },  // Monterrey, Mexico (Region: North America, 2025 Population: 5272360)
};

static const sunrise_sunset_long_lat_presets_t UTZ_NEW_YORK_CITIES[] = {
    { .name = "Detroi", .latitude = 4233, .longitude = -8304, .region = 0 },  // Detroit, United States (Region: North America, 2025 Population: 645705)
    { .name = "Jackso", .latitude = 3033, .longitude = -8166, .region = 0 },  // Jacksonville, United States (Region: North America, 2025 Population: 1008485)
    { .name = "Montre", .latitude = 4550, .longitude = -7357, .region = 0 },  // Montreal, Canada (Region: North America, 2025 Population: 4377310)
    { .name = "New Yo", .latitude = 4071, .longitude = -7401, .region = 0 },  // New York City, United States (Region: North America, 2025 Population: 7936530)
    { .name = "Port-a", .latitude = 1855, .longitude = -7234, .region = 0 },  // Port-au-Prince, Haiti (Region: North America, 2025 Population: 3133080)
    { .name = "Raleig", .latitude = 3578, .longitude = -7864, .region = 0 },  // Raleigh, United States (Region: North America, 2025 Population: 493589)
    { .name = "Toront", .latitude = 4365, .longitude = -7938, .region = 0 },  // Toronto, Canada (Region: North America, 2025 Population: 6491290)
};

static const sunrise_sunset_long_lat_presets_t UTZ_SANTIAGO_CITIES[] = {
    { .name = "Santia", .latitude = -3344, .longitude = -7065, .region = 4 },  // Santiago, Chile (Region: South America, 2025 Population: 6999460)
};

static const sunrise_sunset_long_lat_presets_t UTZ_SAO_PAULO_CITIES[] = {
    { .name = "Asunci", .latitude = -2528, .longitude = -5763, .region = 4 },  // Asuncion, Paraguay (Region: South America, 2025 Population: 3627220)
    { .name = "Belem", .latitude = -145, .longitude = -4847, .region = 4 },  // Belem, Brazil (Region: South America, 2025 Population: 2453800)
    { .name = "Buenos", .latitude = -3461, .longitude = -5839, .region = 4 },  // Buenos Aires, Argentina (Region: South America, 2025 Population: 15752300)
    { .name = "Cordob", .latitude = -3142, .longitude = -6418, .region = 4 },  // Cordoba, Argentina (Region: South America, 2025 Population: 1640600)
    { .name = "Fortal", .latitude = -373, .longitude = -3852, .region = 4 },  // Fortaleza, Brazil (Region: South America, 2025 Population: 4284450)
    { .name = "Goiani", .latitude = -1668, .longitude = -4925, .region = 4 },  // Goiania, Brazil (Region: South America, 2025 Population: 2927080)
    { .name = "Grande", .latitude = -302, .longitude = -4400, .region = 4 },  // Grande Sao Luis, Brazil (Region: South America, 2025 Population: 1548210)
    { .name = "Mendoz", .latitude = -3460, .longitude = -6873, .region = 4 },  // Mendoza, Argentina (Region: South America, 2025 Population: 1257180)
    { .name = "Porto ", .latitude = -3003, .longitude = -5123, .region = 4 },  // Porto Alegre, Brazil (Region: South America, 2025 Population: 4268960)
    { .name = "Recife", .latitude = -806, .longitude = -3488, .region = 4 },  // Recife, Brazil (Region: South America, 2025 Population: 4344050)
    { .name = "Salvad", .latitude = -1298, .longitude = -3848, .region = 4 },  // Salvador, Brazil (Region: South America, 2025 Population: 4029910)
    { .name = "San Mi", .latitude = -2683, .longitude = -6520, .region = 4 },  // San Miguel de Tucuman, Argentina (Region: South America, 2025 Population: 1051040)
    { .name = "Sao Pa", .latitude = -2355, .longitude = -4663, .region = 4 },  // Sao Paulo, Brazil (Region: South America, 2025 Population: 22990000)
};

static const sunrise_sunset_long_lat_presets_t UTZ_LONDON_CITIES[] = {
    { .name = "Glasgo", .latitude = 5586, .longitude = -425, .region = 2 },  // Glasgow, United Kingdom (Region: Europe, 2025 Population: 1718940)
    { .name = "Lisbon", .latitude = 3871, .longitude = -914, .region = 2 },  // Lisbon, Portugal (Region: Europe, 2025 Population: 3028270)
    { .name = "London", .latitude = 5151, .longitude = -13, .region = 2 },  // London, United Kingdom (Region: Europe, 2025 Population: 9840740)
};

static const sunrise_sunset_long_lat_presets_t UTZ_LAGOS_CITIES[] = {
    { .name = "Algier", .latitude = 3677, .longitude = 306, .region = 3 },  // Algiers, Algeria (Region: Africa, 2025 Population: 3004130)
    { .name = "Bangui", .latitude = 436, .longitude = 1858, .region = 3 },  // Bangui, Central African Republic (Region: Africa, 2025 Population: 1016150)
    { .name = "Kano", .latitude = 1199, .longitude = 852, .region = 3 },  // Kano, Nigeria (Region: Africa, 2025 Population: 4645320)
    { .name = "Kinsha", .latitude = -430, .longitude = 1531, .region = 3 },  // Kinshasa, DR Congo (Region: Africa, 2025 Population: 17778500)
    { .name = "Lagos", .latitude = 646, .longitude = 339, .region = 3 },  // Lagos, Nigeria (Region: Africa, 2025 Population: 17156400)
    { .name = "Luanda", .latitude = -883, .longitude = 1324, .region = 3 },  // Luanda, Angola (Region: Africa, 2025 Population: 10027900)
    { .name = "Lubang", .latitude = -1492, .longitude = 1349, .region = 3 },  // Lubango, Angola (Region: Africa, 2025 Population: 1047810)
    { .name = "N-Djam", .latitude = 1212, .longitude = 1505, .region = 3 },  // N-Djamena, Chad (Region: Africa, 2025 Population: 1722780)
    { .name = "Niamey", .latitude = 1352, .longitude = 211, .region = 3 },  // Niamey, Niger (Region: Africa, 2025 Population: 1561490)
    { .name = "Tunis", .latitude = 3384, .longitude = 940, .region = 3 },  // Tunis, Tunisia (Region: Africa, 2025 Population: 2545030)
    { .name = "Yaound", .latitude = 387, .longitude = 1152, .region = 3 },  // Yaounde, Cameroon (Region: Africa, 2025 Population: 4854260)
};

static const sunrise_sunset_long_lat_presets_t UTZ_CAIRO_CITIES[] = {
    { .name = "Cairo", .latitude = 3004, .longitude = 3124, .region = 3 },  // Cairo, Egypt (Region: Africa, 2025 Population: 23074200)
};

static const sunrise_sunset_long_lat_presets_t UTZ_JERUSALEM_CITIES[] = {
    { .name = "Tel Av", .latitude = 3209, .longitude = 3478, .region = 1 },  // Tel Aviv, Israel (Region: Asia, 2025 Population: 4568530)
};

static const sunrise_sunset_long_lat_presets_t UTZ_MOSCOW_CITIES[] = {
    { .name = "Baghda", .latitude = 3331, .longitude = 4439, .region = 1 },  // Baghdad, Iraq (Region: Asia, 2025 Population: 8141120)
    { .name = "Damasc", .latitude = 3351, .longitude = 3631, .region = 1 },  // Damascus, Syria (Region: Asia, 2025 Population: 2799960)
    { .name = "Diyarb", .latitude = 3792, .longitude = 4024, .region = 1 },  // Diyarbakir, Turkey (Region: Asia, 2025 Population: 1128360)
    { .name = "Istanb", .latitude = 4101, .longitude = 2898, .region = 1 },  // Istanbul, Turkey (Region: Asia, 2025 Population: 16236700)
    { .name = "Jiddah", .latitude = 2155, .longitude = 3917, .region = 1 },  // Jiddah, Saudi Arabia (Region: Asia, 2025 Population: 5021600)
    { .name = "Kuwait", .latitude = 2938, .longitude = 4797, .region = 1 },  // Kuwait City, Kuwait (Region: Asia, 2025 Population: 3405000)
    { .name = "Riyadh", .latitude = 2333, .longitude = 4533, .region = 1 },  // Riyadh, Saudi Arabia (Region: Asia, 2025 Population: 7952860)
    { .name = "Sanaa", .latitude = 1535, .longitude = 4420, .region = 1 },  // Sanaa, Yemen (Region: Asia, 2025 Population: 3527430)
    { .name = "Minsk", .latitude = 5390, .longitude = 2756, .region = 2 },  // Minsk, Belarus (Region: Europe, 2025 Population: 2070930)
    { .name = "Addis ", .latitude = 904, .longitude = 3875, .region = 3 },  // Addis Ababa, Ethiopia (Region: Africa, 2025 Population: 5956680)
    { .name = "Antana", .latitude = -1891, .longitude = 4753, .region = 3 },  // Antananarivo, Madagascar (Region: Africa, 2025 Population: 4228980)
    { .name = "Asmara", .latitude = 1534, .longitude = 3893, .region = 3 },  // Asmara, Eritrea (Region: Africa, 2025 Population: 1152180)
    { .name = "Dar es", .latitude = -682, .longitude = 3928, .region = 3 },  // Dar es Salaam, Tanzania (Region: Africa, 2025 Population: 8561520)
    { .name = "Hargey", .latitude = 956, .longitude = 4406, .region = 3 },  // Hargeysa, Somalia (Region: Africa, 2025 Population: 1227620)
    { .name = "Kampal", .latitude = 32, .longitude = 3258, .region = 3 },  // Kampala, Uganda (Region: Africa, 2025 Population: 4265160)
    { .name = "Mogadi", .latitude = 203, .longitude = 4534, .region = 3 },  // Mogadishu, Somalia (Region: Africa, 2025 Population: 2846420)
    { .name = "Nairob", .latitude = -129, .longitude = 3682, .region = 3 },  // Nairobi, Kenya (Region: Africa, 2025 Population: 5766990)
};

static const sunrise_sunset_long_lat_presets_t UTZ_TEHRAN_CITIES[] = {
    { .name = "Mashha", .latitude = 3630, .longitude = 5961, .region = 1 },  // Mashhad, Iran (Region: Asia, 2025 Population: 3460660)
    { .name = "Shiraz", .latitude = 2961, .longitude = 5254, .region = 1 },  // Shiraz, Iran (Region: Asia, 2025 Population: 1763900)
    { .name = "Tabriz", .latitude = 3807, .longitude = 4630, .region = 1 },  // Tabriz, Iran (Region: Asia, 2025 Population: 1695670)
    { .name = "Tehran", .latitude = 3569, .longitude = 5139, .region = 1 },  // Tehran, Iran (Region: Asia, 2025 Population: 9729740)
};

static const sunrise_sunset_long_lat_presets_t UTZ_DUBAI_CITIES[] = {
    { .name = "Baku", .latitude = 4038, .longitude = 4983, .region = 1 },  // Baku, Azerbaijan (Region: Asia, 2025 Population: 2496500)
    { .name = "Dubai", .latitude = 2507, .longitude = 5519, .region = 1 },  // Dubai, United Arab Emirates (Region: Asia, 2025 Population: 3094640)
};

static const sunrise_sunset_long_lat_presets_t UTZ_KOLKATA_CITIES[] = {
    { .name = "Bangal", .latitude = 1298, .longitude = 7759, .region = 1 },  // Bangalore, India (Region: Asia, 2025 Population: 14395400)
    { .name = "Delhi", .latitude = 2863, .longitude = 7722, .region = 1 },  // Delhi, India (Region: Asia, 2025 Population: 34665600)
    { .name = "Guwaha", .latitude = 2618, .longitude = 9175, .region = 1 },  // Guwahati, India (Region: Asia, 2025 Population: 1224170)
    { .name = "Indore", .latitude = 2272, .longitude = 7587, .region = 1 },  // Indore, India (Region: Asia, 2025 Population: 3482830)
    { .name = "Kolkat", .latitude = 2257, .longitude = 8836, .region = 1 },  // Kolkata, India (Region: Asia, 2025 Population: 15845200)
    { .name = "Mumbai", .latitude = 1905, .longitude = 7287, .region = 1 },  // Mumbai, India (Region: Asia, 2025 Population: 22089000)
    { .name = "Srinag", .latitude = 3407, .longitude = 7482, .region = 1 },  // Srinagar, India (Region: Asia, 2025 Population: 1777610)
    { .name = "Thiruv", .latitude = 849, .longitude = 7695, .region = 1 },  // Thiruvananthapuram, India (Region: Asia, 2025 Population: 3072530)
    { .name = "Varana", .latitude = 2534, .longitude = 8301, .region = 1 },  // Varanasi, India (Region: Asia, 2025 Population: 1826010)
    { .name = "Visakh", .latitude = 1769, .longitude = 8329, .region = 1 },  // Visakhapatnam, India (Region: Asia, 2025 Population: 2440420)
};

static const sunrise_sunset_long_lat_presets_t UTZ_KATHMANDU_CITIES[] = {
    { .name = "Kathma", .latitude = 2771, .longitude = 8532, .region = 1 },  // Kathmandu, Nepal (Region: Asia, 2025 Population: 1672900)
};

static const sunrise_sunset_long_lat_presets_t UTZ_YANGON_CITIES[] = {
    { .name = "Mandal", .latitude = 2196, .longitude = 9609, .region = 1 },  // Mandalay, Myanmar (Region: Asia, 2025 Population: 1594300)
    { .name = "Yangon", .latitude = 1680, .longitude = 9616, .region = 1 },  // Yangon, Myanmar (Region: Asia, 2025 Population: 5813190)
};

static const sunrise_sunset_long_lat_presets_t UTZ_BANGKOK_CITIES[] = {
    { .name = "Bangko", .latitude = 1375, .longitude = 10049, .region = 1 },  // Bangkok, Thailand (Region: Asia, 2025 Population: 11391700)
    { .name = "Batam", .latitude = 110, .longitude = 10404, .region = 1 },  // Batam, Indonesia (Region: Asia, 2025 Population: 1858910)
    { .name = "Chiang", .latitude = 1879, .longitude = 9899, .region = 1 },  // Chiang Mai, Thailand (Region: Asia, 2025 Population: 1244190)
    { .name = "Da Nan", .latitude = 1607, .longitude = 10821, .region = 1 },  // Da Nang, Vietnam (Region: Asia, 2025 Population: 1286000)
    { .name = "Hanoi", .latitude = 2103, .longitude = 10585, .region = 1 },  // Hanoi, Vietnam (Region: Asia, 2025 Population: 5602200)
    { .name = "Ho Chi", .latitude = 1082, .longitude = 10663, .region = 1 },  // Ho Chi Minh City, Vietnam (Region: Asia, 2025 Population: 9816320)
    { .name = "Jakart", .latitude = -618, .longitude = 10683, .region = 1 },  // Jakarta, Indonesia (Region: Asia, 2025 Population: 11634100)
    { .name = "Medan", .latitude = 359, .longitude = 9867, .region = 1 },  // Medan, Indonesia (Region: Asia, 2025 Population: 2521590)
    { .name = "Suraba", .latitude = -725, .longitude = 11274, .region = 1 },  // Surabaya, Indonesia (Region: Asia, 2025 Population: 3137620)
};

static const sunrise_sunset_long_lat_presets_t UTZ_SINGAPORE_CITIES[] = {
    { .name = "Baotou", .latitude = 4062, .longitude = 10994, .region = 1 },  // Baotou, China (Region: Asia, 2025 Population: 2425700)
    { .name = "Beijin", .latitude = 4019, .longitude = 11641, .region = 1 },  // Beijing, China (Region: Asia, 2025 Population: 22596500)
    { .name = "Chongq", .latitude = 3006, .longitude = 10787, .region = 1 },  // Chongqing, China (Region: Asia, 2025 Population: 18171200)
    { .name = "Daqing", .latitude = 4632, .longitude = 12456, .region = 1 },  // Daqing, China (Region: Asia, 2025 Population: 2085470)
    { .name = "Davao ", .latitude = 706, .longitude = 12561, .region = 1 },  // Davao City, Philippines (Region: Asia, 2025 Population: 2033990)
    { .name = "Denpas", .latitude = -865, .longitude = 11522, .region = 1 },  // Denpasar, Indonesia (Region: Asia, 2025 Population: 1097310)
    { .name = "Guangz", .latitude = 2313, .longitude = 11326, .region = 1 },  // Guangzhou, China (Region: Asia, 2025 Population: 14878700)
    { .name = "Haerbi", .latitude = 3611, .longitude = 12039, .region = 1 },  // Haerbin, China (Region: Asia, 2025 Population: 7066860)
    { .name = "Jixi H", .latitude = 4530, .longitude = 13098, .region = 1 },  // Jixi Heilongjiang, China (Region: Asia, 2025 Population: 1024970)
    { .name = "Kuala ", .latitude = 315, .longitude = 10170, .region = 1 },  // Kuala Lumpur, Malaysia (Region: Asia, 2025 Population: 9000280)
    { .name = "Kunmin", .latitude = 2504, .longitude = 10271, .region = 1 },  // Kunming, China (Region: Asia, 2025 Population: 4955680)
    { .name = "Lanzho", .latitude = 3647, .longitude = 10373, .region = 1 },  // Lanzhou, China (Region: Asia, 2025 Population: 3430880)
    { .name = "Makass", .latitude = -513, .longitude = 11941, .region = 1 },  // Makassar, Indonesia (Region: Asia, 2025 Population: 1737390)
    { .name = "Manila", .latitude = 1459, .longitude = 12098, .region = 1 },  // Manila, Philippines (Region: Asia, 2025 Population: 15230600)
    { .name = "Nannin", .latitude = 2282, .longitude = 10836, .region = 1 },  // Nanning, China (Region: Asia, 2025 Population: 4383600)
    { .name = "New Ta", .latitude = 2501, .longitude = 12147, .region = 1 },  // New Taipei, Taiwan (Region: Asia, 2025 Population: 4563850)
    { .name = "Samari", .latitude = -50, .longitude = 11714, .region = 1 },  // Samarinda, Indonesia (Region: Asia, 2025 Population: 1154760)
    { .name = "Shangh", .latitude = 3123, .longitude = 12147, .region = 1 },  // Shanghai, China (Region: Asia, 2025 Population: 30482100)
    { .name = "Shenya", .latitude = 4180, .longitude = 12343, .region = 1 },  // Shenyang, China (Region: Asia, 2025 Population: 7974270)
    { .name = "Ulaanb", .latitude = 4792, .longitude = 10692, .region = 1 },  // Ulaanbaatar, Mongolia (Region: Asia, 2025 Population: 1724890)
    { .name = "Wuhan", .latitude = 3060, .longitude = 11430, .region = 1 },  // Wuhan, China (Region: Asia, 2025 Population: 8986480)
    { .name = "Xinxia", .latitude = 3531, .longitude = 11405, .region = 1 },  // Xinxiang, China (Region: Asia, 2025 Population: 1180750)
    { .name = "Perth", .latitude = -3196, .longitude = 11586, .region = 5 },  // Perth, Australia (Region: Oceania, 2025 Population: 2169190)
};

static const sunrise_sunset_long_lat_presets_t UTZ_TOKYO_CITIES[] = {
    { .name = "Fukuok", .latitude = 3363, .longitude = 13062, .region = 1 },  // Fukuoka, Japan (Region: Asia, 2025 Population: 5465920)
    { .name = "Sappor", .latitude = 4306, .longitude = 14135, .region = 1 },  // Sapporo, Japan (Region: Asia, 2025 Population: 2653580)
    { .name = "Seoul", .latitude = 3757, .longitude = 12698, .region = 1 },  // Seoul, South Korea (Region: Asia, 2025 Population: 10025800)
    { .name = "Tokyo", .latitude = 3568, .longitude = 13976, .region = 1 },  // Tokyo, Japan (Region: Asia, 2025 Population: 37036200)
};

static const sunrise_sunset_long_lat_presets_t UTZ_ADELAIDE_CITIES[] = {
    { .name = "Adelai", .latitude = -3493, .longitude = 13860, .region = 5 },  // Adelaide, Australia (Region: Oceania, 2025 Population: 1392940)
};

static const sunrise_sunset_long_lat_presets_t UTZ_BRISBANE_CITIES[] = {
    { .name = "Brisba", .latitude = -2747, .longitude = 15302, .region = 5 },  // Brisbane, Australia (Region: Oceania, 2025 Population: 2568170)
};

static const sunrise_sunset_long_lat_presets_t UTZ_SYDNEY_CITIES[] = {
    { .name = "Melbou", .latitude = -3781, .longitude = 14496, .region = 5 },  // Melbourne, Australia (Region: Oceania, 2025 Population: 5391890)
    { .name = "Sydney", .latitude = -3387, .longitude = 15121, .region = 5 },  // Sydney, Australia (Region: Oceania, 2025 Population: 5248790)
};

static const sunrise_sunset_long_lat_presets_t UTZ_AUCKLAND_CITIES[] = {
    { .name = "Auckla", .latitude = -3685, .longitude = 17476, .region = 5 },  // Auckland, New Zealand (Region: Oceania, 2025 Population: 1711130)
};

static const sunrise_sunset_timezone_city_group_t sunriseSunsetLongLatPresets[] = {
    { NUM_ZONE_NAMES, NULL, 0 },
    { UTZ_LOS_ANGELES, UTZ_LOS_ANGELES_CITIES, sizeof(UTZ_LOS_ANGELES_CITIES)/sizeof(UTZ_LOS_ANGELES_CITIES[0]) },
    { UTZ_DENVER, UTZ_DENVER_CITIES, sizeof(UTZ_DENVER_CITIES)/sizeof(UTZ_DENVER_CITIES[0]) },
    { UTZ_PHOENIX, UTZ_PHOENIX_CITIES, sizeof(UTZ_PHOENIX_CITIES)/sizeof(UTZ_PHOENIX_CITIES[0]) },
    { UTZ_CHICAGO, UTZ_CHICAGO_CITIES, sizeof(UTZ_CHICAGO_CITIES)/sizeof(UTZ_CHICAGO_CITIES[0]) },
    { UTZ_NEW_YORK, UTZ_NEW_YORK_CITIES, sizeof(UTZ_NEW_YORK_CITIES)/sizeof(UTZ_NEW_YORK_CITIES[0]) },
    { UTZ_SANTIAGO, UTZ_SANTIAGO_CITIES, sizeof(UTZ_SANTIAGO_CITIES)/sizeof(UTZ_SANTIAGO_CITIES[0]) },
    { UTZ_SAO_PAULO, UTZ_SAO_PAULO_CITIES, sizeof(UTZ_SAO_PAULO_CITIES)/sizeof(UTZ_SAO_PAULO_CITIES[0]) },
    { UTZ_LONDON, UTZ_LONDON_CITIES, sizeof(UTZ_LONDON_CITIES)/sizeof(UTZ_LONDON_CITIES[0]) },
    { UTZ_LAGOS, UTZ_LAGOS_CITIES, sizeof(UTZ_LAGOS_CITIES)/sizeof(UTZ_LAGOS_CITIES[0]) },
    { UTZ_CAIRO, UTZ_CAIRO_CITIES, sizeof(UTZ_CAIRO_CITIES)/sizeof(UTZ_CAIRO_CITIES[0]) },
    { UTZ_JERUSALEM, UTZ_JERUSALEM_CITIES, sizeof(UTZ_JERUSALEM_CITIES)/sizeof(UTZ_JERUSALEM_CITIES[0]) },
    { UTZ_MOSCOW, UTZ_MOSCOW_CITIES, sizeof(UTZ_MOSCOW_CITIES)/sizeof(UTZ_MOSCOW_CITIES[0]) },
    { UTZ_TEHRAN, UTZ_TEHRAN_CITIES, sizeof(UTZ_TEHRAN_CITIES)/sizeof(UTZ_TEHRAN_CITIES[0]) },
    { UTZ_DUBAI, UTZ_DUBAI_CITIES, sizeof(UTZ_DUBAI_CITIES)/sizeof(UTZ_DUBAI_CITIES[0]) },
    { UTZ_KOLKATA, UTZ_KOLKATA_CITIES, sizeof(UTZ_KOLKATA_CITIES)/sizeof(UTZ_KOLKATA_CITIES[0]) },
    { UTZ_KATHMANDU, UTZ_KATHMANDU_CITIES, sizeof(UTZ_KATHMANDU_CITIES)/sizeof(UTZ_KATHMANDU_CITIES[0]) },
    { UTZ_YANGON, UTZ_YANGON_CITIES, sizeof(UTZ_YANGON_CITIES)/sizeof(UTZ_YANGON_CITIES[0]) },
    { UTZ_BANGKOK, UTZ_BANGKOK_CITIES, sizeof(UTZ_BANGKOK_CITIES)/sizeof(UTZ_BANGKOK_CITIES[0]) },
    { UTZ_SINGAPORE, UTZ_SINGAPORE_CITIES, sizeof(UTZ_SINGAPORE_CITIES)/sizeof(UTZ_SINGAPORE_CITIES[0]) },
    { UTZ_TOKYO, UTZ_TOKYO_CITIES, sizeof(UTZ_TOKYO_CITIES)/sizeof(UTZ_TOKYO_CITIES[0]) },
    { UTZ_ADELAIDE, UTZ_ADELAIDE_CITIES, sizeof(UTZ_ADELAIDE_CITIES)/sizeof(UTZ_ADELAIDE_CITIES[0]) },
    { UTZ_BRISBANE, UTZ_BRISBANE_CITIES, sizeof(UTZ_BRISBANE_CITIES)/sizeof(UTZ_BRISBANE_CITIES[0]) },
    { UTZ_SYDNEY, UTZ_SYDNEY_CITIES, sizeof(UTZ_SYDNEY_CITIES)/sizeof(UTZ_SYDNEY_CITIES[0]) },
    { UTZ_AUCKLAND, UTZ_AUCKLAND_CITIES, sizeof(UTZ_AUCKLAND_CITIES)/sizeof(UTZ_AUCKLAND_CITIES[0]) },
};

#endif // SUNRISE_SUNSET_FACE_H_
