// SPDX-License-Identifier: MIT
// List of images.
// Copyright (C) 2025 Artem Senichev <artemsen@gmail.com>

#include "imglist.h"

#include <dirent.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/** Image list. */
struct imglist {
    char** files;   ///< Array with paths to files
    size_t size;    ///< Size of array
    size_t current; ///< Index of the current image
};

/**
 * Add file to the list.
 * @param list image list context
 * @param file path to the file
 */
static void add_file(imglist* list, const char* path)
{
    const size_t len = strlen(path) + 1 /* last null */;

    // relocate array, if needed
    if (list->current + 1 >= list->size) {
        const size_t new_size = list->size + 256;
        char** ptr = realloc(list->files, new_size * sizeof(*list->files));
        if (!ptr) {
            return;
        }
        list->files = ptr;
        list->size = new_size;
    }

    // add new entry
    list->files[list->current] = malloc(len);
    if (list->files[list->current]) {
        memcpy(list->files[list->current], path, len);
        ++list->current;
    }
}

/**
 * Add files from the directory to the list.
 * @param list image list context
 * @param dir full path to the directory
 */
static void add_dir(imglist* list, const char* path)
{
    DIR* dir_handle;
    struct dirent* dir_entry;
    char* full_path = NULL;

    dir_handle = opendir(path);
    if (!dir_handle) {
        return;
    }

    while ((dir_entry = readdir(dir_handle))) {
        const char* name = dir_entry->d_name;
        struct stat st;
        char* tmp;
        size_t len;

        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue; // skip link to self/parent dirs
        }

        // compose full path
        len = strlen(path) + strlen(name) + 1 /* slash */ + 1 /* last null */;
        tmp = realloc(full_path, len);
        if (!tmp) {
            continue;
        }
        strcpy(tmp, path);
        strcat(tmp, "/");
        strcat(tmp, name);
        full_path = tmp;

        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                add_dir(list, full_path);
            } else if (S_ISREG(st.st_mode)) {
                add_file(list, full_path);
            }
        }
    }

    free(full_path);
    closedir(dir_handle);
}

/**
 * Shuffle image list.
 * @param list image list context
 */
static void shuffle(imglist* list)
{
    for (size_t i = 0; i < list->size; ++i) {
        const size_t j = rand() % list->size;
        if (i != j) {
            char* swap = list->files[i];
            list->files[i] = list->files[j];
            list->files[j] = swap;
        }
    }
}

imglist* imglist_init(const char* dir)
{
    imglist* list = calloc(1, sizeof(*list));
    if (!list) {
        fprintf(stderr, "Not enough memory\n");
        return NULL;
    }

    add_dir(list, dir && *dir ? dir : ".");

    if (list->current == 0) { // list is empty
        free(list);
        fprintf(stderr, "Image list is empty\n");
        return NULL;
    }

    // at this point the current field contains number of entries
    list->size = list->current;

    shuffle(list);

    return list;
}

void imglist_free(imglist* list)
{
    if (list) {
        for (size_t i = 0; i < list->size; ++i) {
            free(list->files[i]);
        }
        free(list->files);
        free(list);
    }
}

const char* imglist_next(imglist* list)
{
    size_t index = list->current;

    do {
        if (++index >= list->size) {
            shuffle(list);
            index = 0;
        }
    } while (!list->files[index] && index != list->current);

    list->current = index;
    return list->files[index];
}

const char* imglist_skip(imglist* list)
{
    free(list->files[list->current]);
    list->files[list->current] = NULL;
    return imglist_next(list);
}
