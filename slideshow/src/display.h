// SPDX-License-Identifier: MIT
// Output display.
// Copyright (C) 2025 Artem Senichev <artemsen@gmail.com>

#pragma once

#include <stddef.h>
#include <stdint.h>

/** Display context. */
typedef struct display display;

/** Display frame buffer. */
struct buffer {
    uint8_t* data;   ///< Buffer data
    size_t width;    ///< Buffer width (pixels)
    size_t height;   ///< Buffer height (pixels)
    size_t stride;   ///< Stride size in bytes
    size_t size;     ///< Total size of the buffer (bytes)
    uint32_t id;     ///< Buffer Id (DRM specific)
    uint32_t handle; ///< Buffer handle (DRM specific)
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
 * Begin drawing.
 * @param display pointer to the display context
 * @return pointer to the current frame buffer
 */
struct buffer* display_draw(display* display);

/**
 * Flush frame buffer to display.
 * @param display pointer to the display context
 */
void display_commit(display* display);
