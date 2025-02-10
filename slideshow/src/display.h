// SPDX-License-Identifier: MIT
// Output display.
// Copyright (C) 2025 Artem Senichev <artemsen@gmail.com>

#pragma once

#include <stddef.h>
#include <stdint.h>

/** Display context. */
typedef struct display display;

/** Display buffer. */
struct buffer {
    size_t width;
    size_t height;
    size_t stride;
    size_t size;
    uint32_t* data;
};

/**
 * Initialize display.
 * @return display context or NULL if error
 */
display* display_init(void);

/**
 * Destroy display context.
 * @param display pointer to the display context
 */
void display_free(display* display);

/**
 * Get output desplay buffer.
 * @param display pointer to the display context
 * @return pointer to the buffer description
 */
struct buffer* display_buffer(display* display);

/**
 * Flush buffer to display.
 * @param display pointer to the display context
 */
void display_flush(display* display);
