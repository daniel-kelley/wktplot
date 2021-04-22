/*
   wkt_snag.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include "wkt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

int wkt_snag(struct wkt *wkt, const char *file)
{
    int err = 1;
    int fd;
    struct stat stat;

    do {
        fd = open(file, O_RDONLY);
        /* open */
        if (fd < 0) {
            fprintf(stderr, "%s: %s\n", file, strerror(errno));
            break;
        }

        /* fstat */
        err = fstat(fd, &stat);

        if (err) {
            fprintf(stderr, "%s: %s\n", file, strerror(errno));
            break;
        }

        wkt->input_len = stat.st_size;

        /* mmap */
        wkt->input = mmap(
            NULL,
            wkt->input_len,
            PROT_READ,
            MAP_PRIVATE,
            fd,
            0);

        if (!wkt->input) {
            fprintf(stderr, "%s: %s\n", file, strerror(errno));
            err = -1;
            break;
        }


    } while (0);

    if (fd >= 0 ) {
        close(fd); /* fd not needed any more */
    }

    return err;
}
