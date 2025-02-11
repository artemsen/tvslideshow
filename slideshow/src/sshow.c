// SPDX-License-Identifier: MIT
// Slide show.
// Copyright (C) 2025 Artem Senichev <artemsen@gmail.com>

#include "sshow.h"

#include "image.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef NDEBUG
#define PHOTO_DELAY 5
#else
#define PHOTO_DELAY 1
#endif

/** Stop flag. */
static bool stop_slideshow;

/**
 * Draw image 1:1.
 * @param img image to draw
 * @return fb destination frame buffer
 */
static void copy_image(const struct image* img, struct buffer* fb)
{
    const size_t stride = img->width * sizeof(xrgb_t);
    if (stride == fb->stride) {
        memcpy(fb->data, img->data, img->height * stride);
    } else {
        for (size_t y = 0; y < fb->height; ++y) {
            xrgb_t* img_ptr = &img->data[y * img->width];
            uint8_t* buf_ptr = &fb->data[y * fb->stride];
            memcpy(buf_ptr, img_ptr, stride);
        }
    }
}

/**
 * Draw scaled image.
 * @param img image to draw
 * @return fb destination frame buffer
 */
static void scale_image(const struct image* img, struct buffer* fb)
{
    const float scale_w = (float)fb->width / img->width;
    const float scale_h = (float)fb->height / img->height;
    const float scale = scale_w < scale_h ? scale_w : scale_h;

    const size_t dst_w = (float)img->width * scale;
    const size_t dst_h = (float)img->height * scale;
    const size_t dst_x1 = fb->width / 2 - dst_w / 2;
    const size_t dst_y1 = fb->height / 2 - dst_h / 2;
    const size_t dst_x2 = dst_x1 + dst_w;
    const size_t dst_y2 = dst_y1 + dst_h;

    // clear background
    memset(fb->data, 0, dst_y1 * fb->stride);
    memset(fb->data + dst_y2 * fb->stride, 0,
           (fb->height - dst_y2) * fb->stride);

    for (size_t y = dst_y1; y < dst_y2; ++y) {
        const size_t img_y = (float)(y - dst_y1) / scale;
        const xrgb_t* img_line = &img->data[img_y * img->width];
        uint8_t* buf_line = &fb->data[y * fb->stride];

        // clear background
        memset(buf_line, 0, dst_x1 * sizeof(uint32_t));
        memset(buf_line + dst_x2, 0, (fb->width - dst_x2) * sizeof(uint32_t));

        for (size_t x = dst_x1; x < dst_x2; ++x) {
            const size_t img_x = (float)(x - dst_x1) / scale;
            *(uint32_t*)&buf_line[x * sizeof(uint32_t)] = img_line[img_x];
        }
    }
}

/**
 * Load next image from the list.
 * @param list pointer to the image list context
 * @return image instance or NULL on errors
 */
static struct image* next_image(imglist* list)
{
    struct image* img = NULL;
    const char* path = imglist_next(list);

    while (path) {
        img = image_load(path);
        if (img) {
            return img;
        }
        path = imglist_skip(list);
    }

    fprintf(stderr, "No more images in the list\n");
    return NULL;
}

/** POSIX signal handler. */
static void on_signal(__attribute__((unused)) int signum)
{
    stop_slideshow = true;
}

bool slide_show(imglist* list, display* display)
{
    // set signal handler
    struct sigaction sigact;
    sigact.sa_handler = on_signal;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(SIGINT, &sigact, NULL);
    sigaction(SIGTERM, &sigact, NULL);

    while (!stop_slideshow) {
        struct buffer* fb;
        struct image* img;

        img = next_image(list);
        if (!img) {
            break;
        }

        fb = display_draw(display);
        if (img->width == fb->width && img->height == fb->height) {
            copy_image(img, fb);
        } else {
            scale_image(img, fb);
        }
        display_commit(display);

        free(img);

        sleep(PHOTO_DELAY);
    }

    return stop_slideshow;
}
