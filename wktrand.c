/*
   wktrand.c

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
    double width;
    double height;
    unsigned long count;
    GEOSGeometry *geom;
    GEOSGeometry **point;
    struct wkt wkt;
};

static int w_random(struct info *info)
{
    unsigned long i;
    double x;
    double y;
    GEOSGeometry *geom;

    info->point = calloc(info->count, sizeof(*info->point));
    assert(info->point != NULL);

    for (i=0; i < info->count; i++) {
        x = drand48() * info->width;
        y = drand48() * info->height;
        geom = GEOSGeom_createPointFromXY_r(info->wkt.handle, x, y);
        assert(geom != NULL);
        info->point[i] = geom;
    }

    info->geom = GEOSGeom_createCollection_r(
        info->wkt.handle,
        GEOS_GEOMETRYCOLLECTION,
        info->point,
        info->count);

    assert(info->geom != NULL);

    return 0;
}

static void w_free(struct info *info)
{
    if (info->geom) {
        GEOSGeom_destroy_r(info->wkt.handle, info->geom);
    }

    if (info->point) {
        /* Note that points themselves do not have to be destroyed. */
        free(info->point);
    }
}

static int w_op(struct info *info, const char *file)
{
    int err;

    err = wkt_open(&info->wkt);
    if (!err) {
        err = w_random(info);
    }
    if (!err) {
        err = wkt_write(&info->wkt, file, info->geom);
        w_free(info);
    }
    wkt_close(&info->wkt);

    return err;
}

static void usage(const char *prog)
{
    fprintf(
        stderr,
        "%s -xf -yf -sn -nn [-bBvh] <output>\n",
        prog);
    fprintf(stderr,"  -h        Print this message\n");
    fprintf(stderr,"  -v        Verbose messages\n");
    fprintf(stderr,"  -b        WKB output\n");
    fprintf(stderr,"  -B        WKB HEX output\n");
    fprintf(stderr,"  -x n      Output width\n");
    fprintf(stderr,"  -y n      Output height\n");
    fprintf(stderr,"  -s f      Random seed\n");
    fprintf(stderr,"  -n n      Number of points\n");
}

int main(int argc, char *argv[])
{
    int err = 1;
    int c;
    int num_arg;
    struct info info;

    memset(&info, 0, sizeof(info));
    info.wkt.writer = WKT_IO_ASCII;

    while ((c = getopt(argc, argv, "x:y:s:n:Bbvh")) != EOF) {
        switch (c) {
        case 'x':
            info.width = strtod(optarg,0);
            break;
        case 'y':
            info.height = strtod(optarg,0);
            break;
        case 'n':
            info.count = strtol(optarg,0,0);
            break;
        case 's':
            srand48(strtol(optarg,0,0));
            break;
        case 'v':
            info.verbose = 1;
            break;
        case 'b':
            info.wkt.writer = WKT_IO_BINARY;
            break;
        case 'B':
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

    if (num_arg <= 1) {
        char *file = num_arg ? argv[optind] : NULL;
        err = w_op(&info, file);
    } else {
        usage(argv[0]);
        err = 1;
    }

    return err;
}
