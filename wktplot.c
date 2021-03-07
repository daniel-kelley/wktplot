/*
   wktplot.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <plot.h>
#include <geos_c.h>
#include "wkt.h"

struct info {
    int verbose;
    double width;
    const char *pen;
    const char *format;
    plPlotter *plotter;
    plPlotterParams *param;
    struct wkt wkt;
};

struct line_info {
    struct info *info;
    double prev[2];
};

static int w_point_iterator(
    struct wkt *wkt,
    unsigned i,
    unsigned n,
    double x,
    double y,
    void *user_data)
{
    struct info *info = user_data;

    (void)wkt;
    if (info->verbose) {
        fprintf(stderr, "Point %u/%u [%g,%g]\n", i, n, x, y);
    }
    pl_fpoint_r(info->plotter, x, y);

    return 0;
}

static int w_line_iterator(
    struct wkt *wkt,
    unsigned i,
    unsigned n,
    double x,
    double y,
    void *user_data)
{
    struct line_info *line_info = user_data;
    struct info *info = line_info->info;
    double *prev = line_info->prev;

    (void)wkt;
    if (info->verbose) {
        fprintf(stderr, "Line %u/%u [%g,%g]\n", i, n, x, y);
    }

    if (i > 0) {
        pl_fline_r(info->plotter, prev[0], prev[1], x, y);
    }

    prev[0] = x;
    prev[1] = y;

    return 0;
}

static int w_handle_point(struct info *info, const GEOSGeometry *geom)
{
    return wkt_iterate_coord_seq(&info->wkt, geom, w_point_iterator, info);
}

static int w_handle_polygon(struct info *info, const GEOSGeometry *geom)
{
    int err = 1;
    int n;
    int i;
    const GEOSGeometry *g;
    struct line_info line_info;

    line_info.info = info;
    do {
        g = GEOSGetExteriorRing_r(info->wkt.handle, geom);
        if (g==NULL) {
            break;
        }

        err = wkt_iterate_coord_seq(&info->wkt, g, w_line_iterator, &line_info);
        if (err) {
            break;
        }

        n = GEOSGetNumInteriorRings_r(info->wkt.handle, geom);

        for (i=0; i<n; i++) {
            g = GEOSGetInteriorRingN_r(info->wkt.handle, geom, i);
            if (g==NULL) {
                break;
            }

            err = wkt_iterate_coord_seq(&info->wkt, g, w_line_iterator, &line_info);
            if (err) {
                break;
            }
        }

    } while (0);

    return err;
}

static int w_handle_linestring(struct info *info, const GEOSGeometry *geom)
{
    int err;
    struct line_info line_info;

    line_info.info = info;

    err = wkt_iterate_coord_seq(&info->wkt, geom, w_line_iterator, &line_info);

    return err;
}

static int w_handle(struct wkt *wkt,
                    const GEOSGeometry *geom,
                    const char *gtype,
                    void *user_data)
{
    int err = 1;
    struct info *info = user_data;

    (void)wkt;
    if (!strcmp(gtype, "Point")) {
        err = w_handle_point(info, geom);
    } else if (!strcmp(gtype, "Polygon")) {
        err = w_handle_polygon(info, geom);
    } else if (!strcmp(gtype, "LineString")) {
        err = w_handle_linestring(info, geom);
    } else {
        fprintf(stderr,"Missing handler for %s\n", gtype);
    }

    return err;
}

static int w_setup(struct info *info)
{
    int err = 1;
    int rc;

    double xmin = 0.0;
    double xmax = 0.0;
    double ymin = 1000.0;
    double ymax = 1000.0;

    do {
        rc = wkt_bounds(&info->wkt, &xmin, &xmax, &ymin, &ymax);
        if (!rc) {
            break;
        }

        if (info->verbose) {
            fprintf(stderr, "bounds: [%g,%g,%g,%g]\n",xmin,xmax,ymin,ymax);
        }

        err = 0;

        /* setup plotter */
        pl_fspace_r(info->plotter, xmin, ymin, xmax, ymax);
        pl_flinewidth_r(info->plotter, info->width);
        pl_pencolorname_r(info->plotter, info->pen ? info->pen : "black");
        pl_erase_r(info->plotter);
    } while (0);

    return err;
}

static int w_interpret(struct info *info)
{
    int err = 1;

    /* interpret */
    err = wkt_iterate(&info->wkt, w_handle, info);

    return err;
}

static int w_scan(struct info *info)
{
    int err;

    err = w_setup(info);
    if (!err) {
        err = w_interpret(info);
    }

    return err;
}

static int w_plot(struct info *info)
{
    int err = 1;

    /* Create */
    info->plotter = pl_newpl_r(
        info->format,
        stdin,
        stdout,
        stderr,
        info->param);

    do {
        if (info->plotter == NULL) {
            fprintf(stderr, "Could not create plotter %s\n", info->format);
            break;
        }

        /* Open plotter */
        if (pl_openpl_r(info->plotter) < 0) {
            fprintf(stderr, "Could not create plotter %s\n", info->format);
            break;
        }

        err = w_scan(info);

        /* close plotter */
        if (pl_closepl_r(info->plotter) < 0) {
            fprintf(stderr, "Could not close plotter %s\n", info->format);
            break;
        }

        /* delete plotter */
        if (pl_deletepl_r(info->plotter) < 0) {
            fprintf(stderr, "Could not delete plotter %s\n", info->format);
            break;
        }

    } while (0);

    return err;
}

static int set_option(struct info *info, const char *arg)
{
    int err = 1;
    char *opt;
    char *val;

    opt = strdup(arg);
    assert(opt != NULL);
    val = strchr(opt, '=');
    if (val) {
        *val++ = 0;
        /* set a Plotter parameter */
        err = pl_setplparam(info->param, opt, val);
    } else {
        fprintf(stderr, "Missing '=' in %s\n",arg);
    }

    free(opt);

    return err;
}

static void usage(const char *prog)
{
    fprintf(stderr,"%s -T format -O opt [-bvwh] <input>\n", prog);
    fprintf(stderr,"  -h        Print this message\n");
    fprintf(stderr,"  -w n      Line width\n");
    fprintf(stderr,"  -T format Output format\n");
    fprintf(stderr,"  -b        Input is WKB\n");
    fprintf(stderr,"  -v        Verbose\n");
    fprintf(stderr,"  -O opt=v  Output option=v\n");
}

int main(int argc, char *argv[])
{
    int err = 1;
    int c;
    struct info info;

    memset(&info, 0, sizeof(info));
    info.param = pl_newplparams();
    info.wkt.reader = WKT_READER_ASCII;
    info.width = 0.1;
    info.format = "svg";
    assert(info.param != NULL);

    while ((c = getopt(argc, argv, "w:T:O:bBvh")) != EOF) {
        switch (c) {
        case 'w':
            info.width = strtod(optarg,0);
            break;
        case 'T':
            info.format = optarg;
            break;
        case 'O':
            set_option(&info, optarg);
            break;
        case 'b':
            info.wkt.reader = WKT_READER_BINARY;
            break;
        case 'B':
            info.wkt.reader = WKT_READER_HEX;
            break;
        case 'v':
            info.verbose = 1;
            break;
        case 'h':
            usage(argv[0]);
            err = EXIT_SUCCESS;
            break;
        default:
            break;
        }
    }

    if (optind < argc) {
        err = wkt_open(&info.wkt, argv[optind]);
        if (!err) {
            err = w_plot(&info);
        }
        wkt_close(&info.wkt);
    }

    return err;
}
