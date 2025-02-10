// SPDX-License-Identifier: MIT
// Slide show.
// Copyright (C) 2025 Artem Senichev <artemsen@gmail.com>

#pragma once

#include "display.h"
#include "imglist.h"

#include <stdbool.h>

/**
 * Start slide show.
 * @param list pointer to the image list context
 * @param display pointer to the display context
 * @return true if slide show exit by normally by signal or false on errors
 */
bool slide_show(imglist* list, display* display);
