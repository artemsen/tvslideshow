// SPDX-License-Identifier: MIT
// Output display.
// Copyright (C) 2025 Artem Senichev <artemsen@gmail.com>

#include "display.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#pragma GCC diagnostic pop

/** Display context. */
struct display {
    int fd;                   ///< DRM file handle
    uint32_t conn_id;         ///< Connector Id
    uint32_t crtc_id;         ///< CRTC Id
    drmModeCrtcPtr crtc_save; ///< Previous CRTC mode
    struct buffer* cfb;       ///< Currently displayed frame buffers
    struct buffer fb[2];      ///< Frame buffers
};

/**
 * Free frame buffer.
 * @param display pointer to the display context
 * @param fb pointer to the frame buffer description
 */
static void free_fb(display* display, struct buffer* fb)
{
    if (fb->id) {
        drmModeRmFB(display->fd, fb->id);
    }
    if (fb->handle) {
        struct drm_mode_destroy_dumb destroy = {
            .handle = fb->handle,
        };
        drmIoctl(display->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
    }
    if (fb->data) {
        munmap(fb->data, fb->size);
    }
}

/**
 * Create frame buffer.
 * @param display pointer to the display context
 * @param fb pointer to the frame buffer description
 * @param width,height size of frame buffer in pixels
 * @return true if frame buffer was created successfully
 */
static bool create_fb(display* display, struct buffer* fb, size_t width,
                      size_t height)
{
    struct drm_mode_create_dumb dumb_create = { 0 };
    struct drm_mode_map_dumb dumb_map = { 0 };
    uint32_t handles[4] = { 0 };
    uint32_t strides[4] = { 0 };
    uint32_t offsets[4] = { 0 };

    // create dumb
    dumb_create.width = width;
    dumb_create.height = height;
    dumb_create.bpp = 32; // xrgb
    if (drmIoctl(display->fd, DRM_IOCTL_MODE_CREATE_DUMB, &dumb_create) < 0) {
        fprintf(stderr, "Unable to create dumb: [%d] %s\n", errno,
                strerror(errno));
        goto fail;
    }

    fb->handle = dumb_create.handle;
    fb->stride = dumb_create.pitch;
    fb->size = dumb_create.size;
    fb->width = width;
    fb->height = height;

    // create frambuffer
    handles[0] = dumb_create.handle;
    strides[0] = dumb_create.pitch;
    if (drmModeAddFB2(display->fd, dumb_create.width, dumb_create.height,
                      DRM_FORMAT_XRGB8888, handles, strides, offsets, &fb->id,
                      0) < 0) {
        fprintf(stderr, "Unable to add frambuffer: [%d] %s\n", errno,
                strerror(errno));
        goto fail;
    }

    // map frambuffer
    dumb_map.handle = dumb_create.handle;
    if (drmIoctl(display->fd, DRM_IOCTL_MODE_MAP_DUMB, &dumb_map) < 0) {
        fprintf(stderr, "Unable to map frambuffer: [%d] %s\n", errno,
                strerror(errno));
        goto fail;
    }

    // create memory map
    fb->data = mmap(0, dumb_create.size, PROT_READ | PROT_WRITE, MAP_SHARED,
                    display->fd, dumb_map.offset);
    if (fb->data == MAP_FAILED) {
        fprintf(stderr, "Unable to create mmap: [%d] %s\n", errno,
                strerror(errno));
        fb->data = NULL;
        goto fail;
    }

    return true;

fail:
    free_fb(display, fb);
    memset(fb, 0, sizeof(*fb));
    return false;
}

/**
 * Get first suitable connector.
 * @param display pointer to the display context
 * @param mode output mode
 * @return true if connector found
 */
static bool get_connector(display* display, drmModeModeInfo* mode)
{
    drmModeRes* res = drmModeGetResources(display->fd);
    if (!res) {
        fprintf(stderr, "Unable to get DRM modes: [%d] %s\n", errno,
                strerror(errno));
        return false;
    }

    for (int i = 0; i < res->count_connectors; ++i) {
        drmModeConnector* conn;

        conn = drmModeGetConnector(display->fd, res->connectors[i]);
        if (!conn) {
            continue;
        }
        if (conn->connection != DRM_MODE_CONNECTED || conn->count_modes == 0) {
            drmModeFreeConnector(conn);
            continue;
        }

        for (int j = 0; j < conn->count_encoders; ++j) {
            drmModeEncoder* enc;
            enc = drmModeGetEncoder(display->fd, conn->encoders[j]);
            if (!enc) {
                continue;
            }
            // just get first available
            *mode = conn->modes[0];
            display->crtc_id = enc->crtc_id;
            display->conn_id = conn->connector_id;
            drmModeFreeEncoder(enc);
            drmModeFreeConnector(conn);
            drmModeFreeResources(res);
            return true;
        }
        drmModeFreeConnector(conn);
    }

    drmModeFreeResources(res);

    fprintf(stderr, "Connector not found\n");
    return false;
}

display* display_init(void)
{
    drmModeModeInfo mode;
    display* display;

    display = calloc(1, sizeof(*display));
    if (!display) {
        fprintf(stderr, "Not enough memory\n");
        return NULL;
    }

    // open DRM, try first 2 cards
    display->fd = -1;
    for (size_t i = 0; display->fd == -1 && i < 2; ++i) {
        int fd;
        uint64_t cap;
        char path[16];
        snprintf(path, sizeof(path), "/dev/dri/card%ld", i);

        // open drm
        fd = open(path, O_RDWR);
        if (fd == -1) {
            continue;
        }

        // check capability
        if (drmGetCap(fd, DRM_CAP_DUMB_BUFFER, &cap) < 0 || !cap) {
            close(fd);
            continue;
        }

        display->fd = fd;
    }
    if (display->fd == -1) {
        fprintf(stderr, "No compatible DRM cards found\n");
        free(display);
        return NULL;
    }

    if (!get_connector(display, &mode)) {
        display_free(display);
        return NULL;
    }

    if (!create_fb(display, &display->fb[0], mode.hdisplay, mode.vdisplay) ||
        !create_fb(display, &display->fb[1], mode.hdisplay, mode.vdisplay)) {
        display_free(display);
        return NULL;
    }

    // save the previous CRTC configuration
    display->crtc_save = drmModeGetCrtc(display->fd, display->crtc_id);
    // perform the modeset
    display->cfb = &display->fb[0];
    if (drmModeSetCrtc(display->fd, display->crtc_id, display->cfb->id, 0, 0,
                       &display->conn_id, 1, &mode) < 0) {
        fprintf(stderr, "Unable to set CRTC mode: [%d] %s\n", errno,
                strerror(errno));
        display_free(display);
        return NULL;
    }

    return display;
}

void display_free(display* display)
{
    if (display->crtc_save) {
        drmModeCrtcPtr crtc = display->crtc_save;
        drmModeSetCrtc(display->fd, crtc->crtc_id, crtc->buffer_id, crtc->x,
                       crtc->y, &display->conn_id, 1, &crtc->mode);
        drmModeFreeCrtc(crtc);
    }

    free_fb(display, &display->fb[0]);
    free_fb(display, &display->fb[1]);

    if (display->fd != -1) {
        close(display->fd);
    }

    free(display);
}

struct buffer* display_draw(display* display)
{
    return display->cfb;
}

void display_commit(display* display)
{
    // swap buffers
    if (drmModePageFlip(display->fd, display->crtc_id, display->cfb->id,
                        DRM_MODE_PAGE_FLIP_EVENT, NULL) < 0) {
        fprintf(stderr, "Unable to flip page: [%d] %s\n", errno,
                strerror(errno));
    }

    if (display->cfb == &display->fb[0]) {
        display->cfb = &display->fb[1];
    } else {
        display->cfb = &display->fb[0];
    }
}
