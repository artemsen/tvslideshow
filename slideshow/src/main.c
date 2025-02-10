// SPDX-License-Identifier: MIT
// Program entry point.
// Copyright (C) 2025 Artem Senichev <artemsen@gmail.com>

#include "display.h"
#include "imglist.h"
#include "sshow.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/** Command line arguments. */
struct cmdarg {
    const char short_opt; ///< Short option character
    const char* long_opt; ///< Long option name
    const char* format;   ///< Format description
    const char* help;     ///< Help string
};

// clang-format off
static const struct cmdarg arguments[] = {
    { 'v', "version",    NULL,    "print version info and exit" },
    { 'h', "help",       NULL,    "print this help and exit" },
};
// clang-format on

/**
 * Print usage info.
 */
static void print_help(void)
{
    puts("Usage: sshow [OPTION]... DIR");
    for (size_t i = 0; i < sizeof(arguments) / sizeof(arguments[0]); ++i) {
        const struct cmdarg* arg = &arguments[i];
        char lopt[32];
        if (arg->format) {
            snprintf(lopt, sizeof(lopt), "%s=%s", arg->long_opt, arg->format);
        } else {
            strncpy(lopt, arg->long_opt, sizeof(lopt) - 1);
        }
        printf("  -%c, --%-14s %s\n", arg->short_opt, lopt, arg->help);
    }
}

/**
 * Print version info.
 */
static void print_version(void)
{
    puts("slideshow version " APP_VERSION ".");
    puts("https://github.com/artemsen/???");
}

/**
 * Parse command line arguments.
 * @param argc number of arguments to parse
 * @param argv arguments array
 * @return index of the first non option argument
 */
static int parse_cmdargs(int argc, char* argv[] /* , struct config* cfg */)
{
    struct option options[1 + sizeof(arguments) / sizeof(arguments[0])];
    char short_opts[sizeof(arguments) / sizeof(arguments[0]) * 2];
    char* short_opts_ptr = short_opts;
    int opt;

    // compose array of option structs
    for (size_t i = 0; i < sizeof(arguments) / sizeof(arguments[0]); ++i) {
        const struct cmdarg* arg = &arguments[i];
        options[i].name = arg->long_opt;
        options[i].has_arg = arg->format ? required_argument : no_argument;
        options[i].flag = NULL;
        options[i].val = arg->short_opt;
        // compose short options string
        *short_opts_ptr++ = arg->short_opt;
        if (arg->format) {
            *short_opts_ptr++ = ':';
        }
    }
    // add terminations
    *short_opts_ptr = 0;
    memset(&options[sizeof(arguments) / sizeof(arguments[0])], 0,
           sizeof(struct option));

    // parse arguments
    while ((opt = getopt_long(argc, argv, short_opts, options, NULL)) != -1) {
        switch (opt) {
            case 'v':
                print_version();
                exit(EXIT_SUCCESS);
                break;
            case 'h':
                print_help();
                exit(EXIT_SUCCESS);
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }

    return optind;
}

/**
 * Application entry point.
 */
int main(int argc, char* argv[])
{
    int rc = EXIT_FAILURE;
    imglist* list = NULL;
    display* display = NULL;
    struct timespec ts;
    // struct config* cfg;
    int argn;

    argn = parse_cmdargs(argc, argv /* , cfg */);

    // init rng
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand(ts.tv_nsec);

    list = imglist_init(argn >= argc ? NULL : argv[argn]);
    if (!list) {
        goto done;
    }

    display = display_init();
    if (!display) {
        goto done;
    }

    rc = slide_show(list, display) ? EXIT_SUCCESS : EXIT_FAILURE;

done:
    display_free(display);
    imglist_free(list);
    return rc;
}
