/*
   wkt_stash.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include "wkt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

int wkt_stash(const char *file, const char *data, size_t len)
{
    int err = 1;
    int fd;
    ssize_t written;
    const char *name = file;
    int opened = 0;

    if (file == NULL || !strcmp(file, "-")) {
        fd = STDOUT_FILENO;
        name = "<stdout>";
    } else {
        fd = open(file, O_WRONLY|O_CREAT, 0666);
        opened = 1;
    }

    if (fd < 0) {
        fprintf(stderr, "%s: %s", name, strerror(errno));
    } else {
        written = write(fd, data, len);
        if (written < 0) {
            fprintf(stderr, "%s: %s", name, strerror(errno));
        } else if ((size_t)written != len) {
            fprintf(stderr, "%s: truncated", name);
        } else {
            err = 0;
        }
    }

    if (fd > 0 && opened) {
        close(fd); /* fd not needed any more */
    }

    return err;
}
