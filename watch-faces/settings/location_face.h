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
    bool quick_ticks_running;
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
} location_long_lat_presets_t;

// Data came from here: https://worldpopulationreview.com/cities
// Removed all cities that are within 500km of a more populated city
static const location_long_lat_presets_t locationLongLatPresets[] =
{
    // North America
    { .name = "Mexico City", .latitude = 1932, .longitude = -9915, .region = 0 },  // Mexico City, Mexico (2025 Population: 22752400)
    { .name = "New York City", .latitude = 4071, .longitude = -7401, .region = 0 },  // New York City, United States (2025 Population: 7936530)
    { .name = "Toronto", .latitude = 4365, .longitude = -7938, .region = 0 },  // Toronto, Canada (2025 Population: 6491290)
    { .name = "Monterrey", .latitude = 2568, .longitude = -10032, .region = 0 },  // Monterrey, Mexico (2025 Population: 5272360)
    { .name = "Montreal", .latitude = 4550, .longitude = -7357, .region = 0 },  // Montreal, Canada (2025 Population: 4377310)
    { .name = "Los Angeles", .latitude = 3405, .longitude = -11824, .region = 0 },  // Los Angeles, United States (2025 Population: 3770958)
    { .name = "Santo Domingo", .latitude = 1847, .longitude = -6989, .region = 0 },  // Santo Domingo, Dominican Republic (2025 Population: 3648110)
    { .name = "Guatemala City", .latitude = 1464, .longitude = -9051, .region = 0 },  // Guatemala City, Guatemala (2025 Population: 3229740)
    { .name = "Vancouver", .latitude = 4926, .longitude = -12311, .region = 0 },  // Vancouver, Canada (2025 Population: 2707920)
    { .name = "Chicago", .latitude = 4188, .longitude = -8762, .region = 0 },  // Chicago, United States (2025 Population: 2611867)
    { .name = "Houston", .latitude = 2976, .longitude = -9537, .region = 0 },  // Houston, United States (2025 Population: 2324082)
    { .name = "Havana", .latitude = 2314, .longitude = -8236, .region = 0 },  // Havana, Cuba (2025 Population: 2156350)
    { .name = "Panama City", .latitude = 897, .longitude = -7953, .region = 0 },  // Panama City, Panama (2025 Population: 2054540)
    { .name = "Calgary", .latitude = 5105, .longitude = -11406, .region = 0 },  // Calgary, Canada (2025 Population: 1687900)
    { .name = "Phoenix", .latitude = 3345, .longitude = -11207, .region = 0 },  // Phoenix, United States (2025 Population: 1675144)
    { .name = "Ciudad Juarez", .latitude = 3174, .longitude = -10649, .region = 0 },  // Ciudad Juarez, Mexico (2025 Population: 1625980)
    { .name = "Merida", .latitude = 2097, .longitude = -8962, .region = 0 },  // Merida, Mexico (2025 Population: 1258230)
    { .name = "Managua", .latitude = 1216, .longitude = -8627, .region = 0 },  // Managua, Nicaragua (2025 Population: 1120900)
    { .name = "Jacksonville", .latitude = 3033, .longitude = -8166, .region = 0 },  // Jacksonville, United States (2025 Population: 1008485)
    { .name = "Charlotte", .latitude = 3523, .longitude = -8084, .region = 0 },  // Charlotte, United States (2025 Population: 935017)
    { .name = "Culiacan", .latitude = 2480, .longitude = -10739, .region = 0 },  // Culiacan, Mexico (2025 Population: 918494)
    { .name = "Winnipeg", .latitude = 4990, .longitude = -9714, .region = 0 },  // Winnipeg, Canada (2025 Population: 857367)
    { .name = "San Francisco", .latitude = 3778, .longitude = -12242, .region = 0 },  // San Francisco, United States (2025 Population: 767968)
    // Asia
    { .name = "Tokyo", .latitude = 3568, .longitude = 13976, .region = 1 },  // Tokyo, Japan (2025 Population: 37036200)
    { .name = "Delhi", .latitude = 2863, .longitude = 7722, .region = 1 },  // Delhi, India (2025 Population: 34665600)
    { .name = "Shanghai", .latitude = 3123, .longitude = 12147, .region = 1 },  // Shanghai, China (2025 Population: 30482100)
    { .name = "Dhaka", .latitude = 2376, .longitude = 9039, .region = 1 },  // Dhaka, Bangladesh (2025 Population: 24652900)
    { .name = "Beijing", .latitude = 4019, .longitude = 11641, .region = 1 },  // Beijing, China (2025 Population: 22596500)
    { .name = "Mumbai", .latitude = 1905, .longitude = 7287, .region = 1 },  // Mumbai, India (2025 Population: 22089000)
    { .name = "Chongqing", .latitude = 3006, .longitude = 10787, .region = 1 },  // Chongqing, China (2025 Population: 18171200)
    { .name = "Karachi", .latitude = 2485, .longitude = 6702, .region = 1 },  // Karachi, Pakistan (2025 Population: 18076800)
    { .name = "Istanbul", .latitude = 4101, .longitude = 2898, .region = 1 },  // Istanbul, Turkey (2025 Population: 16236700)
    { .name = "Manila", .latitude = 1459, .longitude = 12098, .region = 1 },  // Manila, Philippines (2025 Population: 15230600)
    { .name = "Guangzhou", .latitude = 2313, .longitude = 11326, .region = 1 },  // Guangzhou, China (2025 Population: 14878700)
    { .name = "Bangalore", .latitude = 1298, .longitude = 7759, .region = 1 },  // Bangalore, India (2025 Population: 14395400)
    { .name = "Jakarta", .latitude = -618, .longitude = 10683, .region = 1 },  // Jakarta, Indonesia (2025 Population: 11634100)
    { .name = "Bangkok", .latitude = 1375, .longitude = 10049, .region = 1 },  // Bangkok, Thailand (2025 Population: 11391700)
    { .name = "Seoul", .latitude = 3757, .longitude = 12698, .region = 1 },  // Seoul, South Korea (2025 Population: 10025800)
    { .name = "Ho Chi Minh City", .latitude = 1082, .longitude = 10663, .region = 1 },  // Ho Chi Minh City, Vietnam (2025 Population: 9816320)
    { .name = "Tehran", .latitude = 3569, .longitude = 5139, .region = 1 },  // Tehran, Iran (2025 Population: 9729740)
    { .name = "Kuala Lumpur", .latitude = 315, .longitude = 10170, .region = 1 },  // Kuala Lumpur, Malaysia (2025 Population: 9000280)
    { .name = "Wuhan", .latitude = 3060, .longitude = 11430, .region = 1 },  // Wuhan, China (2025 Population: 8986480)
    { .name = "Baghdad", .latitude = 3331, .longitude = 4439, .region = 1 },  // Baghdad, Iraq (2025 Population: 8141120)
    { .name = "Shenyang", .latitude = 4180, .longitude = 12343, .region = 1 },  // Shenyang, China (2025 Population: 7974270)
    { .name = "Riyadh", .latitude = 2333, .longitude = 4533, .region = 1 },  // Riyadh, Saudi Arabia (2025 Population: 7952860)
    { .name = "Haerbin", .latitude = 3611, .longitude = 12039, .region = 1 },  // Haerbin, China (2025 Population: 7066860)
    { .name = "Yangon", .latitude = 1680, .longitude = 9616, .region = 1 },  // Yangon, Myanmar (2025 Population: 5813190)
    { .name = "Hanoi", .latitude = 2103, .longitude = 10585, .region = 1 },  // Hanoi, Vietnam (2025 Population: 5602200)
    { .name = "Fukuoka", .latitude = 3363, .longitude = 13062, .region = 1 },  // Fukuoka, Japan (2025 Population: 5465920)
    { .name = "Urumqi", .latitude = 4382, .longitude = 8761, .region = 1 },  // Urumqi, China (2025 Population: 5132170)
    { .name = "Jiddah", .latitude = 2155, .longitude = 3917, .region = 1 },  // Jiddah, Saudi Arabia (2025 Population: 5021600)
    { .name = "Kunming", .latitude = 2504, .longitude = 10271, .region = 1 },  // Kunming, China (2025 Population: 4955680)
    { .name = "Kabul", .latitude = 3453, .longitude = 6919, .region = 1 },  // Kabul, Afghanistan (2025 Population: 4877020)
    { .name = "New Taipei", .latitude = 2501, .longitude = 12147, .region = 1 },  // New Taipei, Taiwan (2025 Population: 4563850)
    { .name = "Faisalabad", .latitude = 3142, .longitude = 7309, .region = 1 },  // Faisalabad, Pakistan (2025 Population: 3892830)
    { .name = "Sanaa", .latitude = 1535, .longitude = 4420, .region = 1 },  // Sanaa, Yemen (2025 Population: 3527430)
    { .name = "Indore", .latitude = 2272, .longitude = 7587, .region = 1 },  // Indore, India (2025 Population: 3482830)
    { .name = "Mashhad", .latitude = 3630, .longitude = 5961, .region = 1 },  // Mashhad, Iran (2025 Population: 3460660)
    { .name = "Lanzhou", .latitude = 3647, .longitude = 10373, .region = 1 },  // Lanzhou, China (2025 Population: 3430880)
    { .name = "Kuwait City", .latitude = 2938, .longitude = 4797, .region = 1 },  // Kuwait City, Kuwait (2025 Population: 3405000)
    { .name = "Surabaya", .latitude = -725, .longitude = 11274, .region = 1 },  // Surabaya, Indonesia (2025 Population: 3137620)
    { .name = "Dubai", .latitude = 2507, .longitude = 5519, .region = 1 },  // Dubai, United Arab Emirates (2025 Population: 3094640)
    { .name = "Thiruvananthapuram", .latitude = 849, .longitude = 7695, .region = 1 },  // Thiruvananthapuram, India (2025 Population: 3072530)
    { .name = "Damascus", .latitude = 3351, .longitude = 3631, .region = 1 },  // Damascus, Syria (2025 Population: 2799960)
    { .name = "Patna", .latitude = 2561, .longitude = 8512, .region = 1 },  // Patna, India (2025 Population: 2689540)
    { .name = "Tashkent", .latitude = 4131, .longitude = 6928, .region = 1 },  // Tashkent, Uzbekistan (2025 Population: 2665080)
    { .name = "Sapporo", .latitude = 4306, .longitude = 14135, .region = 1 },  // Sapporo, Japan (2025 Population: 2653580)
    { .name = "Baku", .latitude = 4038, .longitude = 4983, .region = 1 },  // Baku, Azerbaijan (2025 Population: 2496500)
    { .name = "Visakhapatnam", .latitude = 1769, .longitude = 8329, .region = 1 },  // Visakhapatnam, India (2025 Population: 2440420)
    { .name = "Baotou", .latitude = 4062, .longitude = 10994, .region = 1 },  // Baotou, China (2025 Population: 2425700)
    { .name = "Daqing", .latitude = 4632, .longitude = 12456, .region = 1 },  // Daqing, China (2025 Population: 2085470)
    { .name = "Almaty", .latitude = 4324, .longitude = 7695, .region = 1 },  // Almaty, Kazakhstan (2025 Population: 2042040)
    { .name = "Davao City", .latitude = 706, .longitude = 12561, .region = 1 },  // Davao City, Philippines (2025 Population: 2033990)
    { .name = "Makassar", .latitude = -513, .longitude = 11941, .region = 1 },  // Makassar, Indonesia (2025 Population: 1737390)
    { .name = "Ulaanbaatar", .latitude = 4792, .longitude = 10692, .region = 1 },  // Ulaanbaatar, Mongolia (2025 Population: 1724890)
    { .name = "Mandalay", .latitude = 2196, .longitude = 9609, .region = 1 },  // Mandalay, Myanmar (2025 Population: 1594300)
    { .name = "Astana", .latitude = 5113, .longitude = 7143, .region = 1 },  // Astana, Kazakhstan (2025 Population: 1352560)
    { .name = "Da Nang", .latitude = 1607, .longitude = 10821, .region = 1 },  // Da Nang, Vietnam (2025 Population: 1286000)
    { .name = "Quetta", .latitude = 3019, .longitude = 6700, .region = 1 },  // Quetta, Pakistan (2025 Population: 1253110)
    { .name = "Xinxiang", .latitude = 3531, .longitude = 11405, .region = 1 },  // Xinxiang, China (2025 Population: 1180750)
    { .name = "Samarinda", .latitude = -50, .longitude = 11714, .region = 1 },  // Samarinda, Indonesia (2025 Population: 1154760)
    { .name = "Diyarbakir", .latitude = 3792, .longitude = 4024, .region = 1 },  // Diyarbakir, Turkey (2025 Population: 1128360)
    { .name = "Jixi Heilongjiang", .latitude = 4530, .longitude = 13098, .region = 1 },  // Jixi Heilongjiang, China (2025 Population: 1024970)
    // Europe
    { .name = "Paris", .latitude = 4885, .longitude = 235, .region = 2 },  // Paris, France (2025 Population: 11346800)
    { .name = "Madrid", .latitude = 4042, .longitude = -370, .region = 2 },  // Madrid, Spain (2025 Population: 6810530)
    { .name = "Barcelona", .latitude = 4138, .longitude = 218, .region = 2 },  // Barcelona, Spain (2025 Population: 5733250)
    { .name = "Rome", .latitude = 4189, .longitude = 1248, .region = 2 },  // Rome, Italy (2025 Population: 4347100)
    { .name = "Berlin", .latitude = 5252, .longitude = 1340, .region = 2 },  // Berlin, Germany (2025 Population: 3580190)
    { .name = "Athens", .latitude = 3798, .longitude = 2373, .region = 2 },  // Athens, Greece (2025 Population: 3155320)
    { .name = "Lisbon", .latitude = 3871, .longitude = -914, .region = 2 },  // Lisbon, Portugal (2025 Population: 3028270)
    { .name = "Kiev", .latitude = 5045, .longitude = 3052, .region = 2 },  // Kiev, Ukraine (2025 Population: 3018160)
    { .name = "Manchester", .latitude = 5348, .longitude = -225, .region = 2 },  // Manchester, United Kingdom (2025 Population: 2832580)
    { .name = "Vienna", .latitude = 4821, .longitude = 1637, .region = 2 },  // Vienna, Austria (2025 Population: 2005500)
    { .name = "Turin", .latitude = 4507, .longitude = 768, .region = 2 },  // Turin, Italy (2025 Population: 1809850)
    { .name = "Warsaw", .latitude = 5223, .longitude = 2107, .region = 2 },  // Warsaw, Poland (2025 Population: 1800230)
    { .name = "Stockholm", .latitude = 5933, .longitude = 1807, .region = 2 },  // Stockholm, Sweden (2025 Population: 1737760)
    { .name = "Sofia", .latitude = 4270, .longitude = 2332, .region = 2 },  // Sofia, Bulgaria (2025 Population: 1286460)
    // Africa
    { .name = "Cairo", .latitude = 3004, .longitude = 3124, .region = 3 },  // Cairo, Egypt (2025 Population: 23074200)
    { .name = "Kinshasa", .latitude = -430, .longitude = 1531, .region = 3 },  // Kinshasa, DR Congo (2025 Population: 17778500)
    { .name = "Lagos", .latitude = 646, .longitude = 339, .region = 3 },  // Lagos, Nigeria (2025 Population: 17156400)
    { .name = "Luanda", .latitude = -883, .longitude = 1324, .region = 3 },  // Luanda, Angola (2025 Population: 10027900)
    { .name = "Dar es Salaam", .latitude = -682, .longitude = 3928, .region = 3 },  // Dar es Salaam, Tanzania (2025 Population: 8561520)
    { .name = "Khartoum", .latitude = 1550, .longitude = 3257, .region = 3 },  // Khartoum, Sudan (2025 Population: 6754180)
    { .name = "Johannesburg", .latitude = -2620, .longitude = 2805, .region = 3 },  // Johannesburg, South Africa (2025 Population: 6444580)
    { .name = "Abidjan", .latitude = 532, .longitude = -402, .region = 3 },  // Abidjan, Ivory Coast (2025 Population: 6056880)
    { .name = "Addis Ababa", .latitude = 904, .longitude = 3875, .region = 3 },  // Addis Ababa, Ethiopia (2025 Population: 5956680)
    { .name = "Nairobi", .latitude = -129, .longitude = 3682, .region = 3 },  // Nairobi, Kenya (2025 Population: 5766990)
    { .name = "Cape Town", .latitude = -3393, .longitude = 1842, .region = 3 },  // Cape Town, South Africa (2025 Population: 5063580)
    { .name = "Yaounde", .latitude = 387, .longitude = 1152, .region = 3 },  // Yaounde, Cameroon (2025 Population: 4854260)
    { .name = "Kano", .latitude = 1199, .longitude = 852, .region = 3 },  // Kano, Nigeria (2025 Population: 4645320)
    { .name = "Kampala", .latitude = 32, .longitude = 3258, .region = 3 },  // Kampala, Uganda (2025 Population: 4265160)
    { .name = "Antananarivo", .latitude = -1891, .longitude = 4753, .region = 3 },  // Antananarivo, Madagascar (2025 Population: 4228980)
    { .name = "Casablanca", .latitude = 3359, .longitude = -762, .region = 3 },  // Casablanca, Morocco (2025 Population: 4012310)
    { .name = "Dakar", .latitude = 1469, .longitude = -1745, .region = 3 },  // Dakar, Senegal (2025 Population: 3658640)
    { .name = "Ouagadougou", .latitude = 1237, .longitude = -153, .region = 3 },  // Ouagadougou, Burkina Faso (2025 Population: 3520820)
    { .name = "Lusaka", .latitude = -1542, .longitude = 2828, .region = 3 },  // Lusaka, Zambia (2025 Population: 3470870)
    { .name = "Bamako", .latitude = 1265, .longitude = -800, .region = 3 },  // Bamako, Mali (2025 Population: 3180340)
    { .name = "Mbuji-Mayi", .latitude = -613, .longitude = 2360, .region = 3 },  // Mbuji-Mayi, DR Congo (2025 Population: 3158340)
    { .name = "Algiers", .latitude = 3677, .longitude = 306, .region = 3 },  // Algiers, Algeria (2025 Population: 3004130)
    { .name = "Mogadishu", .latitude = 203, .longitude = 4534, .region = 3 },  // Mogadishu, Somalia (2025 Population: 2846420)
    { .name = "Tunis", .latitude = 3384, .longitude = 940, .region = 3 },  // Tunis, Tunisia (2025 Population: 2545030)
    { .name = "Conakry", .latitude = 952, .longitude = -1370, .region = 3 },  // Conakry, Guinea (2025 Population: 2251590)
    { .name = "N-Djamena", .latitude = 1212, .longitude = 1505, .region = 3 },  // N-Djamena, Chad (2025 Population: 1722780)
    { .name = "Kisangani", .latitude = 52, .longitude = 2521, .region = 3 },  // Kisangani, DR Congo (2025 Population: 1546690)
    { .name = "Lilongwe", .latitude = -1399, .longitude = 3377, .region = 3 },  // Lilongwe, Malawi (2025 Population: 1393010)
    { .name = "Bukavu", .latitude = -251, .longitude = 2886, .region = 3 },  // Bukavu, DR Congo (2025 Population: 1369430)
    { .name = "Port Elizabeth", .latitude = -3396, .longitude = 2562, .region = 3 },  // Port Elizabeth, South Africa (2025 Population: 1330500)
    { .name = "Hargeysa", .latitude = 956, .longitude = 4406, .region = 3 },  // Hargeysa, Somalia (2025 Population: 1227620)
    { .name = "Asmara", .latitude = 1534, .longitude = 3893, .region = 3 },  // Asmara, Eritrea (2025 Population: 1152180)
    { .name = "Nyala", .latitude = 1228, .longitude = 2477, .region = 3 },  // Nyala, Sudan (2025 Population: 1145590)
    { .name = "Nampula", .latitude = -1497, .longitude = 3927, .region = 3 },  // Nampula, Mozambique (2025 Population: 1057290)
    { .name = "Lubango", .latitude = -1492, .longitude = 1349, .region = 3 },  // Lubango, Angola (2025 Population: 1047810)
    { .name = "Misratah", .latitude = 3237, .longitude = 1509, .region = 3 },  // Misratah, Libya (2025 Population: 1034680)
    { .name = "Bangui", .latitude = 436, .longitude = 1858, .region = 3 },  // Bangui, Central African Republic (2025 Population: 1016150)
    // South America
    { .name = "Sao Paulo", .latitude = -2355, .longitude = -4663, .region = 4 },  // Sao Paulo, Brazil (2025 Population: 22990000)
    { .name = "Buenos Aires", .latitude = -3461, .longitude = -5839, .region = 4 },  // Buenos Aires, Argentina (2025 Population: 15752300)
    { .name = "Bogota", .latitude = 465, .longitude = -7408, .region = 4 },  // Bogota, Colombia (2025 Population: 11795800)
    { .name = "Lima", .latitude = -1205, .longitude = -7703, .region = 4 },  // Lima, Peru (2025 Population: 11517300)
    { .name = "Santiago", .latitude = -3344, .longitude = -7065, .region = 4 },  // Santiago, Chile (2025 Population: 6999460)
    { .name = "Brasilia", .latitude = -1033, .longitude = -5320, .region = 4 },  // Brasilia, Brazil (2025 Population: 4990930)
    { .name = "Recife", .latitude = -806, .longitude = -3488, .region = 4 },  // Recife, Brazil (2025 Population: 4344050)
    { .name = "Fortaleza", .latitude = -373, .longitude = -3852, .region = 4 },  // Fortaleza, Brazil (2025 Population: 4284450)
    { .name = "Porto Alegre", .latitude = -3003, .longitude = -5123, .region = 4 },  // Porto Alegre, Brazil (2025 Population: 4268960)
    { .name = "Salvador", .latitude = -1298, .longitude = -3848, .region = 4 },  // Salvador, Brazil (2025 Population: 4029910)
    { .name = "Asuncion", .latitude = -2528, .longitude = -5763, .region = 4 },  // Asuncion, Paraguay (2025 Population: 3627220)
    { .name = "Guayaquil", .latitude = -229, .longitude = -8010, .region = 4 },  // Guayaquil, Ecuador (2025 Population: 3244750)
    { .name = "Caracas", .latitude = 1051, .longitude = -6691, .region = 4 },  // Caracas, Venezuela (2025 Population: 3015110)
    { .name = "Goiania", .latitude = -1668, .longitude = -4925, .region = 4 },  // Goiania, Brazil (2025 Population: 2927080)
    { .name = "Belem", .latitude = -145, .longitude = -4847, .region = 4 },  // Belem, Brazil (2025 Population: 2453800)
    { .name = "Manaus", .latitude = -313, .longitude = -5998, .region = 4 },  // Manaus, Brazil (2025 Population: 2434640)
    { .name = "Maracaibo", .latitude = 1065, .longitude = -7164, .region = 4 },  // Maracaibo, Venezuela (2025 Population: 2432440)
    { .name = "La Paz", .latitude = -1650, .longitude = -6813, .region = 4 },  // La Paz, Bolivia (2025 Population: 1997370)
    { .name = "Santa Cruz", .latitude = -1733, .longitude = -6150, .region = 4 },  // Santa Cruz, Bolivia (2025 Population: 1891230)
    { .name = "Cordoba", .latitude = -3142, .longitude = -6418, .region = 4 },  // Cordoba, Argentina (2025 Population: 1640600)
    { .name = "Grande Sao Luis", .latitude = -302, .longitude = -4400, .region = 4 },  // Grande Sao Luis, Brazil (2025 Population: 1548210)
    { .name = "San Miguel de Tucuman", .latitude = -2683, .longitude = -6520, .region = 4 },  // San Miguel de Tucuman, Argentina (2025 Population: 1051040)
    { .name = "Ciudad Guayana", .latitude = 832, .longitude = -6269, .region = 4 },  // Ciudad Guayana, Venezuela (2025 Population: 991388)
    { .name = "Campo Grande", .latitude = -2046, .longitude = -5462, .region = 4 },  // Campo Grande, Brazil (2025 Population: 943313)
    // Oceania
    { .name = "Melbourne", .latitude = -3781, .longitude = 14496, .region = 5 },  // Melbourne, Australia (2025 Population: 5391890)
    { .name = "Sydney", .latitude = -3387, .longitude = 15121, .region = 5 },  // Sydney, Australia (2025 Population: 5248790)
    { .name = "Brisbane", .latitude = -2747, .longitude = 15302, .region = 5 },  // Brisbane, Australia (2025 Population: 2568170)
    { .name = "Perth", .latitude = -3196, .longitude = 11586, .region = 5 },  // Perth, Australia (2025 Population: 2169190)
    { .name = "Auckland", .latitude = -3685, .longitude = 17476, .region = 5 },  // Auckland, New Zealand (2025 Population: 1711130)
    { .name = "Adelaide", .latitude = -3493, .longitude = 13860, .region = 5 },  // Adelaide, Australia (2025 Population: 1392940)
};

#endif // LOCATION_FACE_H_
