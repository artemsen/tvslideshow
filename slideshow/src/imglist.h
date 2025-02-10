// SPDX-License-Identifier: MIT
// List of images.
// Copyright (C) 2025 Artem Senichev <artemsen@gmail.com>

#pragma once

/** Image list context. */
typedef struct imglist imglist;

/**
 * Initialize image list.
 * @param dir top directory with images
 * @return image list context or NULL if list is empty
 */
imglist* imglist_init(const char* dir);

/**
 * Destroy image list context.
 * @param list image list context
 */
void imglist_free(imglist* list);

/**
 * Move to the next file.
 * @param list image list context
 * @return path to the next file or NULL if no more files in the list
 */
const char* imglist_next(imglist* list);

/**
 * Skip current image (remove from the list).
 * @param list image list context
 * @return path to the next file or NULL if no more files in the list
 */
const char* imglist_skip(imglist* list);
