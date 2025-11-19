/*
 * MIT License
 *
 * Copyright (c) 2025
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

#ifndef ADDRESS_BOOK_FACE_H_
#define ADDRESS_BOOK_FACE_H_

/*
 * Address book face
 *
 * This face displays contacts from an array of VCard data.
 * To use your own data see vcard_data in address_book_face.c
 *
 * Light Button (Short Press): Show next 4 characters of current name
 * Light Button (Long Press): Show previous name
 * Alarm Button (Short Press): Go back to beginning of current name
 * Alarm Button (Long Press): Show next name
 * Mode Button (Long Press): Display contact attributes (TEL, EMAIL, etc.)
 */

#include "movement.h"

#define ADDRESS_BOOK_MAX_NAME_LENGTH 30
#define ADDRESS_BOOK_MAX_ATTR_LENGTH 40

typedef enum {
    DISPLAY_MODE_NAME,
    DISPLAY_MODE_ATTRIBUTES
} display_mode_t;

typedef struct {
    // As we iterate through the different contacts this is points to the current one
    uint8_t current_contact_index;
    // When we display a name or an attribute value the user can page through the value. This is the offset.
    uint8_t display_offset;
    // Points to the selected attribute (ex TEL or EMAIL)
    uint8_t attr_index;
    // Are we showing the contact name or one of the attributes?
    display_mode_t display_mode;
    char current_name[ADDRESS_BOOK_MAX_NAME_LENGTH];
    char current_attr[ADDRESS_BOOK_MAX_ATTR_LENGTH];
} address_book_state_t;

void address_book_face_setup(uint8_t watch_face_index, void ** context_ptr);
void address_book_face_activate(void *context);
bool address_book_face_loop(movement_event_t event, void *context);
void address_book_face_resign(void *context);

static void attribute_cleanup_copy(const char* attr_name, char* attr_buffer, size_t attr_buffer_size, const char* temp_buffer);

#define address_book_face ((const watch_face_t){ \
    address_book_face_setup, \
    address_book_face_activate, \
    address_book_face_loop, \
    address_book_face_resign, \
    NULL, \
})

#endif // ADDRESS_BOOK_FACE_H_