/*
   wkthull.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <geos_c.h>
#include "wkt.h"

struct info {
    int verbose;
    GEOSGeometry *geom;
    struct wkt wkt;
};

static int w_hull(struct info *info)
{
    info->geom = GEOSConvexHull_r(
        info->wkt.handle,
        info->wkt.geom);

    assert(info->geom != NULL);

    return 0;
}

static void w_free(struct info *info)
{
    if (info->geom) {
        GEOSGeom_destroy_r(info->wkt.handle, info->geom);
    }
}

static int w_op(struct info *info, const char *input, const char *output)
{
    int err;

    err = wkt_open(&info->wkt);
    if (!err) {
        err = wkt_read(&info->wkt, input);
    }
    if (!err) {
        err = w_hull(info);
    }
    if (!err) {
        err = wkt_write(&info->wkt, output, info->geom);
        w_free(info);
    }
    wkt_close(&info->wkt);

    return err;
}

static void usage(const char *prog)
{
    fprintf(
        stderr,
        "%s [-bBvh] <input> [<output>]\n",
        prog);
    fprintf(stderr,"  -h        Print this message\n");
    fprintf(stderr,"  -v        Verbose messages\n");
    fprintf(stderr,"  -b        WKB IO\n");
    fprintf(stderr,"  -B        WKB HEX IO\n");
}

int main(int argc, char *argv[])
{
    int err = 1;
    int c;
    int num_arg;
    struct info info;

    memset(&info, 0, sizeof(info));
    info.wkt.reader = WKT_IO_ASCII;
    info.wkt.writer = WKT_IO_ASCII;

    while ((c = getopt(argc, argv, "Bbevh")) != EOF) {
        switch (c) {
        case 'v':
            info.verbose = 1;
            break;
        case 'b':
            info.wkt.reader = WKT_IO_BINARY;
            info.wkt.writer = WKT_IO_BINARY;
            break;
        case 'B':
            info.wkt.reader = WKT_IO_HEX;
            info.wkt.writer = WKT_IO_HEX;
            break;
        case 'h':
            usage(argv[0]);
            return(EXIT_SUCCESS);
            break;
        default:
            break;
        }
    }

    num_arg = argc - optind;

    if (num_arg == 1 || num_arg == 2) {
        char *input = argv[optind];
        char *output = (num_arg == 2) ? argv[optind+1] : NULL;
        err = w_op(&info, input, output);
    } else {
        usage(argv[0]);
        err = 1;
    }

    return err;
}
