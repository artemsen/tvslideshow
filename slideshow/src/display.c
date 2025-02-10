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
    uint32_t dumb_id;         ///< Dump handle Id
    uint32_t conn_id;         ///< Connector Id
    uint32_t crtc_id;         ///< CRTC Id
    drmModeCrtcPtr crtc_save; ///< Previous CRTC mode
    uint32_t fb_id;           ///< Framebuffer Id
    struct buffer buff;       ///< Framebuffer data
};

static void create_fb(display* display, const drmModeModeInfo* mode)
{
    struct drm_mode_create_dumb dumb_create = { 0 };
    struct drm_mode_map_dumb dumb_map = { 0 };
    uint32_t handles[4] = { 0 };
    uint32_t strides[4] = { 0 };
    uint32_t offsets[4] = { 0 };

    // create dumb
    dumb_create.width = mode->hdisplay;
    dumb_create.height = mode->vdisplay;
    dumb_create.bpp = 32; // xrgb
    if (drmIoctl(display->fd, DRM_IOCTL_MODE_CREATE_DUMB, &dumb_create) < 0) {
        fprintf(stderr, "Unable to create dumb\n");
        return;
    }
    display->dumb_id = dumb_create.handle;

    // create frambuffer
    handles[0] = dumb_create.handle;
    strides[0] = dumb_create.pitch;
    if (drmModeAddFB2(display->fd, dumb_create.width, dumb_create.height,
                      DRM_FORMAT_XRGB8888, handles, strides, offsets,
                      &display->fb_id, 0) < 0) {
        fprintf(stderr, "Unable to add frambuffer\n");
        return;
    }

    // map frambuffer
    dumb_map.handle = dumb_create.handle;
    if (drmIoctl(display->fd, DRM_IOCTL_MODE_MAP_DUMB, &dumb_map) < 0) {
        fprintf(stderr, "Unable to map frambuffer\n");
        return;
    }

    // create memory map
    display->buff.data = mmap(0, dumb_create.size, PROT_READ | PROT_WRITE,
                              MAP_SHARED, display->fd, dumb_map.offset);
    if (display->buff.data == MAP_FAILED) {
        fprintf(stderr, "Unable to create mmap: [%d] %s\n", errno,
                strerror(errno));
        display->buff.data = NULL;
        return;
    }

    display->buff.size = dumb_create.size;
    display->buff.stride = dumb_create.pitch;
    display->buff.width = dumb_create.width;
    display->buff.height = dumb_create.height;
}

static bool init_drm(display* display)
{
    drmModeModeInfo conn_mode;

    drmModeRes* res = drmModeGetResources(display->fd);
    if (!res) {
        fprintf(stderr, "Unable to get DRM modes\n");
        return false;
    }
    for (int i = 0; i < res->count_connectors; ++i) {
        drmModeConnector* conn;
        conn = drmModeGetConnector(display->fd, res->connectors[i]);
        if (conn) {
            if (conn->connection == DRM_MODE_CONNECTED && conn->count_modes) {
                // get first suitable crtc
                drmModeEncoder* enc;
                enc = drmModeGetEncoder(display->fd, conn->encoder_id);
                if (enc) {
                    display->crtc_id = enc->crtc_id;
                    display->conn_id = conn->connector_id;
                    conn_mode = conn->modes[0];
                    create_fb(display, &conn_mode);
                    drmModeFreeEncoder(enc);
                    drmModeFreeConnector(conn);
                    break;
                }
            }
            drmModeFreeConnector(conn);
        }
    }
    drmModeFreeResources(res);

    if (!display->buff.data) {
        return false;
    }

    // save the previous CRTC configuration
    display->crtc_save = drmModeGetCrtc(display->fd, display->crtc_id);
    // perform the modeset
    if (drmModeSetCrtc(display->fd, display->crtc_id, display->fb_id, 0, 0,
                       &display->conn_id, 1, &conn_mode) < 0) {
        fprintf(stderr, "Unable to set CRTC mode\n");
        return false;
    }

    return true;
}

display* display_init(void)
{
    uint64_t cap;
    display* display;

    display = calloc(1, sizeof(*display));
    if (!display) {
        fprintf(stderr, "Not enough memory\n");
        return NULL;
    }

    // open DRM, try first 3 cards
    display->fd = -1;
    for (size_t i = 0; display->fd == -1 && i < 3; ++i) {
        int fd;
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

    if (display->fd == -1 || !init_drm(display)) {
        fprintf(stderr, "No DRM cards found\n");
        display_free(display);
        display = NULL;
    }

    return display;
}

void display_free(display* display)
{
    if (display) {
        if (display->crtc_save) {
            drmModeCrtcPtr crtc = display->crtc_save;
            drmModeSetCrtc(display->fd, crtc->crtc_id, crtc->buffer_id, crtc->x,
                           crtc->y, &display->conn_id, 1, &crtc->mode);
            drmModeFreeCrtc(crtc);
        }

        if (display->fb_id) {
            drmModeRmFB(display->fd, display->fb_id);
        }

        if (display->dumb_id) {
            struct drm_mode_destroy_dumb destroy = {
                .handle = display->dumb_id,
            };
            drmIoctl(display->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
        }

        if (display->buff.data) {
            munmap(display->buff.data, display->buff.size);
        }

        if (display->fd != -1) {
            close(display->fd);
        }

        free(display);
    }
}

struct buffer* display_buffer(display* display)
{
    return &display->buff;
}

void display_flush(display* display)
{
    drmModeClip clip = {
        .x1 = 0,
        .y1 = 0,
        .x2 = display->buff.width,
        .y2 = display->buff.height,
    };
    drmModeDirtyFB(display->fd, display->fb_id, &clip, 1);
}
