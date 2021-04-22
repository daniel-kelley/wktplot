/*
   wktrand.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <geos_c.h>
#include "wkt.h"

#define BACKSTOP 10 /* number of counts to limit uniqueness/distance tests */

struct info {
    int verbose;
    int unique;
    int backstop;
    double interval; /* quantized; 0.0 = none */
    double distance; /* implies unique */
    double distance_squared; /* distance squared */
    double width;
    double height;
    unsigned long points;
    unsigned long count;
    GEOSGeometry *geom;
    GEOSGeometry **point;
    struct wkt wkt;
};

static double w_value(struct info *info, double max)
{
    double value;


    /* Constrain point to be away from edges by given distance */
    max -= (info->distance * 2.0);
    value = (drand48() * max) + info->distance;

    if (info->interval != 0.0) {
        value = floor(value / info->interval) * info->interval;
    }

    return value;
}

/* See if we have this point already. */
static int w_has(struct info *info, GEOSGeometry *geom)
{
    double x0;
    double y0;
    double x1;
    double y1;
    double dx;
    double dy;
    double distance_squared;
    unsigned long i;

    assert(GEOSGeomGetX_r(info->wkt.handle, geom, &x0) != 0);
    assert(GEOSGeomGetY_r(info->wkt.handle, geom, &y0) != 0);
    for (i=0; i<info->points; i++) {
        assert(GEOSGeomGetX_r(info->wkt.handle, info->point[i], &x1) != 0);
        assert(GEOSGeomGetY_r(info->wkt.handle, info->point[i], &y1) != 0);
        /* These values are known to be quantized so f.p. equality
         * should be OK. If testing uniqueness and not quantized, well
         * shame!
         */
        if (x0 == x1 && y0 == y1) {
            return 1;
        }

        /*
         * Explictly check distance. Note: Could not get strtree to
         * work on points: "Can't compute envelope of item in
         * BoundablePair". Work on squared distance because there's no
         * good reason to take the square root.
         */
        dx = x1 - x0;
        dy = y1 - y0;
        distance_squared = (dx*dx) + (dy*dy);
        if (distance_squared < info->distance_squared) {
            return 1;
        }
    }

    return 0;
}

static int w_add(struct info *info, GEOSGeometry *geom)
{
    if (info->unique && w_has(info, geom)) {
        return 0;
    }

    info->point[info->points] = geom;
    info->points++;

    return 1;
}

static int w_random(struct info *info)
{
    double x;
    double y;
    GEOSGeometry *geom;
    long backstop = info->backstop * info->count;

    info->point = calloc(info->count, sizeof(*info->point));
    assert(info->point != NULL);

    /* Save distance squared for distance measurements. */
    info->distance_squared = info->distance * info->distance;

    /* Bad combinations of uniqueness and interval could conspire to
     * make it impossible to generate enough random points, so use a
     * backstop to limit the number of iterations.
     */
    while (info->points < info->count && backstop-- > 0) {
        x = w_value(info, info->width);
        y = w_value(info, info->height);
        geom = GEOSGeom_createPointFromXY_r(info->wkt.handle, x, y);
        assert(geom != NULL);
        if (!w_add(info, geom)) {
            GEOSGeom_destroy_r(info->wkt.handle, geom);
        }
    }

    info->geom = GEOSGeom_createCollection_r(
        info->wkt.handle,
        GEOS_GEOMETRYCOLLECTION,
        info->point,
        info->points);

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
        "%s -xf -yf -sn -nn -qf -rf -Nn [-bBuvh] <output>\n",
        prog);
    fprintf(stderr,"  -h        Print this message\n");
    fprintf(stderr,"  -v        Verbose messages\n");
    fprintf(stderr,"  -u        Ensure points are unique\n");
    fprintf(stderr,"  -b        WKB output\n");
    fprintf(stderr,"  -B        WKB HEX output\n");
    fprintf(stderr,"  -x n      Output width\n");
    fprintf(stderr,"  -y n      Output height\n");
    fprintf(stderr,"  -s f      Random seed\n");
    fprintf(stderr,"  -n n      Number of points\n");
    fprintf(stderr,"  -q f      Quantization interval\n");
    fprintf(stderr,"  -r f      Minimum distance\n");
    fprintf(stderr,"  -N n      Uniqueness/distance retries\n");
}

int main(int argc, char *argv[])
{
    int err = 1;
    int c;
    int num_arg;
    struct info info;

    memset(&info, 0, sizeof(info));
    info.wkt.writer = WKT_IO_ASCII;
    info.backstop = BACKSTOP;

    while ((c = getopt(argc, argv, "x:y:s:n:q:r:N:Bbuvh")) != EOF) {
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
        case 'q':
            info.interval = strtod(optarg,0);
            /* implies -r, -u */
            info.distance = info.interval;
            info.unique = 1;
            break;
        case 'r':
            info.distance = strtod(optarg,0);
            /* implies -u */
            info.unique = 1;
            break;
        case 'N':
            info.backstop = strtol(optarg,0,0);
            break;
        case 'u':
            info.unique = 1;
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
