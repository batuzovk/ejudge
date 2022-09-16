/* -*- mode: c; c-basic-offset: 4 -*- */

/* Copyright (C) 2022 Alexander Chernov <cher@ejudge.ru> */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ejudge/config.h"
#include "ejudge/version.h"

#include "ejudge/ejudge_cfg.h"
#include "ejudge/contests.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>

static const char *program_name;

static void die(const char *format, ...)
  __attribute__((noreturn, format(printf, 1, 2)));
static void die(const char *format, ...)
{
  va_list args;
  char buf[1024];

  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  fprintf(stderr, "%s: %s\n", program_name, buf);
  exit(1);
}

static int
sort_func(const void *p1, const void *p2)
{
    int v1 = *(const int*) p1;
    int v2 = *(const int*) p2;
    if (v1 < v2) return -1;
    return v1 > v2;
}

static void
process_contest(
        struct ejudge_cfg *ejudge_config,
        int contest_id,
        const unsigned char *from_plugin,
        const unsigned char *to_plugin,
        int remove_mode,
        int force_from_mode)
{
}

int
main(int argc, char *argv[])
{
    int all_mode = 0;
    int remove_mode = 0;
    int force_from_mode = 0;
    const char *from_plugin = NULL;
    const char *to_plugin = NULL;
    int *cnts_ids = NULL;
    int cnts_idu = 0;
    int cnts_ida = 0;
    unsigned char ejudge_xml_path[PATH_MAX] = {};
    unsigned char ejudge_contests_dir[PATH_MAX] = {};
    struct ejudge_cfg *ejudge_config = NULL;
    const int *ej_cnts_ids = NULL;
    int ej_cnts_count = 0;

    {
        char *p = strrchr(argv[0], '/');
        if (!p) {
            program_name = argv[0];
        } else {
            program_name = p + 1;
        }
    }

    int argi = 1;
    while (argi < argc) {
        if (!strcmp(argv[argi], "--all")) {
            all_mode = 1;
            ++argi;
        } else if (!strcmp(argv[argi], "--remove-old")) {
            remove_mode = 1;
            ++argi;
        } else if (!strcmp(argv[argi], "--force-from")) {
            force_from_mode = 1;
            ++argi;
        } else if (!strcmp(argv[argi], "--from")) {
            if (argi + 1 >= argc) die("argument expected for --from");
            from_plugin = argv[argi + 1];
            argi += 2;
        } else if (!strcmp(argv[argi], "--to")) {
            if (argi + 1 >= argc) die("argument expected for --to");
            to_plugin = argv[argi + 1];
            argi += 2;
        } else {
            break;
        }
    }

    if (all_mode) {
        if (argi != argc) die("contests not allowed after --all");
    } else {
        for (; argi < argc; ++argi) {
            char *eptr = NULL;
            errno = 0;
            long id1 = strtol(argv[argi], &eptr, 10);
            if (errno || eptr == argv[argi] || (int) id1 != id1 || id1 <= 0)
                die("invalid contest id '%s'", argv[argi]);
            if (!*eptr) {
                if (cnts_idu == cnts_ida) {
                    if (!(cnts_ida *= 2)) cnts_ida = 16;
                    cnts_ids = realloc(cnts_ids, cnts_ida * sizeof(cnts_ids[0]));
                }
                cnts_ids[cnts_idu++] = id1;
            } else if (*eptr == '-') {
                const char *s = eptr + 1;
                errno = 0;
                long id2 = strtol(s, &eptr, 10);
                if (errno || eptr == s || *eptr || (int) id2 != id2 || id2 <= 0)
                    die("invalid contest id '%s'", argv[argi]);
                if (id2 < id1) die("invalid contest range");
                if (id2 - id1 > 1000000) die("invalid contest range");
                for (long cc = id1; cc <= id2; ++cc) {
                    if (cnts_idu == cnts_ida) {
                        if (!(cnts_ida *= 2)) cnts_ida = 16;
                        cnts_ids = realloc(cnts_ids, cnts_ida * sizeof(cnts_ids[0]));
                    }
                    cnts_ids[cnts_idu++] = cc;
                }
            } else {
                die("invalid contest id '%s'", argv[argi]);
            }
        }
        qsort(cnts_ids, cnts_idu, sizeof(cnts_ids[0]), sort_func);
        if (cnts_idu > 1) {
            int dst = 1;
            for (int src = 1; src < cnts_idu; ++src) {
                if (cnts_ids[src - 1] != cnts_ids[src]) {
                    cnts_ids[dst++] = cnts_ids[src];
                }
            }
            cnts_idu = dst;
        }
    }

    if(cnts_idu <= 0 && !all_mode) {
        // nothing
        return 0;
    }

    if (!from_plugin || !*from_plugin) {
        from_plugin = "auto";
    }
    if (!to_plugin || !*to_plugin) {
        to_plugin = "mysql";
    }

    if (!ejudge_xml_path[0]) {
#if defined EJUDGE_XML_PATH
        if (snprintf(ejudge_xml_path, sizeof(ejudge_xml_path), "%s", EJUDGE_XML_PATH) >= sizeof(ejudge_xml_path)) die("path too long: %s", EJUDGE_XML_PATH);
#endif
    }
    if (!ejudge_xml_path[0])
        die("path to ejudge.xml is not set");

    ejudge_config = ejudge_cfg_parse(ejudge_xml_path, 1);
    if (!ejudge_config) die("failed to parse %s", ejudge_xml_path);

    if (!ejudge_config->contests_dir)
        die("contests config directory not specified");
    if (snprintf(ejudge_contests_dir, sizeof(ejudge_contests_dir), "%s", ejudge_config->contests_dir) >= sizeof(ejudge_contests_dir)) die("path too long: %s", ejudge_config->contests_dir);

    contests_set_directory(ejudge_contests_dir);
    ej_cnts_count = contests_get_list(&ej_cnts_ids);
    if (ej_cnts_count <= 0) {
        return 0;
    }

    if (all_mode) {
        cnts_idu = cnts_ida = ej_cnts_count;
        cnts_ids = calloc(cnts_idu, sizeof(cnts_ids[0]));
        memcpy(cnts_ids, ej_cnts_ids, cnts_idu * sizeof(cnts_ids[0]));
    }

    int i1 = 0, i2 = 0;
    while (i1 < cnts_idu && i2 < ej_cnts_count) {
        if (cnts_ids[i1] < ej_cnts_ids[i2]) {
            ++i1;
        } else if (cnts_ids[i1] > ej_cnts_ids[i2]) {
            ++i2;
        } else {
            process_contest(ejudge_config, cnts_ids[i1],
                            from_plugin, to_plugin, remove_mode, force_from_mode);
            ++i1; ++i2;
        }
    }
}