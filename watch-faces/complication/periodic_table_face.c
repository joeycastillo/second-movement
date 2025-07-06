/*
 * MIT License
 *
 * Copyright (c) 2023 PrimmR
 * Copyright (c) 2024 David Volovskiy
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

#include "watch_slcd.h"
#include "watch_common_display.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "periodic_table_face.h"

#define FREQ_FAST 8
#define FREQ 2

static bool _quick_ticks_running;
static uint8_t _ts_ticks = 0;
static int16_t _text_pos;
static const char* _text_looping;
static const char title_text[] = "Periodic table";

void periodic_table_face_setup(uint8_t watch_face_index, void **context_ptr)
{
    (void)watch_face_index;
    if (*context_ptr == NULL)
    {
        *context_ptr = malloc(sizeof(periodic_table_state_t));
        memset(*context_ptr, 0, sizeof(periodic_table_state_t));
    }
}

void periodic_table_face_activate(void *context)
{
    periodic_table_state_t *state = (periodic_table_state_t *)context;

    state->atomic_num = 0;
    state->mode = 0;
    state->selection_index = 0;
    _quick_ticks_running = false;
    movement_request_tick_frequency(FREQ);
}

typedef enum {
    SCREEN_TITLE = 0,
    SCREEN_ELEMENT,
    SCREEN_ATOMIC_MASS,
    SCREEN_DISCOVER_YEAR,
    SCREEN_ELECTRONEGATIVITY,
    SCREEN_FULL_NAME,    
    SCREENS_COUNT
} PeriodicScreens;

const char screen_name[SCREENS_COUNT][3] = {
    [SCREEN_ATOMIC_MASS] = "ma",
    [SCREEN_DISCOVER_YEAR] = " y",
    [SCREEN_ELECTRONEGATIVITY] = "EL",
    [SCREEN_FULL_NAME] = " n",
};

// Compatability for original LCD
static inline const char* _get_screen_name(PeriodicScreens screen) {
    if (screen == SCREEN_ATOMIC_MASS && watch_get_lcd_type() != WATCH_LCD_TYPE_CUSTOM) return "am";
    return screen_name[screen];
}

typedef enum {
    NONE = 0,
    ZERO,
    ONE,
    TWO,
    THREE,
    FOUR,    
    FIVE,
    SIX,
    SEVEN,
    TRANSITION,
    LANTHANIDE,
    ACTINIDE,
    GROUPS_COUNT
} PeriodicGroup;

const char group_name[GROUPS_COUNT][3] = {
    [NONE] = "  ",
    [ZERO] = " 0",
    [ONE] = " 1",
    [TWO] = " 2",
    [THREE] = " 3",
    [FOUR] = " 4",
    [FIVE] = " 5",
    [SIX] = " 6",
    [SEVEN] = " 7",
    [TRANSITION] = " T",
    [ACTINIDE] = "Ac",
    [LANTHANIDE] = "La",
};

// Compatability for original LCD
static inline const char* _get_group_name(PeriodicGroup group) {
    if (group == LANTHANIDE && watch_get_lcd_type() != WATCH_LCD_TYPE_CUSTOM) return "1a";
    return group_name[group];
}

typedef struct
{
    char symbol[3];
    char name[14];  // Longest is Rutherfordium
    int16_t year_discovered;  // Negative is BC
    uint16_t atomic_mass;  // In units of 0.01 AMU
    uint16_t electronegativity;  // In units of 0.01
    PeriodicGroup group;
} element;

// Comments on the table denote symbols that cannot be displayed
#define MAX_ELEMENT 118
const element table[MAX_ELEMENT] = {
    { .symbol = "H", .name = "Hydrogen", .year_discovered = 1671, .atomic_mass = 101, .electronegativity = 220, .group = NONE },
    { .symbol = "He", .name = "Helium", .year_discovered = 1868, .atomic_mass = 400, .electronegativity = 0, .group = ZERO },
    { .symbol = "Li", .name = "Lithium", .year_discovered = 1817, .atomic_mass = 694, .electronegativity = 98, .group = ONE },
    { .symbol = "Be", .name = "Beryllium", .year_discovered = 1798, .atomic_mass = 901, .electronegativity = 157, .group = TWO },
    { .symbol = "B", .name = "Boron", .year_discovered = 1787, .atomic_mass = 1081, .electronegativity = 204, .group = THREE },
    { .symbol = "C", .name = "Carbon", .year_discovered = -26000, .atomic_mass = 1201, .electronegativity = 255, .group = FOUR },
    { .symbol = "N", .name = "Nitrogen", .year_discovered = 1772, .atomic_mass = 1401, .electronegativity = 304, .group = FIVE },
    { .symbol = "O", .name = "Oxygen", .year_discovered = 1771, .atomic_mass = 1600, .electronegativity = 344, .group = SIX },
    { .symbol = "F", .name = "Fluorine", .year_discovered = 1771, .atomic_mass = 1900, .electronegativity = 398, .group = SEVEN },
    { .symbol = "Ne", .name = "Neon", .year_discovered = 1898, .atomic_mass = 2018, .electronegativity = 0, .group = ZERO },
    { .symbol = "Na", .name = "Sodium", .year_discovered = 1702, .atomic_mass = 2299, .electronegativity = 93, .group = ONE },
    { .symbol = "Mg", .name = "Magnesium", .year_discovered = 1755, .atomic_mass = 2431, .electronegativity = 131, .group = TWO },
    { .symbol = "Al", .name = "Aluminium", .year_discovered = 1746, .atomic_mass = 2698, .electronegativity = 161, .group = THREE },
    { .symbol = "Si", .name = "Silicon", .year_discovered = 1739, .atomic_mass = 2809, .electronegativity = 190, .group = FOUR },
    { .symbol = "P", .name = "Phosphorus", .year_discovered = 1669, .atomic_mass = 3097, .electronegativity = 219, .group = FIVE },
    { .symbol = "S", .name = "Sulfur", .year_discovered = -2000, .atomic_mass = 3206, .electronegativity = 258, .group = SIX },
    { .symbol = "Cl", .name = "Chlorine", .year_discovered = 1774, .atomic_mass = 3545., .electronegativity = 316, .group = SEVEN },
    { .symbol = "Ar", .name = "Argon", .year_discovered = 1894, .atomic_mass = 3995., .electronegativity = 0, .group = ZERO },
    { .symbol = "K", .name = "Potassium", .year_discovered = 1702, .atomic_mass = 3910, .electronegativity = 82, .group = ONE },
    { .symbol = "Ca", .name = "Calcium", .year_discovered = 1739, .atomic_mass = 4008, .electronegativity = 100, .group = TWO },
    { .symbol = "Sc", .name = "Scandium", .year_discovered = 1879, .atomic_mass = 4496, .electronegativity = 136, .group = TRANSITION },
    { .symbol = "Ti", .name = "Titanium", .year_discovered = 1791, .atomic_mass = 4787, .electronegativity = 154, .group = TRANSITION },
    { .symbol = "W", .name = "Vanadium", .year_discovered = 1801, .atomic_mass = 5094, .electronegativity = 163, .group = TRANSITION },
    { .symbol = "Cr", .name = "Chromium", .year_discovered = 1797, .atomic_mass = 5200, .electronegativity = 166, .group = TRANSITION },
    { .symbol = "Mn", .name = "Manganese", .year_discovered = 1774, .atomic_mass = 5494, .electronegativity = 155, .group = TRANSITION },
    { .symbol = "Fe", .name = "Iron", .year_discovered = -5000, .atomic_mass = 5585, .electronegativity = 183, .group = TRANSITION },
    { .symbol = "Co", .name = "Cobalt", .year_discovered = 1735, .atomic_mass = 5893, .electronegativity = 188, .group = TRANSITION },
    { .symbol = "Ni", .name = "Nickel", .year_discovered = 1751, .atomic_mass = 5869, .electronegativity = 191, .group = TRANSITION },
    { .symbol = "Cu", .name = "Copper", .year_discovered = -9000, .atomic_mass = 6355, .electronegativity = 190, .group = TRANSITION },
    { .symbol = "Zn", .name = "Zinc", .year_discovered = -1000, .atomic_mass = 6538, .electronegativity = 165, .group = TRANSITION },
    { .symbol = "Ga", .name = "Gallium", .year_discovered = 1875, .atomic_mass = 6972, .electronegativity = 181, .group = THREE },
    { .symbol = "Ge", .name = "Germanium", .year_discovered = 1886, .atomic_mass = 7263, .electronegativity = 201, .group = FOUR },
    { .symbol = "As", .name = "Arsenic", .year_discovered = 300, .atomic_mass = 7492, .electronegativity = 218, .group = FIVE },
    { .symbol = "Se", .name = "Selenium", .year_discovered = 1817, .atomic_mass = 7897, .electronegativity = 255, .group = SIX },
    { .symbol = "Br", .name = "Bromine", .year_discovered = 1825, .atomic_mass = 7990., .electronegativity = 296, .group = SEVEN },
    { .symbol = "Kr", .name = "Krypton", .year_discovered = 1898, .atomic_mass = 8380, .electronegativity = 300, .group = ZERO },
    { .symbol = "Rb", .name = "Rubidium", .year_discovered = 1861, .atomic_mass = 8547, .electronegativity = 82, .group = ONE },
    { .symbol = "Sr", .name = "Strontium", .year_discovered = 1787, .atomic_mass = 8762, .electronegativity = 95, .group = TWO },
    { .symbol = "Y", .name = "Yttrium", .year_discovered = 1794, .atomic_mass = 8891, .electronegativity = 122, .group = TRANSITION },
    { .symbol = "Zr", .name = "Zirconium", .year_discovered = 1789, .atomic_mass = 9122, .electronegativity = 133, .group = TRANSITION },
    { .symbol = "Nb", .name = "Niobium", .year_discovered = 1801, .atomic_mass = 9291, .electronegativity = 160, .group = TRANSITION },
    { .symbol = "Mo", .name = "Molybdenum", .year_discovered = 1778, .atomic_mass = 9595, .electronegativity = 216, .group = TRANSITION },
    { .symbol = "Tc", .name = "Technetium", .year_discovered = 1937, .atomic_mass = 9700, .electronegativity = 190, .group = TRANSITION },
    { .symbol = "Ru", .name = "Ruthenium", .year_discovered = 1844, .atomic_mass = 10107, .electronegativity = 220, .group = TRANSITION },
    { .symbol = "Rh", .name = "Rhodium", .year_discovered = 1804, .atomic_mass = 10291, .electronegativity = 228, .group = TRANSITION },
    { .symbol = "Pd", .name = "Palladium", .year_discovered = 1802, .atomic_mass = 10642, .electronegativity = 220, .group = TRANSITION },
    { .symbol = "Ag", .name = "Silver", .year_discovered = -5000, .atomic_mass = 10787, .electronegativity = 193, .group = TRANSITION },
    { .symbol = "Cd", .name = "Cadmium", .year_discovered = 1817, .atomic_mass = 11241, .electronegativity = 169, .group = TRANSITION },
    { .symbol = "In", .name = "Indium", .year_discovered = 1863, .atomic_mass = 11482, .electronegativity = 178, .group = THREE },
    { .symbol = "Sn", .name = "Tin", .year_discovered = -3500, .atomic_mass = 11871, .electronegativity = 196, .group = FOUR },
    { .symbol = "Sb", .name = "Antimony", .year_discovered = -3000, .atomic_mass = 12176, .electronegativity = 205, .group = FIVE },
    { .symbol = "Te", .name = "Tellurium", .year_discovered = 1782, .atomic_mass = 12760, .electronegativity = 210, .group = SIX },
    { .symbol = "I", .name = "Iodine", .year_discovered = 1811, .atomic_mass = 12690, .electronegativity = 266, .group = SEVEN },
    { .symbol = "Xe", .name = "Xenon", .year_discovered = 1898, .atomic_mass = 13129, .electronegativity = 260, .group = ZERO },
    { .symbol = "Cs", .name = "Caesium", .year_discovered = 1860, .atomic_mass = 13291, .electronegativity = 79, .group = ONE },
    { .symbol = "Ba", .name = "Barium", .year_discovered = 1772, .atomic_mass = 13733., .electronegativity = 89, .group = TWO },
    { .symbol = "La", .name = "Lanthanum", .year_discovered = 1838, .atomic_mass = 13891, .electronegativity = 110, .group = LANTHANIDE },
    { .symbol = "Ce", .name = "Cerium", .year_discovered = 1803, .atomic_mass = 14012, .electronegativity = 112, .group = LANTHANIDE },
    { .symbol = "Pr", .name = "Praseodymium", .year_discovered = 1885, .atomic_mass = 14091, .electronegativity = 113, .group = LANTHANIDE },
    { .symbol = "Nd", .name = "Neodymium", .year_discovered = 1841, .atomic_mass = 14424, .electronegativity = 114, .group = LANTHANIDE },
    { .symbol = "Pm", .name = "Promethium", .year_discovered = 1945, .atomic_mass = 14500, .electronegativity = 113, .group = LANTHANIDE },
    { .symbol = "Sm", .name = "Samarium", .year_discovered = 1879, .atomic_mass = 15036., .electronegativity = 117, .group = LANTHANIDE },
    { .symbol = "Eu", .name = "Europium", .year_discovered = 1896, .atomic_mass = 15196, .electronegativity = 120, .group = LANTHANIDE },
    { .symbol = "Gd", .name = "Gadolinium", .year_discovered = 1880, .atomic_mass = 15725, .electronegativity = 120, .group = LANTHANIDE },
    { .symbol = "Tb", .name = "Terbium", .year_discovered = 1843, .atomic_mass = 15893, .electronegativity = 120, .group = LANTHANIDE },
    { .symbol = "Dy", .name = "Dysprosium", .year_discovered = 1886, .atomic_mass = 16250, .electronegativity = 122, .group = LANTHANIDE },
    { .symbol = "Ho", .name = "Holmium", .year_discovered = 1878, .atomic_mass = 16493, .electronegativity = 123, .group = LANTHANIDE },
    { .symbol = "Er", .name = "Erbium", .year_discovered = 1843, .atomic_mass = 16726, .electronegativity = 124, .group = LANTHANIDE },
    { .symbol = "Tm", .name = "Thulium", .year_discovered = 1879, .atomic_mass = 16893, .electronegativity = 125, .group = LANTHANIDE },
    { .symbol = "Yb", .name = "Ytterbium", .year_discovered = 1878, .atomic_mass = 17305, .electronegativity = 110, .group = LANTHANIDE },
    { .symbol = "Lu", .name = "Lutetium", .year_discovered = 1906, .atomic_mass = 17497, .electronegativity = 127, .group = LANTHANIDE },
    { .symbol = "Hf", .name = "Hafnium", .year_discovered = 1922, .atomic_mass = 17849, .electronegativity = 130, .group = TRANSITION },
    { .symbol = "Ta", .name = "Tantalum", .year_discovered = 1802, .atomic_mass = 18095, .electronegativity = 150, .group = TRANSITION },
    { .symbol = "W", .name = "Tungsten", .year_discovered = 1781, .atomic_mass = 18384, .electronegativity = 236, .group = TRANSITION },
    { .symbol = "Re", .name = "Rhenium", .year_discovered = 1908, .atomic_mass = 18621, .electronegativity = 190, .group = TRANSITION },
    { .symbol = "Os", .name = "Osmium", .year_discovered = 1803, .atomic_mass = 19023, .electronegativity = 220, .group = TRANSITION },
    { .symbol = "Ir", .name = "Iridium", .year_discovered = 1803, .atomic_mass = 19222, .electronegativity = 220, .group = TRANSITION },
    { .symbol = "Pt", .name = "Platinum", .year_discovered = -600, .atomic_mass = 19508, .electronegativity = 228, .group = TRANSITION },
    { .symbol = "Au", .name = "Gold", .year_discovered = -6000, .atomic_mass = 19697, .electronegativity = 254, .group = TRANSITION },
    { .symbol = "Hf", .name = "Mercury", .year_discovered = -1500, .atomic_mass = 20059, .electronegativity = 200, .group = TRANSITION },
    { .symbol = "Tl", .name = "Thallium", .year_discovered = 1861, .atomic_mass = 20438, .electronegativity = 162, .group = THREE },
    { .symbol = "Pb", .name = "Lead", .year_discovered = -7000, .atomic_mass = 20720, .electronegativity = 187, .group = FOUR },
    { .symbol = "Bi", .name = "Bismuth", .year_discovered = 1500, .atomic_mass = 20898, .electronegativity = 202, .group = FIVE },
    { .symbol = "Po", .name = "Polonium", .year_discovered = 1898, .atomic_mass = 20900, .electronegativity = 200, .group = SIX },
    { .symbol = "At", .name = "Astatine", .year_discovered = 1940, .atomic_mass = 21000, .electronegativity = 220, .group = SEVEN },
    { .symbol = "Rn", .name = "Radon", .year_discovered = 1899, .atomic_mass = 22200, .electronegativity = 220, .group = ZERO },
    { .symbol = "Fr", .name = "Francium", .year_discovered = 1939, .atomic_mass = 22300, .electronegativity = 79, .group = ONE },
    { .symbol = "Ra", .name = "Radium", .year_discovered = 1898, .atomic_mass = 22600, .electronegativity = 90, .group = TWO },
    { .symbol = "Ac", .name = "Actinium", .year_discovered = 1902, .atomic_mass = 22700, .electronegativity = 110, .group = ACTINIDE },
    { .symbol = "Th", .name = "Thorium", .year_discovered = 1829, .atomic_mass = 23204, .electronegativity = 130, .group = ACTINIDE },
    { .symbol = "Pa", .name = "Protactinium", .year_discovered = 1913, .atomic_mass = 23104, .electronegativity = 150, .group = ACTINIDE },
    { .symbol = "U", .name = "Uranium", .year_discovered = 1789, .atomic_mass = 23803, .electronegativity = 138, .group = ACTINIDE },
    { .symbol = "Np", .name = "Neptunium", .year_discovered = 1940, .atomic_mass = 23700, .electronegativity = 136, .group = ACTINIDE },
    { .symbol = "Pu", .name = "Plutonium", .year_discovered = 1941, .atomic_mass = 24400, .electronegativity = 128, .group = ACTINIDE },
    { .symbol = "Am", .name = "Americium", .year_discovered = 1944, .atomic_mass = 24300, .electronegativity = 113, .group = ACTINIDE },
    { .symbol = "Cm", .name = "Curium", .year_discovered = 1944, .atomic_mass = 24700, .electronegativity = 128, .group = ACTINIDE },
    { .symbol = "Bk", .name = "Berkelium", .year_discovered = 1949, .atomic_mass = 24700, .electronegativity = 130, .group = ACTINIDE },
    { .symbol = "Cf", .name = "Californium", .year_discovered = 1950, .atomic_mass = 25100, .electronegativity = 130, .group = ACTINIDE },
    { .symbol = "Es", .name = "Einsteinium", .year_discovered = 1952, .atomic_mass = 25200, .electronegativity = 130, .group = ACTINIDE },
    { .symbol = "Fm", .name = "Fermium", .year_discovered = 1953, .atomic_mass = 25700, .electronegativity = 130, .group = ACTINIDE },
    { .symbol = "Md", .name = "Mendelevium", .year_discovered = 1955, .atomic_mass = 25800, .electronegativity = 130, .group = ACTINIDE },
    { .symbol = "No", .name = "Nobelium", .year_discovered = 1965, .atomic_mass = 25900, .electronegativity = 130, .group = ACTINIDE },
    { .symbol = "Lr", .name = "Lawrencium", .year_discovered = 1961, .atomic_mass = 26600, .electronegativity = 130, .group = ACTINIDE },
    { .symbol = "Rf", .name = "Rutherfordium", .year_discovered = 1969, .atomic_mass = 26700, .electronegativity = 0, .group = TRANSITION },
    { .symbol = "Db", .name = "Dubnium", .year_discovered = 1970, .atomic_mass = 26800, .electronegativity = 0, .group = TRANSITION },
    { .symbol = "Sg", .name = "Seaborgium", .year_discovered = 1974, .atomic_mass = 26700, .electronegativity = 0, .group = TRANSITION },
    { .symbol = "Bh", .name = "Bohrium", .year_discovered = 1981, .atomic_mass = 27000, .electronegativity = 0, .group = TRANSITION },
    { .symbol = "Hs", .name = "Hassium", .year_discovered = 1984, .atomic_mass = 27100, .electronegativity = 0, .group = TRANSITION },
    { .symbol = "Mt", .name = "Meitnerium", .year_discovered = 1982, .atomic_mass = 27800, .electronegativity = 0, .group = TRANSITION },
    { .symbol = "Ds", .name = "Darmstadtium", .year_discovered = 1994, .atomic_mass = 28100, .electronegativity = 0, .group = TRANSITION },
    { .symbol = "Rg", .name = "Roentgenium", .year_discovered = 1994, .atomic_mass = 28200, .electronegativity = 0, .group = TRANSITION },
    { .symbol = "Cn", .name = "Copernicium", .year_discovered = 1996, .atomic_mass = 28500, .electronegativity = 0, .group = TRANSITION },
    { .symbol = "Nh", .name = "Nihonium", .year_discovered = 2004, .atomic_mass = 28600, .electronegativity = 0, .group = THREE },
    { .symbol = "Fl", .name = "Flerovium", .year_discovered = 1999, .atomic_mass = 28900, .electronegativity = 0, .group = FOUR },
    { .symbol = "Mc", .name = "Moscovium", .year_discovered = 2003, .atomic_mass = 29000, .electronegativity = 0, .group = FIVE },
    { .symbol = "Lw", .name = "Livermorium", .year_discovered = 2000, .atomic_mass = 29300, .electronegativity = 0, .group = SIX },
    { .symbol = "Ts", .name = "Tennessine", .year_discovered = 2009, .atomic_mass = 29400, .electronegativity = 0, .group = SEVEN },
    { .symbol = "Og", .name = "Oganesson", .year_discovered = 2002, .atomic_mass = 29400, .electronegativity = 0, .group = ZERO },
};

static void _make_upper(char *string) {
    size_t i = 0;
    while(string[i] != 0) {
        if (string[i] >= 'a' && string[i] <= 'z')
            string[i]-=32;  // 32 = 'a'-'A'
        i++;
    }
}

static void _display_element(periodic_table_state_t *state)
{
    char buf[7];
    char ele[3];
    uint8_t atomic_num = state->atomic_num;
    
    watch_display_text(WATCH_POSITION_TOP_RIGHT, _get_group_name(table[atomic_num - 1].group));

    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) { // Display symbol at top
        watch_display_text(WATCH_POSITION_TOP_LEFT, table[atomic_num - 1].symbol);
        sprintf(buf, "%3d", atomic_num);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    } else {
        strcpy(ele, table[atomic_num - 1].symbol);
        _make_upper(ele);
        sprintf(buf, "%3d %-2s", atomic_num, ele);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    }
}

static void _display_atomic_mass(periodic_table_state_t *state)
{
    char buf[7];
    uint16_t mass = table[state->atomic_num - 1].atomic_mass;
    uint16_t integer = mass / 100;
    uint16_t decimal = mass % 100;

    watch_display_text(WATCH_POSITION_TOP_LEFT, table[state->atomic_num - 1].symbol);
    watch_display_text(WATCH_POSITION_TOP_RIGHT, _get_screen_name(state->mode));

    if (decimal == 0) {
            sprintf(buf, "%4d", integer);
            watch_display_text(WATCH_POSITION_BOTTOM, buf); 
    } else {
        if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM && integer < 200) { // Display using decimal point
            watch_set_decimal_if_available();
            sprintf(buf, "%6d", mass);
            watch_display_text(WATCH_POSITION_BOTTOM, buf+2); // Truncate string to keep 0s in tens pos 
            if (integer >= 100) // Use extra 100s digit on the left
                watch_set_pixel(0, 22);
        } else { // Display using _
            sprintf(buf, "%3d_%.2d", integer, decimal);
            watch_display_text(WATCH_POSITION_BOTTOM, buf); 
        }
    }
}

static void _display_year_discovered(periodic_table_state_t *state)
{
    char year_buf[7];
    int16_t year = table[state->atomic_num - 1].year_discovered;
    if (abs(year) > 9999)
        sprintf(year_buf, "----  ");
    else
        sprintf(year_buf, "%4d  ", abs(year));
    if (year < 0) {
        year_buf[4] = 'b';
        year_buf[5] = 'c';
    }

    watch_display_text(WATCH_POSITION_TOP_LEFT, table[state->atomic_num - 1].symbol);
    watch_display_text(WATCH_POSITION_TOP_RIGHT, _get_screen_name(state->mode));
    watch_display_text(WATCH_POSITION_BOTTOM, year_buf); 
}

static void _display_name(periodic_table_state_t *state)
{
    watch_display_text(WATCH_POSITION_TOP_LEFT, table[state->atomic_num - 1].symbol);
    watch_display_text(WATCH_POSITION_TOP_RIGHT, _get_screen_name(state->mode));

    const char* elm_name = table[state->atomic_num - 1].name;

    // Better display for 'I' on new LCD
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM && elm_name[0] == 'I' && strlen(elm_name) <= 7){
        watch_display_text(WATCH_POSITION_BOTTOM, elm_name+1); 
        watch_set_pixel(0, 22);
        return;
    }


    _text_looping = elm_name;
    _text_pos = 0;

    char buf[7];
    sprintf(buf, "%.6s", elm_name);
    watch_display_text(WATCH_POSITION_BOTTOM, buf); 
}

static void _display_electronegativity(periodic_table_state_t *state)
{
    char buf[11];
    uint16_t electronegativity = table[state->atomic_num - 1].electronegativity;
    uint16_t integer = electronegativity / 100;
    uint16_t decimal = electronegativity % 100;

    watch_display_text(WATCH_POSITION_TOP_LEFT, table[state->atomic_num - 1].symbol);
    watch_display_text(WATCH_POSITION_TOP_RIGHT, _get_screen_name(state->mode));

    if (decimal == 0){
        sprintf(buf, "%4d", integer);
    } else {
        if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
            // Integer always under 100 so no need to handle extra place
            watch_set_decimal_if_available();
            sprintf(buf, "%4d", electronegativity);
        } else {
            sprintf(buf, "%3d_%.2d", integer, decimal);
        }
    }
    watch_display_text(WATCH_POSITION_BOTTOM, buf); 
}

static void start_quick_cyc(void){
    _quick_ticks_running = true;
    movement_request_tick_frequency(FREQ_FAST);
}

static void stop_quick_cyc(void){
    _quick_ticks_running = false;
    movement_request_tick_frequency(FREQ);
}

static int16_t _loop_text(const char* text, int8_t curr_loc, bool shift){
    // if curr_loc, then use that many ticks as a delay before looping
    char buf[15];
    uint8_t text_len = strlen(text);
    if (curr_loc == -1) curr_loc = 0;  // To avoid double-showing the 0
    if (text_len <= 6 || curr_loc < 0) {
        if (shift)
            sprintf(buf, " %.5s", text);
        else
            sprintf(buf, "%.6s", text);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
        if (curr_loc < 0) return ++curr_loc;
        return 0;
    }
    else if (curr_loc == (text_len + 1))
        curr_loc = 0;

    if (shift) // Extra space on title screen on F-91W screen
        sprintf(buf, " %.6s %.6s", text + curr_loc, text);  // Take wrapping substring
    else 
        sprintf(buf, "%.6s %.6s", text + curr_loc, text);  // Take wrapping substring

    buf[6] = '\0'; // Prevent overflow
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
    return curr_loc + 1; // Next position
}

static void _display_title(periodic_table_state_t *state){
    state->atomic_num = 0;
    watch_clear_colon();
    watch_clear_all_indicators();
    _text_looping = title_text;
    _text_pos = FREQ * -1;
    _text_pos = _loop_text(_text_looping, _text_pos, watch_get_lcd_type() != WATCH_LCD_TYPE_CUSTOM);
}

static void _display_screen(periodic_table_state_t *state, bool should_sound){
    watch_clear_display();
    watch_clear_all_indicators();
    switch (state->mode)
    {
    case SCREEN_TITLE:
        _display_title(state);
        break;
    case SCREEN_ELEMENT:
    case SCREENS_COUNT:
        _display_element(state);
        break;
    case SCREEN_ATOMIC_MASS:
        _display_atomic_mass(state);
        break;
    case SCREEN_DISCOVER_YEAR:
        _display_year_discovered(state);
        break;
    case SCREEN_ELECTRONEGATIVITY:
        _display_electronegativity(state);
        break;
    case SCREEN_FULL_NAME:
        _display_name(state);
        break;
    }
    if (should_sound) watch_buzzer_play_note(BUZZER_NOTE_C7, 50);
}

static void _handle_forward(periodic_table_state_t *state, bool should_sound){
    state->atomic_num = (state->atomic_num % MAX_ELEMENT) + 1; // Wraps back to 1
    state->mode = SCREEN_ELEMENT;
    _display_screen(state, false);
    if (should_sound) watch_buzzer_play_note(BUZZER_NOTE_C7, 50);
}

static void _handle_backward(periodic_table_state_t *state, bool should_sound){
    if (state->atomic_num <= 1) state->atomic_num = MAX_ELEMENT;
    else state->atomic_num = state->atomic_num - 1;
    state->mode = SCREEN_ELEMENT;
    _display_screen(state, false);
    if (should_sound) watch_buzzer_play_note(BUZZER_NOTE_A6, 50);
}

static void _handle_mode_still_pressed(periodic_table_state_t *state, bool should_sound) {
    if (_ts_ticks != 0){
        if (!HAL_GPIO_BTN_MODE_read()) {
            _ts_ticks = 0;
            return;
        }
        else if (--_ts_ticks == 0){
            switch (state->mode)
            {
            case SCREEN_TITLE:
                movement_move_to_face(0);
                return;
            case SCREEN_ELEMENT:
                state->mode = SCREEN_TITLE;
                _display_screen(state, should_sound);
                break;
            default:
                state->mode = SCREEN_ELEMENT;
                _display_screen(state, should_sound);
                break;
            }
            _ts_ticks = 2;
        }
    }
}

bool periodic_table_face_loop(movement_event_t event, void *context)
{
    periodic_table_state_t *state = (periodic_table_state_t *)context;
    switch (event.event_type)
    {
    case EVENT_ACTIVATE:
        state->mode = SCREEN_TITLE;
        _display_screen(state, false);
        break;
    case EVENT_TICK:
        if (state->mode == SCREEN_TITLE) _text_pos = _loop_text(_text_looping, _text_pos, watch_get_lcd_type() != WATCH_LCD_TYPE_CUSTOM);
        else if (state->mode == SCREEN_FULL_NAME && 
            !(watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM && table[state->atomic_num - 1].name[0] == 'I' && strlen(table[state->atomic_num - 1].name) <= 7)) 
                _text_pos = _loop_text(_text_looping, _text_pos, false);
        if (_quick_ticks_running) {
            if (HAL_GPIO_BTN_LIGHT_read()) _handle_backward(state, false);
            else if (HAL_GPIO_BTN_ALARM_read()) _handle_forward(state, false);
            else stop_quick_cyc();
        }

        _handle_mode_still_pressed(state, movement_button_should_sound());
        break;
    case EVENT_LIGHT_BUTTON_UP:
        if (state->mode <= SCREEN_ELEMENT) {
            _handle_backward(state, movement_button_should_sound());
        }
        else {
            state->mode = SCREEN_ELEMENT;
            _display_screen(state, movement_button_should_sound());
        }
        break;
    case EVENT_LIGHT_BUTTON_DOWN:
        break;
    case EVENT_ALARM_BUTTON_UP:
        if (state->mode <= SCREEN_ELEMENT) {
            _handle_forward(state, movement_button_should_sound());
        }
        else {
            state->mode = SCREEN_ELEMENT;
            _display_screen(state, movement_button_should_sound());
        }
        break;
    case EVENT_ALARM_LONG_PRESS:
        if (state->mode <= SCREEN_ELEMENT) {
            start_quick_cyc();
            _handle_forward(state, movement_button_should_sound());
        }
        break;
    case EVENT_LIGHT_LONG_PRESS:
        if (state->mode <= SCREEN_ELEMENT) {
            start_quick_cyc();
            _handle_backward(state, movement_button_should_sound());
        }
        else {
            movement_illuminate_led();
        }
        break;
    case EVENT_MODE_BUTTON_UP:
        if (state->mode == SCREEN_TITLE) movement_move_to_next_face();
        else {
            state->mode = (state->mode + 1) % SCREENS_COUNT;
            if (state->mode == SCREEN_TITLE)
                state->mode = (state->mode + 1) % SCREENS_COUNT;
            if (state->mode == SCREEN_ELEMENT){
                _display_screen(state, false);
                if (movement_button_should_sound()) watch_buzzer_play_note(BUZZER_NOTE_A6, 50);
            }
            else
                _display_screen(state, movement_button_should_sound());
        }
        break;
    case EVENT_MODE_LONG_PRESS:
        switch (state->mode)
        {
        case SCREEN_TITLE:
            return movement_default_loop_handler(event);
        default:
            state->mode = SCREEN_TITLE;
            _display_screen(state, movement_button_should_sound());
            break;
        }
        _ts_ticks = 2;
        break;
    case EVENT_TIMEOUT:
        // Display title after timeout
        if (state->mode == SCREEN_TITLE) break;
        state->mode = SCREEN_TITLE;
        _display_screen(state, false);
        break;
    case EVENT_LOW_ENERGY_UPDATE:
        // Display static title and tick animation during LE
        watch_clear_display();
        watch_display_text(WATCH_POSITION_TOP_LEFT, "Pd");
        watch_display_text(WATCH_POSITION_BOTTOM, "Table");
        watch_start_sleep_animation(500);
        break;
    default:
        return movement_default_loop_handler(event);
    }

    return true;
}

void periodic_table_face_resign(void *context)
{
    (void)context;

    // handle any cleanup before your watch face goes off-screen.
}
