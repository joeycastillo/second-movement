#ifndef TEST_MOVEMENT_H
#define TEST_MOVEMENT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// Minimal Second Movement boundary; game logic is the real wumpus_face.c.
typedef enum {
    EVENT_NONE, EVENT_ACTIVATE, EVENT_TICK, EVENT_LOW_ENERGY_UPDATE,
    EVENT_BACKGROUND_TASK, EVENT_TIMEOUT,
    EVENT_LIGHT_BUTTON_DOWN, EVENT_LIGHT_BUTTON_UP, EVENT_LIGHT_LONG_PRESS,
    EVENT_LIGHT_LONG_UP, EVENT_LIGHT_REALLY_LONG_PRESS,
    EVENT_MODE_BUTTON_DOWN, EVENT_MODE_BUTTON_UP, EVENT_MODE_LONG_PRESS,
    EVENT_MODE_LONG_UP, EVENT_MODE_REALLY_LONG_PRESS,
    EVENT_ALARM_BUTTON_DOWN, EVENT_ALARM_BUTTON_UP, EVENT_ALARM_LONG_PRESS,
    EVENT_ALARM_LONG_UP, EVENT_ALARM_REALLY_LONG_PRESS,
} movement_event_type_t;
typedef struct { uint8_t event_type; uint8_t subsecond; } movement_event_t;
typedef enum {
    WATCH_POSITION_FULL, WATCH_POSITION_TOP, WATCH_POSITION_TOP_LEFT,
    WATCH_POSITION_TOP_RIGHT, WATCH_POSITION_BOTTOM, WATCH_POSITION_HOURS,
    WATCH_POSITION_MINUTES, WATCH_POSITION_SECONDS,
} watch_position_t;
typedef enum {
    WATCH_LCD_TYPE_UNKNOWN = 0,
    WATCH_LCD_TYPE_CLASSIC = 0xA9,
    WATCH_LCD_TYPE_CUSTOM = 0x56,
} watch_lcd_type_t;
typedef struct {
    void (*setup)(uint8_t, void **);
    void (*activate)(void *);
    bool (*loop)(movement_event_t, void *);
    void (*resign)(void *);
    void *advise;
} watch_face_t;

uint32_t arc4random_uniform(uint32_t upper_bound);
void watch_display_text(watch_position_t location, const char *string);
void watch_display_text_with_fallback(watch_position_t location, const char *string, const char *fallback);
void watch_clear_pixel(uint8_t com, uint8_t seg);
void watch_set_colon(void);
void watch_clear_colon(void);
watch_lcd_type_t watch_get_lcd_type(void);
void movement_force_led_on(uint8_t red, uint8_t green, uint8_t blue);
void movement_force_led_off(void);
void movement_request_tick_frequency(uint8_t frequency);
void movement_move_to_face(uint8_t index);
bool movement_default_loop_handler(movement_event_t event);

#endif
