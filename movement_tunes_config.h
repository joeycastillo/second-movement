/*
 * MIT License
 *
 * Copyright (c) 2025 Alessandro Genova
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

#ifndef MOVEMENT_TUNES_CONFIG_H_
#define MOVEMENT_TUNES_CONFIG_H_

/* You can include multiple tunes in the build, and pick the desired one at run-time
 * using the `tunes_face`
 */

/* Custom signal tune. Check movement_custom_signal_tunes.h for options. */
// #define INCLUDE_SIGNAL_TUNE_ZELDA_SECRET
// #define INCLUDE_SIGNAL_TUNE_MARIO_THEME
// #define INCLUDE_SIGNAL_TUNE_MARIO_1UP
// #define INCLUDE_SIGNAL_TUNE_MARIO_PUP
// #define INCLUDE_SIGNAL_TUNE_MGS_CODEC
// #define INCLUDE_SIGNAL_TUNE_KIM_POSSIBLE
// #define INCLUDE_SIGNAL_TUNE_POWER_RANGERS
// #define INCLUDE_SIGNAL_TUNE_LAYLA
// #define INCLUDE_SIGNAL_TUNE_HARRY_POTTER_SHORT
// #define INCLUDE_SIGNAL_TUNE_HARRY_POTTER_LONG
// #define INCLUDE_SIGNAL_TUNE_JURASSIC_PARK
// #define INCLUDE_SIGNAL_TUNE_EVANGELION
// #define INCLUDE_SIGNAL_TUNE_WINDOWS_XP
// #define INCLUDE_SIGNAL_TUNE_WHATSAPP
// #define INCLUDE_SIGNAL_TUNE_FF_VICTORY
// #define INCLUDE_SIGNAL_TUNE_GAME_BOY
// #define INCLUDE_SIGNAL_TUNE_GAME_BOY_PAUSE
// #define INCLUDE_SIGNAL_TUNE_WESTMINSTER
// #define INCLUDE_SIGNAL_TUNE_OCEAN
// #define INCLUDE_SIGNAL_TUNE_KIRBY_VICTORY
// #define INCLUDE_SIGNAL_TUNE_THIRD_SANCTUARY
// #define INCLUDE_SIGNAL_TUNE_MINECRAFT
// #define INCLUDE_SIGNAL_TUNE_SONIC_RING
// #define INCLUDE_SIGNAL_TUNE_NEO_GEO
// #define INCLUDE_SIGNAL_TUNE_BOSUN_WHISTLE
// #define INCLUDE_SIGNAL_TUNE_AMONG_US

/* Custom alarm tune. Check movement_custom_alarm_tunes.h for options. */
// #define INCLUDE_ALARM_TUNE_GIGI_DAG
// #define INCLUDE_ALARM_TUNE_TWINKLE
// #define INCLUDE_ALARM_TUNE_BABY
// #define INCLUDE_ALARM_TUNE_INTER
// #define INCLUDE_ALARM_TUNE_ELMO
// #define INCLUDE_ALARM_TUNE_FARM
// #define INCLUDE_ALARM_TUNE_ONEOFTHESEDAYS
// #define INCLUDE_ALARM_TUNE_BAD_APPLE                  // Bad Apple!! (Alstroemeria Records)
// #define INCLUDE_ALARM_TUNE_TO_HEART                   // Feeling Heart (ToHeart)
// #define INCLUDE_ALARM_TUNE_NOKIA                      // Nokia Ringtone
// #define INCLUDE_ALARM_TUNE_FLOWERING_NIGHT            // Flowering Night (Touhou 9)
// #define INCLUDE_ALARM_TUNE_PICTIONARY                 // Pictionary NES Title
// #define INCLUDE_ALARM_TUNE_E1M1                       // E1M1 (Doom)
// #define INCLUDE_ALARM_TUNE_NAZRIN                     // A Tiny, Tiny, Clever Commander (Touhou 11)
// #define INCLUDE_ALARM_TUNE_CIRNOS_PERFECT_MATH_CLASS  // Cirno's Perfect Math Class (IOSYS)
// #define INCLUDE_ALARM_TUNE_LUCKY_STAR                 // Fun Fun Dayo (Lucky Star)
// #define INCLUDE_ALARM_TUNE_SKYPE                      // Skype Call Music
// #define INCLUDE_ALARM_TUNE_CRAZY_FROG                 // Axel F (Crazy Frog)
// #define INCLUDE_ALARM_TUNE_CARAMELLDANSEN             // Caramelldansen (Caramella Girls)
// #define INCLUDE_ALARM_TUNE_IPHONE                     // Opening (iPhone Ringtone)
// #define INCLUDE_ALARM_TUNE_BUTTERFLY                  // Butterfly (Smile)
// #define INCLUDE_ALARM_TUNE_RUNNING_IN_THE_90S         // Running in the 90's (Max Coveri)
// #define INCLUDE_ALARM_TUNE_FORD                       // Ford Chime
// #define INCLUDE_ALARM_TUNE_PARTY_NIGHT                // Party Night (Di Gi Charat)
// #define INCLUDE_ALARM_TUNE_SIX_TRILLION_YEARS         // A Tale of Six Trillion Years And A Night (Ia)
// #define INCLUDE_ALARM_TUNE_GOD_KNOWS                  // God Knows... (The Melancholy of Haruhi Suzumiya)
// #define INCLUDE_ALARM_TUNE_LEVAN_POLKKA               // Levan Polkka (Eino Kettunen)
// #define INCLUDE_ALARM_TUNE_CBAT                       // Cbat (Hudson Mohawke)
// #define INCLUDE_ALARM_TUNE_PACMAN                     // Intermission (Pac-Man)
// #define INCLUDE_ALARM_TUNE_SAMSUNG                    // Morning Flower (Samsung Ringtone)
// #define INCLUDE_ALARM_TUNE_CHEETAHMEN                 // Theme (The Cheetahmen)
// #define INCLUDE_ALARM_TUNE_CRAB_RAVE                  // Crab Rave (Noisestorm)
// #define INCLUDE_ALARM_TUNE_SONIC_1_INVINCIBILITY      // Invincibility (Sonic The Hedgehog)
// #define INCLUDE_ALARM_TUNE_BRAIN_POWER                // Brain Power (NOMA)
// #define INCLUDE_ALARM_TUNE_MAGICAL_SOUND_SHOWER       // Magical Sound Shower (Out Run)
// #define INCLUDE_ALARM_TUNE_SAILOR_MOON                // Moonlight Densetsu (Sailor Moon)
// #define INCLUDE_ALARM_TUNE_DRILLCHU                   // Superstrong Cavity Reconstructor DrillChu (Camellia ft. Nanahira)
// #define INCLUDE_ALARM_TUNE_UNWELCOME_SCHOOL           // Unwelcome School (Blue Archive)
// #define INCLUDE_ALARM_TUNE_YOU                        // You (Higurashi When They Cry)
// #define INCLUDE_ALARM_TUNE_CALAMARI_INKANTATION       // Calamari Inkantation (Splatoon)
// #define INCLUDE_ALARM_TUNE_INNOCENT_STARTER           // Innocent Starter (Magical Girl Lyrical Nanoha)
// #define INCLUDE_ALARM_TUNE_SMACK_MY                   // Smack My Bitch Up (The Prodigy)
// #define INCLUDE_ALARM_TUNE_LONELY_ROLLING_STAR        // Lonely Rolling Star (Katamari Damacy)
// #define INCLUDE_ALARM_TUNE_VOICES                     // Voices (Macross Plus)
// #define INCLUDE_ALARM_TUNE_DK_ISLAND_SWING            // DK Island Swing (Donkey Kong Country)*/
// #define INCLUDE_ALARM_TUNE_LEASE                      // Lease (My Merry May)

#endif
