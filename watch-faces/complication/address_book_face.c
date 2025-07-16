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

#include <stdlib.h>
#include <string.h>
#include "address_book_face.h"
#include "watch.h"

// Address book entries in VCard format. https://en.wikipedia.org/wiki/VCard
static const char* vcard_data[] = {
    "BEGIN:VCARD\nVERSION:3.0\nFN:John Doe\nORG:Example Corp\nTEL:+1-555-123-4567\nEMAIL:john@example.com\nEND:VCARD",
    "BEGIN:VCARD\nVERSION:3.0\nFN:Jane Smith\nORG:Tech Inc\nTEL:+46 480 555 00 00\nEMAIL:jane@tech.com\nEND:VCARD",
    "BEGIN:VCARD\nVERSION:3.0\nFN:Bob Wilson\nORG:Design Studio\nTEL:+1-555-456-7890\nEMAIL;TYPE=INTERNET;TYPE=HOME:bob@gmail.com\nEND:VCARD",
    "BEGIN:VCARD\nVERSION:3.0\nFN:Alice Johnson\nORG:Marketing LLC\nTEL:+1-555-321-0987\nEMAIL:alice@marketing.com\nEND:VCARD",
    "BEGIN:VCARD\nVERSION:3.0\nFN:Charlie Brown\nORG:Consulting\nTEL:+1-555-654-3210\nEMAIL:charlie@consult.com\nEND:VCARD"
};

static const uint8_t vcard_count = sizeof(vcard_data) / sizeof(vcard_data[0]);

// Only display these attributes.
static const char* attribute_names[] = {"TEL", "EMAIL"};
static const uint8_t attribute_count = sizeof(attribute_names) / sizeof(attribute_names[0]);

// The contact name looks like this in the VCard:
// FN:John Doe\n
static void parse_vcard_name(const char* vcard, char* name_buffer, size_t name_buffer_size) {
    const char* fn_start = strstr(vcard, "FN:");
    if (fn_start) {
        fn_start += 3; // Skip "FN:"
        const char* fn_end = strchr(fn_start, '\n');
        if (fn_end) {
            size_t name_len = fn_end - fn_start;
            if (name_len >= name_buffer_size) {
                name_len = name_buffer_size - 1;
            }
            strncpy(name_buffer, fn_start, name_len);
            name_buffer[name_len] = '\0';
        } else {
            // No newline found, copy until end of string or buffer limit
            size_t remaining_len = strlen(fn_start);
            if (remaining_len >= name_buffer_size) {
                remaining_len = name_buffer_size - 1;
            }
            strncpy(name_buffer, fn_start, remaining_len);
            name_buffer[remaining_len] = '\0';
        }
    } else {
        strncpy(name_buffer, "Unknown", name_buffer_size - 1);
        name_buffer[name_buffer_size - 1] = '\0';
    }
}

// Extract an attribute's name and value. Example:
// EMAIL;TYPE=INTERNET;TYPE=HOME:bob@gmail.com
// Would parse out to just EMAIL and bob@gmail.com
static void parse_vcard_attribute(
    const char* vcard,
    const char* attr_name,
    char* attr_buffer,
    size_t attr_buffer_size
) {
    const char* line_start = vcard;
    const char* attr_start = NULL;

    while ((line_start = strstr(line_start, attr_name)) != NULL) {
        // Check if this is at the start of a line (after '\n' or at beginning)
        if (line_start != vcard && *(line_start - 1) != '\n') {
            line_start++;
            continue;
        }

        // Move past the attribute name
        const char* pos = line_start + strlen(attr_name);

        // Skip any parameters (like ;TYPE=INTERNET;TYPE=HOME)
        while (*pos == ';') {
            // Find the end of this parameter
            while (*pos != ':' && *pos != '\n' && *pos != '\0') {
                pos++;
            }
            if (*pos == '\n' || *pos == '\0') {
                break;
            }
        }

        // Check if we found the colon that starts the value
        if (*pos == ':') {
            attr_start = pos + 1;
            break;
        }

        line_start++;
    }

    if (attr_start) {
        const char* attr_end = strchr(attr_start, '\n');
        char temp_buffer[ADDRESS_BOOK_MAX_ATTR_LENGTH];
        memset(temp_buffer, ' ', sizeof(temp_buffer));

        if (attr_end) {
            size_t attr_len = attr_end - attr_start;
            if (attr_len >= sizeof(temp_buffer)) {
                attr_len = sizeof(temp_buffer) - 1;
            }
            strncpy(temp_buffer, attr_start, attr_len);
            temp_buffer[attr_len] = '\0';
        } else {
            // No newline found, copy until end of string or buffer limit
            size_t remaining_len = strlen(attr_start);
            if (remaining_len >= sizeof(temp_buffer)) {
                remaining_len = sizeof(temp_buffer) - 1;
            }
            strncpy(temp_buffer, attr_start, remaining_len);
            temp_buffer[remaining_len] = '\0';
        }

        attribute_cleanup_copy(attr_name, attr_buffer, attr_buffer_size, temp_buffer);
    } else {
        strncpy(attr_buffer, "N/A", attr_buffer_size - 1);
        attr_buffer[attr_buffer_size - 1] = '\0';
    }
}

static bool is_current_attribute_numerical(address_book_state_t *state) {
    // right now only TEL is numerical
    return state->attr_index == 0;
}

// The temp buffer contains the copied attribute value.
// This method copies the temporary contents into the target attr_buffer,
// but also does any attribute type specific cleanup.
//
// For example phone numbers might have any parenthesis, dashes, whitespace or similar characters removed.
static void attribute_cleanup_copy(
    const char* attr_name,
    char* attr_buffer,
    size_t attr_buffer_size,
    const char* temp_buffer
) {
    memset(attr_buffer, ' ', attr_buffer_size);

    // If this is a TEL attribute, remove special characters (but not +) and whitespace
    if (strcmp(attr_name, "TEL") == 0) {
        char* write_ptr = attr_buffer;
        size_t remaining_space = attr_buffer_size - 1;

        for (const char* read_ptr = temp_buffer; *read_ptr && remaining_space > 0; read_ptr++) {
            // Skip special characters and whitespace
            if (*read_ptr == '-' || *read_ptr == ' ' || *read_ptr == '(' || *read_ptr == ')') {
                continue;
            }

            // Copy the character
            *write_ptr++ = *read_ptr;
            remaining_space--;
        }
        *write_ptr = '\0';
    } else {
        // For non-TEL attributes, just copy directly
        strncpy(attr_buffer, temp_buffer, attr_buffer_size - 1);
        attr_buffer[attr_buffer_size - 1] = '\0';
    }
}

static void display_name_mode(address_book_state_t *state) {
    parse_vcard_name(vcard_data[state->current_contact_index], state->current_name, ADDRESS_BOOK_MAX_NAME_LENGTH);

    watch_display_text_with_fallback(WATCH_POSITION_TOP, "ADDR", "AD");

    // Since we are limited in display area we have to show the name in chunks
    char display_name[7];
    memset(display_name, ' ', sizeof(display_name));
    size_t name_len = strlen(state->current_name);

    if (state->display_offset >= name_len) {
        state->display_offset = 0;
    }

    const char* start_ptr = state->current_name + state->display_offset;

    // Skip leading whitespace
    while (start_ptr < state->current_name + name_len && *start_ptr == ' ') {
        start_ptr++;
    }

    size_t remaining_len = strlen(start_ptr);

    // The first 4 characters are bigger on the display
    // So if the name is more than 4 we just add __ at the end to
    // indicate that there is more.
    memset(display_name, ' ', sizeof(display_name));
    if (remaining_len <= 4) {
        strncpy(display_name, start_ptr, remaining_len);
        display_name[6] = '\0';
    } else {
        strncpy(display_name, start_ptr, 4);
        display_name[4] = '_';
        display_name[5] = '_';
        display_name[6] = '\0';
    }

    watch_display_text(WATCH_POSITION_BOTTOM, display_name);
}

static void display_attributes_mode(address_book_state_t *state) {
    const char* attr_name = attribute_names[state->attr_index];
    parse_vcard_attribute(
        vcard_data[state->current_contact_index],
        attr_name,
        state->current_attr,
        ADDRESS_BOOK_MAX_ATTR_LENGTH
    );

    // Name of the attribute (TEL, EMA...) is displayed at the top of the watch
    char attr_name_display[5];
    memset(attr_name_display, ' ', sizeof(attr_name_display));
    strncpy(attr_name_display, attr_name, 3);
    attr_name_display[4] = '\0';

    watch_display_text_with_fallback(WATCH_POSITION_TOP, attr_name_display, attr_name_display);

    // This is the value to be displayed at the bottom
    char display_attr[7];
    memset(display_attr, ' ', sizeof(display_attr));

    size_t attr_len = strlen(state->current_attr);

    if (state->display_offset >= attr_len) {
        state->display_offset = 0;
    }

    const char* start_ptr = state->current_attr + state->display_offset;

    // Skip leading whitespace
    while (start_ptr < state->current_attr + attr_len && *start_ptr == ' ') {
        start_ptr++;
    }

    size_t remaining_len = attr_len - (start_ptr - state->current_attr);

    if (is_current_attribute_numerical(state)) {
        // For numerical attributes (like TEL), use full 6 characters
        if (remaining_len <= 6) {
            strncpy(display_attr, start_ptr, remaining_len);
        } else {
            strncpy(display_attr, start_ptr, 6);
        }
    } else {
        // For non-numerical attributes (like EMAIL), use 4 characters with __ indicator
        if (remaining_len <= 4) {
            strncpy(display_attr, start_ptr, remaining_len);
        } else {
            strncpy(display_attr, start_ptr, 4);
            display_attr[4] = '_';
            display_attr[5] = '_';
        }
    }

    display_attr[6] = '\0';

    watch_display_text(WATCH_POSITION_BOTTOM, display_attr);
}

static void update_display(address_book_state_t *state) {
    if (state->display_mode == DISPLAY_MODE_NAME) {
        display_name_mode(state);
    } else if (state->display_mode == DISPLAY_MODE_ATTRIBUTES) {
        display_attributes_mode(state);
    }
}

void address_book_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(address_book_state_t));
        memset(*context_ptr, 0, sizeof(address_book_state_t));
        address_book_state_t *state = (address_book_state_t *)*context_ptr;
        state->current_contact_index = 0;
    }
}

void address_book_face_activate(void *context) {
    address_book_state_t *state = (address_book_state_t *)context;
    update_display(state);
}

bool address_book_face_loop(movement_event_t event, void *context) {
    address_book_state_t *state = (address_book_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            update_display(state);
            break;

        case EVENT_LIGHT_BUTTON_UP:
            state->display_offset = 0;
            update_display(state);
            break;

        case EVENT_LIGHT_LONG_PRESS:
            // Show the previous contents (name, tel etc)
            if (state->display_mode == DISPLAY_MODE_NAME) {
                if (state->current_contact_index > 0) {
                    state->current_contact_index--;
                } else {
                    state->current_contact_index = vcard_count - 1;
                }
            } else {
                if (state->attr_index > 0) {
                    state->attr_index--;
                } else {
                    state->attr_index = attribute_count - 1;
                }
            }
            state->display_offset = 0;
            update_display(state);
            break;

        case EVENT_ALARM_BUTTON_UP:
            // Show the next few characters in the name or attribute
            if (state->display_mode == DISPLAY_MODE_NAME) {
                state->display_offset += 4;
            } else if (is_current_attribute_numerical(state)) {
                // For numerical attributes, advance by 6 characters
                state->display_offset += 6;
            } else {
                state->display_offset += 4;
            }

            update_display(state);
            break;

        case EVENT_ALARM_LONG_PRESS:
            // Next contact or attribute
            if (state->display_mode == DISPLAY_MODE_NAME) {
                state->current_contact_index = (state->current_contact_index + 1) % vcard_count;
            } else {
                state->attr_index = (state->attr_index + 1) % attribute_count;
            }
            state->display_offset = 0;
            update_display(state);
            break;

        case EVENT_MODE_LONG_PRESS:
            // Show the attributes of this contact
            if (state->display_mode == DISPLAY_MODE_NAME) {
                state->display_mode = DISPLAY_MODE_ATTRIBUTES;
                state->attr_index = 0;
            } else {
                state->display_mode = DISPLAY_MODE_NAME;
            }
            state->display_offset = 0;
            update_display(state);
            break;

        case EVENT_TIMEOUT:
            break;

        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void address_book_face_resign(void *context) {
    (void) context;
}