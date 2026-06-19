// Line-up - https://clashfinder.com/data/event/b2026.json
#include "festival_schedule_face.h"

#define FESTIVAL_SCHEDULE_NUM_ACTS 115
#define FESTIVAL_SCHEDULE_GCF_MINUTE 5

const festival_schedule_t festival_acts[FESTIVAL_SCHEDULE_NUM_ACTS + 1]=
{
    {
        .artist = "a HUNdREd dRUMS",
        .stage = FESTIVAL_SCHEDULE_STAGE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 15, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 16, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_EDM,
        .popularity = 110
    },
    {
        .artist = "adVENTURE CLUB",
        .stage = FESTIVAL_SCHEDULE_STAGE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 21, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 22, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_DUBSTEP,
        .popularity = 60
    },
    {
        .artist = "aLaBaMa SHaKES",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 18, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 19, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_SOUL,
        .popularity = 27
    },
    {
        .artist = "aLY `n aJ",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 14, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 15, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_POP,
        .popularity = 31
    },
    {
        .artist = "aMBLE ",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 15, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 16, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_WORLD,
        .popularity = 88
    },
    {
        .artist = "aMYL aNd THE SNIFFERS",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHICH,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 17, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 18, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_PUNK,
        .popularity = 45
    },
    {
        .artist = "aRCY dRIVE",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 15, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 16, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_ROCK,
        .popularity = 83
    },
    {
        .artist = "aUdREY HOBERT",
        .stage = FESTIVAL_SCHEDULE_STAGE_THAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 16, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 17, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_POP,
        .popularity = 50
    },
    {
        .artist = "BBNO$ ",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHICH,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 16, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 17, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_RAP,
        .popularity = 13
    },
    {
        .artist = "BIG GIGaNTIC",
        .stage = FESTIVAL_SCHEDULE_STAGE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 17, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 18, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_EDM,
        .popularity = 61
    },
    {
        .artist = "BIG GIGaNTIC 2",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHERE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 3, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 4, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_EDM,
        .popularity = 61
    },
    {
        .artist = "BLONdSHELL",
        .stage = FESTIVAL_SCHEDULE_STAGE_THAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 15, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 15, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_ROCK,
        .popularity = 57
    },
    {
        .artist = "BLOOd ORaNGE",
        .stage = FESTIVAL_SCHEDULE_STAGE_THAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 22, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 23, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_ALT,
        .popularity = 7
    },
    {
        .artist = "BLUES TRaVELER",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 16, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 17, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_ROCK,
        .popularity = 43
    },
    {
        .artist = "BOYS NOIZE",
        .stage = FESTIVAL_SCHEDULE_STAGE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 19, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 20, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_EDM,
        .popularity = 39
    },
    {
        .artist = "BUFFaLO TRaFFIC JaM",
        .stage = FESTIVAL_SCHEDULE_STAGE_THIS,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 13, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 14, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_FOLK,
        .popularity = 86
    },
    {
        .artist = "CHaSE `n STaTUS",
        .stage = FESTIVAL_SCHEDULE_STAGE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 0, .unit.minute = 40},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 1, .unit.minute = 55},
        .genre = FESTIVAL_SCHEDULE_GENRE_DnB,
        .popularity = 25
    },
    {
        .artist = "CHaTS ",
        .stage = FESTIVAL_SCHEDULE_STAGE_THAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 16, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 16, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_PUNK,
        .popularity = 58
    },
    {
        .artist = "CLIPSE",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHICH,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 18, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 19, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_RAP,
        .popularity = 23
    },
    {
        .artist = "CLOONEE",
        .stage = FESTIVAL_SCHEDULE_STAGE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 0, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 1, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_HOUSE,
        .popularity = 67
    },
    {
        .artist = "CLOZEE",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHERE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 2, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 3, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_EDM,
        .popularity = 74
    },
    {
        .artist = "CONFIdENCE MaN",
        .stage = FESTIVAL_SCHEDULE_STAGE_THAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 14, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 15, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_GENRE_DANCE,
        .popularity = 65
    },
    {
        .artist = "CONGRESS THE BaNd",
        .stage = FESTIVAL_SCHEDULE_STAGE_THIS,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 14, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 14, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_ROCK,
        .popularity = 103
    },
    {
        .artist = "COSTa ",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHERE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 23, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 0, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_GENRE_EDM,
        .popularity = 93
    },
    {
        .artist = "daILY BREad",
        .stage = FESTIVAL_SCHEDULE_STAGE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 18, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 19, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_WORLD,
        .popularity = 94
    },
    {
        .artist = "daNIEL aLLaN",
        .stage = FESTIVAL_SCHEDULE_STAGE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 17, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 18, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_GENRE_HOUSE,
        .popularity = 80
    },
    {
        .artist = "daRE  ",
        .stage = FESTIVAL_SCHEDULE_STAGE_THIS,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 1, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 2, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_EDM,
        .popularity = 48
    },
    {
        .artist = "dEaTHPaCT",
        .stage = FESTIVAL_SCHEDULE_STAGE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 18, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 19, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_GENRE_EDM,
        .popularity = 89
    },
    {
        .artist = "dEL WaTER GaP",
        .stage = FESTIVAL_SCHEDULE_STAGE_THAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 18, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 19, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_POP,
        .popularity = 36
    },
    {
        .artist = "dORa JaR",
        .stage = FESTIVAL_SCHEDULE_STAGE_THAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 13, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 13, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_POP,
        .popularity = 63
    },
    {
        .artist = "EaZYBaKEd",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHERE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 4, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 5, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_EDM,
        .popularity = 102
    },
    {
        .artist = "EFFIN ",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHERE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 5, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 6, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_DUBSTEP,
        .popularity = 92
    },
    {
        .artist = "FCUKERS",
        .stage = FESTIVAL_SCHEDULE_STAGE_THIS,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 16, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 17, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_HOUSE,
        .popularity = 59
    },
    {
        .artist = "FLIPTURN",
        .stage = FESTIVAL_SCHEDULE_STAGE_THAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 22, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 23, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_ROCK,
        .popularity = 41
    },
    {
        .artist = "FOUR TET",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 11, .unit.hour = 20, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 11, .unit.hour = 22, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_EDM,
        .popularity = 19
    },
    {
        .artist = "FREddIE GIBBS",
        .stage = FESTIVAL_SCHEDULE_STAGE_THAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 0, .unit.minute = 50},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 2, .unit.minute = 5},
        .genre = FESTIVAL_SCHEDULE_GENRE_RAP,
        .popularity = 10
    },
    {
        .artist = "GaNJa WHITE NIGHT",
        .stage = FESTIVAL_SCHEDULE_STAGE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 2, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 4, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_DUBSTEP,
        .popularity = 76
    },
    {
        .artist = "GEESE ",
        .stage = FESTIVAL_SCHEDULE_STAGE_THAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 19, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 20, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_PUNK,
        .popularity = 24
    },
    {
        .artist = "GIRL TONES",
        .stage = FESTIVAL_SCHEDULE_STAGE_THAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 13, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 14, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_GENRE_ROCK,
        .popularity = 98
    },
    {
        .artist = "GOLdIE BOUTILIER",
        .stage = FESTIVAL_SCHEDULE_STAGE_THIS,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 14, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 15, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_POP,
        .popularity = 69
    },
    {
        .artist = "GORGON CITY",
        .stage = FESTIVAL_SCHEDULE_STAGE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 2, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 5, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_GENRE_HOUSE,
        .popularity = 47
    },
    {
        .artist = "GRIZ  ",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 20, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 22, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_DUBSTEP,
        .popularity = 56
    },
    {
        .artist = "HEMLOCKE SPRINGS",
        .stage = FESTIVAL_SCHEDULE_STAGE_THIS,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 15, .unit.minute = 20},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 16, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_POP,
        .popularity = 54
    },
    {
        .artist = "HOLLY HUMBERSTONE",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHICH,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 16, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 17, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_POP,
        .popularity = 42
    },
    {
        .artist = "HOT MULLIGaN",
        .stage = FESTIVAL_SCHEDULE_STAGE_THIS,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 21, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 22, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_PUNK,
        .popularity = 32
    },
    {
        .artist = "INZO  ",
        .stage = FESTIVAL_SCHEDULE_STAGE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 4, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 5, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_EDM,
        .popularity = 73
    },
    {
        .artist = "JaCKIE HOLLaNdER",
        .stage = FESTIVAL_SCHEDULE_STAGE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 16, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 17, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_EDM,
        .popularity = 99
    },
    {
        .artist = "JaPaNESE BREaKFaST",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHICH,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 16, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 17, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_ALT,
        .popularity = 14
    },
    {
        .artist = "JESSIE MURPH",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHICH,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 20, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 21, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_POP,
        .popularity = 52
    },
    {
        .artist = "JUELZ ",
        .stage = FESTIVAL_SCHEDULE_STAGE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 17, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 18, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_RAP,
        .popularity = 85
    },
    {
        .artist = "KESHa ",
        .stage = FESTIVAL_SCHEDULE_STAGE_THIS,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 20, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 22, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_POP,
        .popularity = 6
    },
    {
        .artist = "KESHa 2",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHICH,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 20, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 21, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_POP,
        .popularity = 6
    },
    {
        .artist = "LaMBRINI GIRLS",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHICH,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 13, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 14, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_PUNK,
        .popularity = 78
    },
    {
        .artist = "LaSZEWO",
        .stage = FESTIVAL_SCHEDULE_STAGE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 18, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 19, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_EDM,
        .popularity = 72
    },
    {
        .artist = "LIL JON",
        .stage = FESTIVAL_SCHEDULE_STAGE_THAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 0, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 1, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_RAP,
        .popularity = 46
    },
    {
        .artist = "LITTLE STRaNGER",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHICH,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 13, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 14, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_ALT,
        .popularity = 95
    },
    {
        .artist = "LSZEE ",
        .stage = FESTIVAL_SCHEDULE_STAGE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 20, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 21, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_DUBSTEP,
        .popularity = 77
    },
    {
        .artist = "LUMaSI",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHERE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 5, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 6, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_DUBSTEP,
        .popularity = 108
    },
    {
        .artist = "MaJOR LaZER",
        .stage = FESTIVAL_SCHEDULE_STAGE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 1, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 2, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_EDM,
        .popularity = 9
    },
    {
        .artist = "MaRIaH THE SCIENTIST",
        .stage = FESTIVAL_SCHEDULE_STAGE_THAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 20, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 21, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_SOUL,
        .popularity = 38
    },
    {
        .artist = "MaRY dROPPINZ",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHERE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 2, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 3, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_DUBSTEP,
        .popularity = 101
    },
    {
        .artist = "MIdNIGHT GENERaTION",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 13, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 14, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_DANCE,
        .popularity = 90
    },
    {
        .artist = "MOdEST MOUSE",
        .stage = FESTIVAL_SCHEDULE_STAGE_THIS,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 20, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 21, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_ROCK,
        .popularity = 3
    },
    {
        .artist = "MOTHER MOTHER",
        .stage = FESTIVAL_SCHEDULE_STAGE_THIS,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 18, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 19, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_ALT,
        .popularity = 8
    },
    {
        .artist = "MOTIFV",
        .stage = FESTIVAL_SCHEDULE_STAGE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 13, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 14, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_EDM,
        .popularity = 111
    },
    {
        .artist = "MOUNTaIN GRaSS UNIT",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHICH,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 14, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 15, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_GENRE_FOLK,
        .popularity = 109
    },
    {
        .artist = "MT_ JOY",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHICH,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 22, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 23, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_FOLK,
        .popularity = 35
    },
    {
        .artist = "NaT MYERS",
        .stage = FESTIVAL_SCHEDULE_STAGE_THIS,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 12, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 13, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_GENRE_SOUL,
        .popularity = 107
    },
    {
        .artist = "NEIGHBOURHOOd",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 20, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 22, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_ALT,
        .popularity = 2
    },
    {
        .artist = "NIKITa, THE WICKEd",
        .stage = FESTIVAL_SCHEDULE_STAGE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 15, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 16, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_EDM,
        .popularity = 96
    },
    {
        .artist = "NOaH KaHaN",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 21, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 23, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_ALT,
        .popularity = 4
    },
    {
        .artist = "NOTION",
        .stage = FESTIVAL_SCHEDULE_STAGE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 19, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 20, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_DnB,
        .popularity = 62
    },
    {
        .artist = "OSEES ",
        .stage = FESTIVAL_SCHEDULE_STAGE_THIS,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 0, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 1, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_ROCK,
        .popularity = 84
    },
    {
        .artist = "PaSSION PIT",
        .stage = FESTIVAL_SCHEDULE_STAGE_THIS,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 18, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 19, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_EDM,
        .popularity = 11
    },
    {
        .artist = "PaWPaW ROd",
        .stage = FESTIVAL_SCHEDULE_STAGE_THIS,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 13, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 14, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_GENRE_RAP,
        .popularity = 66
    },
    {
        .artist = "PROBCaUSE",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHERE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 23, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 0, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_RAP,
        .popularity = 100
    },
    {
        .artist = "R;F;S dU SOL",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 23, .unit.minute = 10},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 0, .unit.minute = 40},
        .genre = FESTIVAL_SCHEDULE_GENRE_EDM,
        .popularity = 30
    },
    {
        .artist = "RaCHEL CHINOURIRI",
        .stage = FESTIVAL_SCHEDULE_STAGE_THIS,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 16, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 17, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_GENRE_POP,
        .popularity = 37
    },
    {
        .artist = "RaINBOW KITTEN SURPRISE",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHICH,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 19, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 20, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_ROCK,
        .popularity = 21
    },
    {
        .artist = "RICHaRd FINGER",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHERE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 3, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 4, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_DUBSTEP,
        .popularity = 112
    },
    {
        .artist = "ROLE MOdEL",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 19, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 20, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_OTHER,
        .popularity = 18
    },
    {
        .artist = "RUNaROUNdS",
        .stage = FESTIVAL_SCHEDULE_STAGE_THIS,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 17, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 18, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_WORLD,
        .popularity = 91
    },
    {
        .artist = "SaN HOLO",
        .stage = FESTIVAL_SCHEDULE_STAGE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 16, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 17, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_GENRE_EDM,
        .popularity = 53
    },
    {
        .artist = "SaRa LaNdRY",
        .stage = FESTIVAL_SCHEDULE_STAGE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 22, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 23, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_EDM,
        .popularity = 75
    },
    {
        .artist = "SG LEWIS",
        .stage = FESTIVAL_SCHEDULE_STAGE_THAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 19, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 20, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_EDM,
        .popularity = 40
    },
    {
        .artist = "SIdEPIECE",
        .stage = FESTIVAL_SCHEDULE_STAGE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 22, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 23, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_GENRE_HOUSE,
        .popularity = 79
    },
    {
        .artist = "SKRILLEX",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 11, .unit.hour = 22, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 0, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_DUBSTEP,
        .popularity = 5
    },
    {
        .artist = "SMINO ",
        .stage = FESTIVAL_SCHEDULE_STAGE_THIS,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 19, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 20, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_RAP,
        .popularity = 22
    },
    {
        .artist = "SMOaKLaNd",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHERE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 4, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 5, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_DUBSTEP,
        .popularity = 104
    },
    {
        .artist = "SNOW STRIPPERS",
        .stage = FESTIVAL_SCHEDULE_STAGE_THIS,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 2, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 2, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_EDM,
        .popularity = 16
    },
    {
        .artist = "SPaCEY JaNE",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHICH,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 14, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 15, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_ROCK,
        .popularity = 49
    },
    {
        .artist = "SPIRITUaL CRaMP",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 11, .unit.hour = 17, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 11, .unit.hour = 18, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_PUNK,
        .popularity = 87
    },
    {
        .artist = "STEPH STRINGS",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHICH,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 13, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 14, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_POP,
        .popularity = 105
    },
    {
        .artist = "STEVE `n CORY aRE dEad",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHERE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 18, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 20, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_ROCK,
        .popularity = 113
    },
    {
        .artist = "STEWS ",
        .stage = FESTIVAL_SCHEDULE_STAGE_THAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 13, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 13, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_ROCK,
        .popularity = 97
    },
    {
        .artist = "STROKES",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 23, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 0, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_GENRE_ROCK,
        .popularity = 1
    },
    {
        .artist = "SUB FOCUS",
        .stage = FESTIVAL_SCHEDULE_STAGE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 20, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 21, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_DnB,
        .popularity = 34
    },
    {
        .artist = "SUNaMI",
        .stage = FESTIVAL_SCHEDULE_STAGE_THIS,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 12, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 13, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_PUNK,
        .popularity = 64
    },
    {
        .artist = "TaSH SULTaNa",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 16, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 17, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_ALT,
        .popularity = 51
    },
    {
        .artist = "TEddY SWIMS",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHICH,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 21, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 23, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_SOUL,
        .popularity = 29
    },
    {
        .artist = "TEdESCHI TRUCKS BaNd",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 17, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 18, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_ROCK,
        .popularity = 68
    },
    {
        .artist = "TRIXIE MaTTEL",
        .stage = FESTIVAL_SCHEDULE_STAGE_THAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 16, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 16, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_COUNTRY,
        .popularity = 70
    },
    {
        .artist = "TROMBONE SHORTY",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 15, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 16, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_SOUL,
        .popularity = 81
    },
    {
        .artist = "TURNOVER",
        .stage = FESTIVAL_SCHEDULE_STAGE_THIS,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 18, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 19, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_PUNK,
        .popularity = 33
    },
    {
        .artist = "TURNSTILE",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHICH,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 0, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 1, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_PUNK,
        .popularity = 17
    },
    {
        .artist = "VILLaNELLE",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 13, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 14, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_GENRE_ROCK,
        .popularity = 106
    },
    {
        .artist = "VINCE STaPLES",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 11, .unit.hour = 19, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 11, .unit.hour = 20, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_RAP,
        .popularity = 12
    },
    {
        .artist = "WaYLON WYaTT",
        .stage = FESTIVAL_SCHEDULE_STAGE_THIS,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 15, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 16, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_GENRE_WORLD,
        .popularity = 82
    },
    {
        .artist = "WEdNESdaY",
        .stage = FESTIVAL_SCHEDULE_STAGE_THAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 14, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 15, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_GENRE_ALT,
        .popularity = 44
    },
    {
        .artist = "WEIRd aL YaNKOVIC",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHICH,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 0, .unit.minute = 40},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 2, .unit.minute = 10},
        .genre = FESTIVAL_SCHEDULE_GENRE_OTHER,
        .popularity = 28
    },
    {
        .artist = "WET LEG",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHICH,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 18, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 19, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_GENRE_ROCK,
        .popularity = 26
    },
    {
        .artist = "WOLFMOTHER",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHICH,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 14, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 15, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_GENRE_ROCK,
        .popularity = 15
    },
    {
        .artist = "WYaTT FLORES",
        .stage = FESTIVAL_SCHEDULE_STAGE_THAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 17, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 18, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_COUNTRY,
        .popularity = 71
    },
    {
        .artist = "YUNGBLUd",
        .stage = FESTIVAL_SCHEDULE_STAGE_WHAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 18, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 19, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_ROCK,
        .popularity = 20
    },
    {
        .artist = "ZaCK FOX",
        .stage = FESTIVAL_SCHEDULE_STAGE_THAT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 17, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 18, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_GENRE_RAP,
        .popularity = 55
    },
    [FESTIVAL_SCHEDULE_NUM_ACTS]  = { //Fall back
        .artist = "No Act",
        .stage = FESTIVAL_SCHEDULE_STAGE_COUNT,
        .start_time = {.unit.year = 0, .unit.month = 0, .unit.day = 0, .unit.hour = 0, .unit.minute = 0},
        .end_time = {.unit.year = 63, .unit.month = 15, .unit.day = 31, .unit.hour = 31, .unit.minute = 63},
        .genre = FESTIVAL_SCHEDULE_GENRE_COUNT,
        .popularity = 0
    }
};
