// SPDX-License-Identifier: MIT
// Image loader.
// Copyright (C) 2025 Artem Senichev <artemsen@gmail.com>

#pragma once

#include <stddef.h>
#include <stdint.h>

typedef uint32_t xrgb_t;

/** Image data. */
struct image {
    size_t width;
    size_t height;
    xrgb_t* data;
};

/**
 * Load JPEG image.
 * @param path path to the image for loading
 * @return image pixmap or NULL on errors, the caller should free the buffer
 */
struct image* image_load(const char* path);
