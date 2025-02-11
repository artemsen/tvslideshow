// SPDX-License-Identifier: MIT
// Image loader.
// Copyright (C) 2025 Artem Senichev <artemsen@gmail.com>

#include "image.h"

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

// depends on stdio.h, uses FILE but doesn't include the header
#include <jpeglib.h>

struct jpg_error_manager {
    struct jpeg_error_mgr mgr;
    jmp_buf setjmp;
};

/** JPEG error handler. */
static void jpg_error_exit(j_common_ptr jpg)
{
    struct jpg_error_manager* err = (struct jpg_error_manager*)jpg->err;
    char msg[JMSG_LENGTH_MAX] = { 0 };
    (*(jpg->err->format_message))(jpg, msg);
    longjmp(err->setjmp, 1);
}

struct image* image_load(const char* path)
{
    struct image* img = NULL;
    FILE* file;
    struct jpeg_decompress_struct jpg;
    struct jpg_error_manager err;

    // open image file
    file = fopen(path, "rb");
    if (!file) {
        return NULL;
    }

    // setup error handling
    jpg.err = jpeg_std_error(&err.mgr);
    err.mgr.error_exit = jpg_error_exit;
    if (setjmp(err.setjmp)) {
        free(img);
        if (file) {
            fclose(file);
        }
        jpeg_destroy_decompress(&jpg);
        return NULL;
    }

    // initialize jpeg decoder
    jpeg_create_decompress(&jpg);
    jpeg_stdio_src(&jpg, file);
    jpeg_read_header(&jpg, TRUE);
    jpg.out_color_space = JCS_RGB;
    jpeg_start_decompress(&jpg);

    // allocate image data buffer
    img = malloc(sizeof(*img) +
                 jpg.output_width * jpg.output_height * sizeof(xrgb_t));
    if (!img) {
        jpeg_destroy_decompress(&jpg);
        fclose(file);
        return NULL;
    }
    img->data = (xrgb_t*)((uint8_t*)img + sizeof(*img));
    img->width = jpg.output_width;
    img->height = jpg.output_height;

    // decode image
    while (jpg.output_scanline < jpg.output_height) {
        uint8_t* line =
            (uint8_t*)&img->data[jpg.output_scanline * jpg.output_width];
        jpeg_read_scanlines(&jpg, &line, 1);

        // convert to 32-bit xrgb
        if (jpg.out_color_components == 1) {
            uint32_t* pixel = (uint32_t*)line;
            for (int x = jpg.output_width - 1; x >= 0; --x) {
                const xrgb_t c = *(line + x);
                pixel[x] = ((xrgb_t)0xff << 24) | (c << 16) | (c << 8) | c;
            }
        } else if (jpg.out_color_components == 3) {
            uint32_t* pixel = (uint32_t*)line;
            for (int x = jpg.output_width - 1; x >= 0; --x) {
                const uint8_t* src = line + x * 3;
                const xrgb_t r = src[0];
                const xrgb_t g = src[1];
                const xrgb_t b = src[2];
                pixel[x] = ((xrgb_t)0xff << 24) | (r << 16) | (g << 8) | b;
            }
        }
    }

    jpeg_finish_decompress(&jpg);
    jpeg_destroy_decompress(&jpg);
    fclose(file);

    return img;
}
