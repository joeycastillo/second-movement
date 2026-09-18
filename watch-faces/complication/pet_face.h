/*
 * MIT License
 *
 * Copyright (c) 2026 Daniel Schrempf
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

#pragma once

#include "movement.h"

/*
 * PET
 *
 * This was created for, and submitted as, a final project for CS50x
 * (https://cs50.harvard.edu/x/)
 * (https://www.youtube.com/watch?v=8aZwXIta17M&t=3s)
 *
 * Thanks for taking care of PET for me!
 *
 * They're a little funny looking, but if you just turn them sideways, you'll
 * see their colon eyes. Not their actual colon haha, but I guess they're just a
 * face, so maybe it is a colon colon? Whatever, if they do use their colon
 * colon, make sure to sweep up after them.
 *
 * In terms of food, they need 4 bits for breakfast, lunch, and dinner. You can
 * spread this out if you like, but don't give them any more than that. Again,
 * I'm not sure where they're putting all this food, but I know for sure that 5
 * comes right back out.
 *
 * If you give them a hug, expect a kiss in return, or at least I think that's a
 * kiss? Either way, I'm pretty sure they like it. They also love to play, but
 * can get motion sick really quickly; probably because they're so small? I'm
 * still figuring this guy out too.
 *
 * Unfortunately, it does seem like the little guy holds a mean grudge. If
 * you're not on top of the whole routine, they definitely remember. There's no
 * real coming back once they start getting upset. Good news is I'm pretty sure
 * they're immortal, or I guess it'd be like semi-mortal? If you find a
 * tombstone, they're easily persuaded to resurrect themselves. Which seems to
 * have the added bonus of forgetting all your negligence. Like, I mean, the
 * universal "you". I've killed them a lot too. Nothing to worry about.
 *
 * They go down real easy, their bedtime is like clockwork or something. But
 * they really don't like being disturbed. They are super cute in their little
 * nightcap though, so feel free to check in. Just don't try to play with them
 * or feed them or anything.
 *
 * That's about it! Thanks again for boarding them on your watch!
 *
 * GAMEPLAY --------------------------------------------------------------------
 *
 *      HAPPY 0  CONFUSED 12  UPSET 18  ANGRY 24  DEAD 36
 *
 *      FOOD            | -2            | Max 4 per sitting / 12 per day
 *      PLAY            | -3            | Max 1 per 2h      / 8  per day
 *      HUG             | -1            | Max 4 per day, no cooldown
 *
 *      AGING           | +1            | Every waking hour
 *      DIRTY ROOM      | +1            | Every waking hour if unswept
 *      HUNGRY          | +6            | Every day if unfed
 *      BARF            | +2, buff back | Over-play, +5; over-feed, +10 at most
 *      WOKEN UP        | +2            | If bothered at night
 *
 *      MORNING         | 06:00 - 10:59 | 5h
 *      AFTERNOON       | 11:00 - 15:59 | 5h
 *      EVENING         | 16:00 - 20:59 | 5h
 *      NIGHT           | 21:00 - 05:59 | 9h
 *
 * INPUTS ----------------------------------------------------------------------
 *
 *      MODE            | Short         | Change watch face (system)
 *      LIGHT           | Short         | Feed once
 *      LIGHT           | Extra Long    | Animation showcase on or off
 *      LIGHT + ALARM   | Together      | Hug
 *      ALARM           | Short         | Sweep waste
 *      ALARM           | Long          | Resurrect, only while dead
 *      ALARM           | Extra Long    | Play
 *      ACCELEROMETER   | Tap 3x        | Play
 *
 * BEHAVIOURS ------------------------------------------------------------------
 *
 *      STATUS          | Happy, Confused, Upset, Angry, Dead
 *      POSITIVE        | Eat, Kiss, Play small, Play big
 *      NEUTRAL         | Snore, Wake, Poo, Resurrect
 *      NEGATIVE        | Barf
 *
 * REGIONS ---------------------------------------------------------------------
 *
 *      COLON           | Eyes
 *      HRS MIN SEC(10) | Character design
 *      SEC(1)          | Waste
 *      DAY(1)          | Character sounds
 *      DAY(10)         | Status + or -
 *      DATE(1) BELL    | Food plate and dinner bell
 *      SIGNAL          | Visual cue for sfx
 */

// -----------
// GAME TUNING
// -----------

#define PET_SECONDS_PER_TIC (60 * 60)
#define PET_TIC_DEAD 36
#define PET_TIC_CONFUSED 12
#define PET_TIC_UPSET 18
#define PET_TIC_ANGRY 24
#define PET_MISSED_FEED_SECONDS (24 * 60 * 60)
#define PET_MISSED_FEED_TICS 6
#define PET_BUFF_PLAY 3
#define PET_BUFF_HUG 1
#define PET_HUG_CAP 4
#define PET_BUFF_EAT 2
#define PET_DEBUFF_DISTURB 2
#define PET_DEBUFF_BARF 2
#define PET_POO_DELAY_SECONDS (12 * 60 * 60)
#define PET_POO_SECONDS_PER_TIC (PET_SECONDS_PER_TIC)

// -------
// FEEDING
// -------

#define PET_FOOD_MAX 4
#define PET_FEED_SETTLE_SECONDS 3
#define PET_FEED_PIP_SECONDS 1

// ----
// PLAY
// ----

#define PET_SHAKE_TAPS 3
#define PET_SHAKE_WINDOW_SECONDS 2
#define PET_SHAKE_GAP_TICKS 1
#define PET_SHAKE_THRESHOLD 8
#define PET_PLAY_WINDOW_SECONDS 5
#define PET_PLAY_STAGE_BARF 3
#define PET_PLAY_COOLDOWN_SECONDS (2 * 60 * 60)

// ---
// DAY
// ---

#define PET_HOUR_WAKE 6
#define PET_HOUR_AFTERNOON 11
#define PET_HOUR_SLEEP 21
#define PET_AWAKE_SECONDS_PER_DAY ((PET_HOUR_SLEEP - PET_HOUR_WAKE) * 60 * 60)

// -----
// MEALS
// -----

#define PET_FEED_SEGMENTS 3
#define PET_FEED_SEGMENT_HOURS ((PET_HOUR_SLEEP - PET_HOUR_WAKE) / PET_FEED_SEGMENTS)
#define PET_FEED_SEGMENT_CAP 4
#if (PET_HOUR_SLEEP - PET_HOUR_WAKE) % PET_FEED_SEGMENTS
#error "the waking day must divide evenly into PET_FEED_SEGMENTS sittings"
#endif

#if (PET_FEED_SEGMENT_CAP * PET_BUFF_EAT + PET_HUG_CAP * PET_BUFF_HUG + PET_BUFF_PLAY) \
    != (PET_HOUR_SLEEP - PET_HOUR_WAKE)
#error "one complete visit must be worth one day of passive decay"
#endif
#define PET_BARF_SETTLE_SECONDS (30 * 60)

// -----
// SLEEP
// -----

#define PET_NIGHT_AWAKE_SECONDS 30
#define PET_SNORE_BREATH_CYCLE 3
#define PET_SNORE_AUDIBLE_BREATHS 2

// ----------
// ANIMATIONS
// ----------

#define PET_ANIM_HZ 8
#define PET_FLASH_TICKS (PET_ANIM_HZ)
#define PET_BELL_TICKS (PET_ANIM_HZ / 2)
#define PET_BUFF_POSITION 0
#define PET_FOOD_POSITION 3
#define SEG_A (1 << 0)
#define SEG_B (1 << 1)
#define SEG_C (1 << 2)
#define SEG_D (1 << 3)
#define SEG_E (1 << 4)
#define SEG_F (1 << 5)
#define SEG_G (1 << 6)
#define SEG_H (1 << 7)
#define SEG_NONE 0
#define PET_FRAME_COLON (1 << 0)
#define PET_FRAME_SIGNAL (1 << 1)
#define PET_FRAME_BELL (1 << 2)

// --------
// SHOWCASE
// --------

#define PET_SHOWCASE 1
#define PET_SHOWCASE_PLAYS 2

typedef struct
{
    uint8_t seg[10];
    uint8_t flags;
    uint8_t hold;
} pet_frame_t;

typedef enum
{
    PET_LAYER_CHARACTER = 0,
    PET_LAYER_STATUS,
    PET_LAYER_COUNT
} pet_layer_id_t;

typedef struct
{
    uint8_t seg[10];
    uint8_t flags;
} pet_layer_def_t;

typedef struct
{
    uint8_t sound;
    uint8_t position;
    uint8_t mask;
    bool on_clear;
    bool once;
} pet_cue_t;

typedef struct
{
    const pet_frame_t *frames;
    uint8_t count;
    bool loop;
    pet_layer_id_t layer;
    const pet_cue_t *cues;
    uint8_t cue_count;
} pet_anim_t;

#define PET_FRAMES(t) (t), (uint8_t)(sizeof(t) / sizeof((t)[0]))
#define PET_NO_FRAMES NULL, 0
#define PET_CUES(t) (t), (uint8_t)(sizeof(t) / sizeof((t)[0]))
#define PET_NO_CUES NULL, 0

typedef struct
{
    uint8_t anim;
    uint8_t idle;
    uint8_t frame;
    uint8_t hold_left;
    uint8_t cues_fired;
} pet_layer_t;

typedef enum
{
    PET_ANIM_NONE = 0,
    PET_ANIM_HAPPY,
    PET_ANIM_CONFUSED,
    PET_ANIM_UPSET,
    PET_ANIM_ANGRY,
    PET_ANIM_DEAD,
    PET_ANIM_RESURRECT,
    PET_ANIM_POO,
    PET_ANIM_PLAY_SMALL,
    PET_ANIM_PLAY_BIG,
    PET_ANIM_BARF,
    PET_ANIM_EAT,
    PET_ANIM_KISS,
    PET_ANIM_SNORE,
    PET_ANIM_WAKE,
    PET_ANIM_PILE,
    PET_ANIM_PUDDLE,
    PET_ANIM_COUNT
} pet_anim_id_t;

typedef enum
{
    PET_SOUND_SNORE_IN = 0,
    PET_SOUND_SNORE_OUT,
    PET_SOUND_KISS,
    PET_SOUND_BARF_UHOH,
    PET_SOUND_BARF_SLIDE,
    PET_SOUND_EAT_GULP,
    PET_SOUND_EAT_CHEW,
    PET_SOUND_POO,
    PET_SOUND_PLAY_SMALL,
    PET_SOUND_PLAY_BIG,
    PET_SOUND_WAKE,
    PET_SOUND_RESURRECT_FADE,
    PET_SOUND_RESURRECT_RISE,
    PET_SOUND_FEED,
    PET_SOUND_SWEEP,
    PET_SOUND_GRUMBLE,
    PET_SOUND_DEATH,
    PET_SOUND_MOOD_UP,
    PET_SOUND_MOOD_DOWN,
    PET_SOUND_COUNT
} pet_sound_id_t;

typedef enum
{
    PET_MOOD_HAPPY = 0,
    PET_MOOD_CONFUSED,
    PET_MOOD_UPSET,
    PET_MOOD_ANGRY,
    PET_MOOD_DEAD
} pet_mood_t;

typedef enum
{
    PET_DAYPART_MORNING = 0,
    PET_DAYPART_AFTERNOON,
    PET_DAYPART_NIGHT
} pet_daypart_t;

typedef enum
{
    PET_SCENE_IDLE = 0,
    PET_SCENE_ASLEEP,
    PET_SCENE_NIGHT_AWAKE,
    PET_SCENE_FEEDING,
    PET_SCENE_PLAYING,
    PET_SCENE_DEAD
} pet_scene_t;

#define PET_QUEUE_LEN 4

typedef struct
{
    uint8_t tics;
    bool has_poo;
    bool has_barf;
    uint32_t last_update_ts;
    uint32_t last_fed_ts;
    uint32_t poo_due_ts;
    uint32_t poo_since_ts;
    uint16_t awake_residual;
    uint16_t poo_residual;
    uint32_t last_play_buff_ts;
    uint32_t barf_until_ts;
    uint16_t barf_sitting;
    uint8_t hugs_today;
    uint8_t hug_day;
    uint8_t pips_this_seg;
    uint8_t seg_buff_tics;
    uint16_t fed_sitting;
    uint8_t woke_day;
    pet_scene_t scene;
    uint8_t shown_mood;
    pet_layer_t layer[PET_LAYER_COUNT];
    pet_anim_id_t queue[PET_QUEUE_LEN];
    uint8_t queue_len;
    uint8_t buff_ticks;
    uint8_t debuff_ticks;
    uint8_t bell_ticks;
    uint8_t signal_ticks;
    uint8_t food_queue;
    uint16_t feed_ticks;
    uint16_t play_ticks;
    uint8_t play_stage;
    bool play_buffed;
    uint8_t shake_taps;
    uint8_t shake_ticks;
    uint8_t shake_gap;
    bool poo_pending;
    uint16_t night_awake_ticks;
    uint8_t breath;
    bool tap_enabled;
    bool light_down;
    bool alarm_down;
    bool chord_hugged;
    bool alarm_resurrected;
    uint8_t shadow[10];
    uint8_t shadow_flags;
    bool shadow_stale;
    bool showcase_on;
    uint8_t showcase_step;
    uint8_t showcase_plays;
} pet_state_t;

void pet_face_setup(uint8_t watch_face_index, void **context_ptr);
void pet_face_activate(void *context);
bool pet_face_loop(movement_event_t event, void *context);
void pet_face_resign(void *context);

#define pet_face ((const watch_face_t){ \
    pet_face_setup,                     \
    pet_face_activate,                  \
    pet_face_loop,                      \
    pet_face_resign,                    \
    NULL,                               \
})
