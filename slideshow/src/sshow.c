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

/** Stop flag. */
static bool stop_slideshow;

/**
 * Draw image 1:1.
 * @param img image to draw
 * @return buf destibation buffer
 */
static void copy_image(const struct image* img, struct buffer* buf)
{
    const size_t stride = img->width * sizeof(xrgb_t);
    if (stride == buf->stride) {
        memcpy(buf->data, img->data, img->height * stride);
    } else {
        for (size_t y = 0; y < buf->height; ++y) {
            xrgb_t* img_ptr = &img->data[y * img->width];
            uint32_t* buf_ptr = &buf->data[y * buf->stride / sizeof(uint32_t)];
            memcpy(buf_ptr, img_ptr, stride);
        }
    }
}

/**
 * Draw scaled image.
 * @param img image to draw
 * @return buf destibation buffer
 */
static void scale_image(const struct image* img, struct buffer* buf)
{
    const float scale_w = (float)buf->width / img->width;
    const float scale_h = (float)buf->height / img->height;
    const float scale = scale_w < scale_h ? scale_w : scale_h;

    const size_t dst_w = (float)img->width * scale;
    const size_t dst_h = (float)img->height * scale;
    const size_t dst_x1 = buf->width / 2 - dst_w / 2;
    const size_t dst_y1 = buf->height / 2 - dst_h / 2;
    const size_t dst_x2 = dst_x1 + dst_w;
    const size_t dst_y2 = dst_y1 + dst_h;

    // clear background
    memset(buf->data, 0, dst_y1 * buf->stride);
    memset(buf->data + dst_y2 * buf->stride / sizeof(uint32_t), 0,
           (buf->height - dst_y2) * buf->stride);

    for (size_t y = dst_y1; y < dst_y2; ++y) {
        const size_t img_y = (float)(y - dst_y1) / scale;
        const xrgb_t* img_line = &img->data[img_y * img->width];
        uint32_t* buf_line = &buf->data[y * buf->stride / sizeof(uint32_t)];

        // clear background
        memset(buf_line, 0, dst_x1 * sizeof(uint32_t));
        memset(buf_line + dst_x2, 0, (buf->width - dst_x2) * sizeof(uint32_t));

        for (size_t x = dst_x1; x < dst_x2; ++x) {
            const size_t img_x = (float)(x - dst_x1) / scale;
            buf_line[x] = img_line[img_x];
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
    struct buffer* buf = display_buffer(display);

    // set signal handler
    struct sigaction sigact;
    sigact.sa_handler = on_signal;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(SIGINT, &sigact, NULL);
    sigaction(SIGTERM, &sigact, NULL);

    while (!stop_slideshow) {
        struct image* img = next_image(list);
        if (!img) {
            break;
        }
        if (img->width == buf->width && img->height == buf->height) {
            copy_image(img, buf);
        } else {
            scale_image(img, buf);
        }
        free(img);

        display_flush(display);

        sleep(5);
    }

    return stop_slideshow;
}
