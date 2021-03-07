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
#include <sys/types.h>
#include <fcntl.h>

int wkt_stash(const char *file, const char *data, size_t len)
{
    int err = 1;
    int fd;
    ssize_t written;

    fd = open(file, O_WRONLY|O_CREAT, 0666);

    if (fd < 0) {
        fprintf(stderr, "%s: %s", file, strerror(errno));
    } else {
        written = write(fd, data, len);
        if (written < 0) {
            fprintf(stderr, "%s: %s", file, strerror(errno));
        } else if ((size_t)written != len) {
            fprintf(stderr, "%s: truncated", file);
        } else {
            err = 0;
        }
    }

    if (fd >= 0 ) {
        close(fd); /* fd not needed any more */
    }

    return err;
}
