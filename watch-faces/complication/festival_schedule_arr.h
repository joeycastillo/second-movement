// Line-up - https://clashfinder.com/data/event/bonnaroo2k26.json
#include "festival_schedule_face.h"

#define FESTIVAL_SCHEDULE_NUM_ACTS 101

const festival_schedule_t festival_acts[FESTIVAL_SCHEDULE_NUM_ACTS + 1]=
{
    {
        .artist = "a HUNdREd dRUMS",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 15, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 16, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_EDM,
        .popularity = 97
    },
    {
        .artist = "adVENTURE CLUB",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 19, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 20, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_DUBSTEP,
        .popularity = 60
    },
    {
        .artist = "aLaBaMa SHaKES",
        .stage = FESTIVAL_SCHEDULE_WHAT_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 18, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 19, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_SOUL,
        .popularity = 25
    },
    {
        .artist = "aLY `n aJ",
        .stage = FESTIVAL_SCHEDULE_WHICH_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 13, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 14, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_POP,
        .popularity = 29
    },
    {
        .artist = "aMBLE ",
        .stage = FESTIVAL_SCHEDULE_THAT_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 14, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 15, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_WORLD,
        .popularity = 83
    },
    {
        .artist = "aMYL aNd THE SNIFFERS",
        .stage = FESTIVAL_SCHEDULE_THAT_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 21, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 22, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_PUNK,
        .popularity = 45
    },
    {
        .artist = "aRCY dRIVE",
        .stage = FESTIVAL_SCHEDULE_THAT_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 14, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 14, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_ROCK,
        .popularity = 78
    },
    {
        .artist = "aUdREY HOBERT",
        .stage = FESTIVAL_SCHEDULE_THAT_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 14, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 15, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_POP,
        .popularity = 55
    },
    {
        .artist = "BBNO$ ",
        .stage = FESTIVAL_SCHEDULE_THIS_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 0, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 1, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_RAP,
        .popularity = 13
    },
    {
        .artist = "BIG GIGaNTIC",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 18, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 19, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_EDM,
        .popularity = 61
    },
    {
        .artist = "BLONdSHELL",
        .stage = FESTIVAL_SCHEDULE_WHICH_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 15, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 15, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_ROCK,
        .popularity = 58
    },
    {
        .artist = "BLOOd ORaNGE",
        .stage = FESTIVAL_SCHEDULE_THIS_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 18, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 19, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_ALT,
        .popularity = 7
    },
    {
        .artist = "BLUES TRaVELER",
        .stage = FESTIVAL_SCHEDULE_WHICH_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 15, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 16, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_ROCK,
        .popularity = 44
    },
    {
        .artist = "BOYS NOIZE",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 19, .unit.minute = 25},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 20, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_EDM,
        .popularity = 38
    },
    {
        .artist = "BUFFaLO TRaFFIC JaM",
        .stage = FESTIVAL_SCHEDULE_WHICH_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 14, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 15, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_FOLK,
        .popularity = 84
    },
    {
        .artist = "CHaSE `n STaTUS",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 22, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 23, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_DnB,
        .popularity = 24
    },
    {
        .artist = "CHaTS ",
        .stage = FESTIVAL_SCHEDULE_THIS_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 15, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 15, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_PUNK,
        .popularity = 59
    },
    {
        .artist = "CLIPSE",
        .stage = FESTIVAL_SCHEDULE_WHICH_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 18, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 19, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_RAP,
        .popularity = 23
    },
    {
        .artist = "CLOONEE",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 0, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 1, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_HOUSE,
        .popularity = 67
    },
    {
        .artist = "CONFIdENCE MaN",
        .stage = FESTIVAL_SCHEDULE_THIS_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 16, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 17, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_DANCE,
        .popularity = 65
    },
    {
        .artist = "CONGRESS THE BaNd",
        .stage = FESTIVAL_SCHEDULE_THAT_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 12, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 12, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_ROCK,
        .popularity = 94
    },
    {
        .artist = "daILY BREad",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 19, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 20, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_EDM,
        .popularity = 87
    },
    {
        .artist = "daNIEL aLLEN",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 15, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 16, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_EDM,
        .popularity = 100
    },
    {
        .artist = "daRE  ",
        .stage = FESTIVAL_SCHEDULE_THIS_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 2, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 3, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_EDM,
        .popularity = 49
    },
    {
        .artist = "dEaTHPaCT",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 18, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 19, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_EDM,
        .popularity = 82
    },
    {
        .artist = "dEL WaTER GaP",
        .stage = FESTIVAL_SCHEDULE_THAT_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 16, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 17, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_FOLK,
        .popularity = 36
    },
    {
        .artist = "dORa JaR",
        .stage = FESTIVAL_SCHEDULE_THIS_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 13, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 14, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_POP,
        .popularity = 62
    },
    {
        .artist = "FCUKERS",
        .stage = FESTIVAL_SCHEDULE_THIS_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 15, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 16, .unit.minute = 25},
        .genre = FESTIVAL_SCHEDULE_HOUSE,
        .popularity = 68
    },
    {
        .artist = "FLIPTURN",
        .stage = FESTIVAL_SCHEDULE_WHICH_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 17, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 18, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_ROCK,
        .popularity = 42
    },
    {
        .artist = "FOUR TET",
        .stage = FESTIVAL_SCHEDULE_WHAT_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 11, .unit.hour = 20, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 11, .unit.hour = 22, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_EDM,
        .popularity = 17
    },
    {
        .artist = "FREddY GIBBS",
        .stage = FESTIVAL_SCHEDULE_THIS_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 0, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 1, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_RAP,
        .popularity = 10
    },
    {
        .artist = "GEESE ",
        .stage = FESTIVAL_SCHEDULE_THAT_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 22, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 23, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_ROCK,
        .popularity = 26
    },
    {
        .artist = "GIRL TONES",
        .stage = FESTIVAL_SCHEDULE_THIS_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 12, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 13, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_ROCK,
        .popularity = 91
    },
    {
        .artist = "GOLdIE BOUTILIER",
        .stage = FESTIVAL_SCHEDULE_THAT_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 13, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 14, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_POP,
        .popularity = 71
    },
    {
        .artist = "GORGON CITY",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 2, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 3, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_HOUSE,
        .popularity = 47
    },
    {
        .artist = "GRIZ  ",
        .stage = FESTIVAL_SCHEDULE_WHAT_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 20, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 22, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_DUBSTEP,
        .popularity = 57
    },
    {
        .artist = "HEMLOCKE SPRINGS",
        .stage = FESTIVAL_SCHEDULE_THIS_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 14, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 15, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_POP,
        .popularity = 54
    },
    {
        .artist = "HOLLY HUMBERSTONE",
        .stage = FESTIVAL_SCHEDULE_WHAT_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 15, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 15, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_POP,
        .popularity = 43
    },
    {
        .artist = "HOT MULLIGaN",
        .stage = FESTIVAL_SCHEDULE_THAT_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 19, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 20, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_PUNK,
        .popularity = 32
    },
    {
        .artist = "JaCKIE HOLLaNdER",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 14, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 15, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_EDM,
        .popularity = 92
    },
    {
        .artist = "JaPaNESE BREaKFaST",
        .stage = FESTIVAL_SCHEDULE_THIS_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 17, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 18, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_ALT,
        .popularity = 14
    },
    {
        .artist = "JESSIE MURPH",
        .stage = FESTIVAL_SCHEDULE_WHICH_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 19, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 20, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_POP,
        .popularity = 53
    },
    {
        .artist = "JUELZ ",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 14, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 15, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_RAP,
        .popularity = 80
    },
    {
        .artist = "KESHa ",
        .stage = FESTIVAL_SCHEDULE_THIS_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 20, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 21, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_POP,
        .popularity = 6
    },
    {
        .artist = "KESHa 2",
        .stage = FESTIVAL_SCHEDULE_WHICH_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 20, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 21, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_POP,
        .popularity = 6
    },
    {
        .artist = "LaMBRINI GIRLS",
        .stage = FESTIVAL_SCHEDULE_WHICH_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 14, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 15, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_PUNK,
        .popularity = 76
    },
    {
        .artist = "LaSZEWO",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 17, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 18, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_EDM,
        .popularity = 73
    },
    {
        .artist = "LIL JON",
        .stage = FESTIVAL_SCHEDULE_THIS_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 20, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 21, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_RAP,
        .popularity = 46
    },
    {
        .artist = "LITTLE STRaNGER",
        .stage = FESTIVAL_SCHEDULE_THAT_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 13, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 14, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_ALT,
        .popularity = 88
    },
    {
        .artist = "LSZEE ",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 20, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 22, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_DUBSTEP,
        .popularity = 75
    },
    {
        .artist = "MaJOR LaZER",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 1, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 2, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_EDM,
        .popularity = 9
    },
    {
        .artist = "MaRIaH THE SCIENTIST",
        .stage = FESTIVAL_SCHEDULE_THIS_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 19, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 20, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_SOUL,
        .popularity = 40
    },
    {
        .artist = "MIdNIGHT GENERaTION",
        .stage = FESTIVAL_SCHEDULE_THIS_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 15, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 15, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_DANCE,
        .popularity = 85
    },
    {
        .artist = "MOdEST MOUSE",
        .stage = FESTIVAL_SCHEDULE_THAT_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 20, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 21, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_ROCK,
        .popularity = 3
    },
    {
        .artist = "MOTHER MOTHER",
        .stage = FESTIVAL_SCHEDULE_THIS_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 16, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 17, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_ALT,
        .popularity = 8
    },
    {
        .artist = "MOTIFV",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 14, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 15, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_EDM,
        .popularity = 98
    },
    {
        .artist = "MOUNTaIN GRaSS UNIT",
        .stage = FESTIVAL_SCHEDULE_THIS_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 13, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 14, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_FOLK,
        .popularity = 96
    },
    {
        .artist = "MT_ JOY",
        .stage = FESTIVAL_SCHEDULE_WHICH_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 21, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 23, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_FOLK,
        .popularity = 35
    },
    {
        .artist = "NEIGHBORHOOd",
        .stage = FESTIVAL_SCHEDULE_WHICH_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 21, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 22, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_ALT,
        .popularity = 2
    },
    {
        .artist = "NIKITa, THE WICKEd",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 13, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 14, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_EDM,
        .popularity = 89
    },
    {
        .artist = "NOaH KaHaN",
        .stage = FESTIVAL_SCHEDULE_WHAT_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 21, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 23, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_ALT,
        .popularity = 4
    },
    {
        .artist = "NOTION",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 18, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 19, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_DnB,
        .popularity = 63
    },
    {
        .artist = "OH SEES",
        .stage = FESTIVAL_SCHEDULE_THAT_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 17, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 18, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_ROCK,
        .popularity = 39
    },
    {
        .artist = "PaSSION PIT",
        .stage = FESTIVAL_SCHEDULE_THAT_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 19, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 20, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_EDM,
        .popularity = 11
    },
    {
        .artist = "PaWPaW ROd",
        .stage = FESTIVAL_SCHEDULE_THAT_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 12, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 13, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_RAP,
        .popularity = 66
    },
    {
        .artist = "R;F;S dU SOL",
        .stage = FESTIVAL_SCHEDULE_WHAT_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 22, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 0, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_EDM,
        .popularity = 31
    },
    {
        .artist = "RaCHEL CHINOURIRI",
        .stage = FESTIVAL_SCHEDULE_WHAT_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 16, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 17, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_POP,
        .popularity = 37
    },
    {
        .artist = "RaINBOW KITTEN SURPRISE",
        .stage = FESTIVAL_SCHEDULE_WHICH_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 19, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 20, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_ROCK,
        .popularity = 21
    },
    {
        .artist = "ROLE MOdEL",
        .stage = FESTIVAL_SCHEDULE_WHAT_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 19, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 20, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_OTHER,
        .popularity = 18
    },
    {
        .artist = "RUNaROUNdS",
        .stage = FESTIVAL_SCHEDULE_THAT_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 15, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 16, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_WORLD,
        .popularity = 86
    },
    {
        .artist = "SaN HOLO",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 17, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 18, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_EDM,
        .popularity = 52
    },
    {
        .artist = "SaRa LaNdRY",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 0, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 1, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_EDM,
        .popularity = 74
    },
    {
        .artist = "SG LEWIS",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 17, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 18, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_EDM,
        .popularity = 41
    },
    {
        .artist = "SIdEPIECE",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 22, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 23, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_HOUSE,
        .popularity = 77
    },
    {
        .artist = "SKRILLEX",
        .stage = FESTIVAL_SCHEDULE_WHAT_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 11, .unit.hour = 23, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 1, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_DUBSTEP,
        .popularity = 5
    },
    {
        .artist = "SMINO ",
        .stage = FESTIVAL_SCHEDULE_THAT_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 17, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 18, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_RAP,
        .popularity = 22
    },
    {
        .artist = "SNOW STRIPPERS",
        .stage = FESTIVAL_SCHEDULE_THIS_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 1, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 2, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_EDM,
        .popularity = 19
    },
    {
        .artist = "SPaCEY JaNE",
        .stage = FESTIVAL_SCHEDULE_WHAT_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 15, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 16, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_ROCK,
        .popularity = 50
    },
    {
        .artist = "SPIRITUaL CRaMP",
        .stage = FESTIVAL_SCHEDULE_WHAT_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 11, .unit.hour = 17, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 11, .unit.hour = 18, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_PUNK,
        .popularity = 81
    },
    {
        .artist = "STEPH STRINGS",
        .stage = FESTIVAL_SCHEDULE_THAT_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 12, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 13, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_POP,
        .popularity = 93
    },
    {
        .artist = "STEWS ",
        .stage = FESTIVAL_SCHEDULE_THAT_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 13, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 13, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_ROCK,
        .popularity = 90
    },
    {
        .artist = "STROKES",
        .stage = FESTIVAL_SCHEDULE_WHAT_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 23, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 0, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_ROCK,
        .popularity = 1
    },
    {
        .artist = "SUB FOCUS",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 20, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 21, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_DnB,
        .popularity = 34
    },
    {
        .artist = "SUNaMI",
        .stage = FESTIVAL_SCHEDULE_THIS_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 12, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 13, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_PUNK,
        .popularity = 64
    },
    {
        .artist = "TaSH SULTaNa",
        .stage = FESTIVAL_SCHEDULE_THIS_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 18, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 19, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_ALT,
        .popularity = 51
    },
    {
        .artist = "TEddY SWIMS",
        .stage = FESTIVAL_SCHEDULE_WHAT_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 20, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 21, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_SOUL,
        .popularity = 30
    },
    {
        .artist = "TEdESCHI TRUCKS BaNd",
        .stage = FESTIVAL_SCHEDULE_WHAT_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 17, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 18, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_ROCK,
        .popularity = 69
    },
    {
        .artist = "TRIXIE MaTTEL",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 15, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 16, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_COUNTRY,
        .popularity = 70
    },
    {
        .artist = "TROMBONE SHORTY",
        .stage = FESTIVAL_SCHEDULE_THAT_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 18, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 19, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_SOUL,
        .popularity = 99
    },
    {
        .artist = "TURNOVER",
        .stage = FESTIVAL_SCHEDULE_WHICH_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 16, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 17, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_PUNK,
        .popularity = 33
    },
    {
        .artist = "TURNSTILE",
        .stage = FESTIVAL_SCHEDULE_WHICH_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 0, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 2, .unit.minute = 0},
        .genre = FESTIVAL_SCHEDULE_PUNK,
        .popularity = 16
    },
    {
        .artist = "VILLaNELLE",
        .stage = FESTIVAL_SCHEDULE_THIS_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 12, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 13, .unit.minute = 15},
        .genre = FESTIVAL_SCHEDULE_ROCK,
        .popularity = 95
    },
    {
        .artist = "VINCE STaPLES",
        .stage = FESTIVAL_SCHEDULE_WHAT_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 11, .unit.hour = 18, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 11, .unit.hour = 19, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_RAP,
        .popularity = 12
    },
    {
        .artist = "WaYLON WYaTT",
        .stage = FESTIVAL_SCHEDULE_WHICH_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 15, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 16, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_WORLD,
        .popularity = 79
    },
    {
        .artist = "WEdNESdaY",
        .stage = FESTIVAL_SCHEDULE_WHAT_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 15, .unit.minute = 0},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 15, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_ALT,
        .popularity = 48
    },
    {
        .artist = "WEIRd aL YaNKOVIC",
        .stage = FESTIVAL_SCHEDULE_WHICH_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 0, .unit.minute = 15},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 14, .unit.hour = 1, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_OTHER,
        .popularity = 27
    },
    {
        .artist = "WET LEG",
        .stage = FESTIVAL_SCHEDULE_WHICH_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 17, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 18, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_ROCK,
        .popularity = 28
    },
    {
        .artist = "WOLFMOTHER",
        .stage = FESTIVAL_SCHEDULE_THAT_TENT,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 15, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 16, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_ROCK,
        .popularity = 15
    },
    {
        .artist = "WYaTT FLORES",
        .stage = FESTIVAL_SCHEDULE_WHAT_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 16, .unit.minute = 30},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 13, .unit.hour = 17, .unit.minute = 30},
        .genre = FESTIVAL_SCHEDULE_COUNTRY,
        .popularity = 72
    },
    {
        .artist = "YUNGBLUd",
        .stage = FESTIVAL_SCHEDULE_WHAT_STAGE,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 18, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 19, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_ROCK,
        .popularity = 20
    },
    {
        .artist = "ZaCK FOX",
        .stage = FESTIVAL_SCHEDULE_THE_OTHER,
        .start_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 20, .unit.minute = 45},
        .end_time = {.unit.year = 6, .unit.month = 6, .unit.day = 12, .unit.hour = 21, .unit.minute = 45},
        .genre = FESTIVAL_SCHEDULE_RAP,
        .popularity = 56
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
