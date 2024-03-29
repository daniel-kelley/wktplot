/*
   wktplot.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <plot.h>
#include <igraph/igraph.h>
#include "wkt.h"

/* igraph 0.8.X shim */
#if IGRAPH_VERSION_MAJOR == 0
#if IGRAPH_VERSION_MINOR < 9
#define igraph_set_attribute_table igraph_i_set_attribute_table
#endif
#endif

#ifndef MARKER_SIZE_DEFAULT
#define MARKER_SIZE_DEFAULT 10
#endif

struct info {
    int verbose;
    double width;
    const char *pen;
    const char *format;
    struct {
        int valid;
        int symbol;
        double size;
    } marker;
    int has_color;
    int polygon_idx;
    igraph_t color;
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
    if (info->marker.valid) {
        pl_fmarker_r(info->plotter,
                     x, y, info->marker.symbol, info->marker.size);
    } else {
        pl_fpoint_r(info->plotter, x, y);
    }

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

static int w_label_polygon(struct info *info)
{
    double color;
    double x;
    double y;
    char color_s[32];

    color = igraph_cattribute_VAN(&info->color, "color", info->polygon_idx);
    x = igraph_cattribute_VAN(&info->color, "x", info->polygon_idx);
    y = igraph_cattribute_VAN(&info->color, "y", info->polygon_idx);
    snprintf(color_s, sizeof(color_s), "%g", color);
    pl_fmove_r(info->plotter, x, y);
    pl_alabel_r(info->plotter, 'c', 'c', color_s);

    return 0;
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

        if (info->has_color) {
            err = w_label_polygon(info);
            if (err) {
                break;
            }
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

        info->polygon_idx++;

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
    } else if (!strcmp(gtype, "LinearRing")) {
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

        /* delete plotter parms */
        if (info->param && pl_deleteplparams(info->param) < 0) {
            fprintf(stderr, "Could not delete plotter params\n");
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

static int set_point_format(struct info *info, const char *arg)
{
    int err = 1;
    char *opt;
    char *val;
    char *endp;

    opt = strdup(arg);
    assert(opt != NULL);
    val = strchr(opt, ',');
    if (val) {
        *val++ = 0;
    }

    do {
        errno = 0;
        info->marker.symbol = strtol(opt, &endp, 0);
        if (errno || *endp != 0) {
            /* Some kind of conversion error. */
            break;
        }
        info->marker.size = MARKER_SIZE_DEFAULT;
        if (val) {
            info->marker.size = strtod(val, &endp);
            if (errno || *endp != 0) {
                /* Some kind of conversion error. */
                break;
            }
        }

        info->marker.valid = 1;
        err = 0;

    } while (0);

    if (err) {
        fprintf(stderr, "Error converting %s\n",arg);
    }

    free(opt);

    return err;
}

static int color_read(struct info *info, const char *filename)
{
    int err = 1;
    FILE *f;

    do {
        igraph_set_attribute_table(&igraph_cattribute_table);
        f = fopen(filename, "r");
        if (f == NULL) {
            fprintf( stderr, "Cannot open %s: %s\n", filename, strerror(errno));
            break;
        }
        err = igraph_read_graph_graphml(&info->color, f, 0);
        fclose(f);
        info->has_color = !err;
    } while (0);

    return err;
}

static void color_cleanup(struct info *info)
{
    igraph_destroy(&info->color);
}

static void usage(const char *prog)
{
    fprintf(stderr,"%s -T format -O opt -p fmt [-bBvh] <input>\n", prog);
    fprintf(stderr,"  -h        Print this message\n");
    fprintf(stderr,"  -w n      Line width\n");
    fprintf(stderr,"  -p n[,m]  Points are marker n size m\n");
    fprintf(stderr,"  -T format Output format\n");
    fprintf(stderr,"  -c gml    Read color GML file\n");
    fprintf(stderr,"  -b        Input is WKB\n");
    fprintf(stderr,"  -B        Input is WKH\n");
    fprintf(stderr,"  -v        Verbose\n");
    fprintf(stderr,"  -O opt=v  Output option=v\n");
}

int main(int argc, char *argv[])
{
    int err = 1;
    int c;
    struct info info;
    const char *color_file = NULL;

    memset(&info, 0, sizeof(info));
    info.param = pl_newplparams();
    info.wkt.reader = WKT_IO_ASCII;
    info.width = 0.1;
    info.format = "svg";
    assert(info.param != NULL);

    while ((c = getopt(argc, argv, "w:T:O:c:p:bBvh")) != EOF) {
        switch (c) {
        case 'w':
            info.width = strtod(optarg,0);
            break;
        case 'T':
            info.format = optarg;
            break;
        case 'c':
            color_file = optarg;
            break;
        case 'O':
            set_option(&info, optarg);
            break;
        case 'p':
            set_point_format(&info, optarg);
            break;
        case 'b':
            info.wkt.reader = WKT_IO_BINARY;
            break;
        case 'B':
            info.wkt.reader = WKT_IO_HEX;
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
        err = wkt_open(&info.wkt);
        if (!err && color_file) {
            err = color_read(&info, color_file);
        }
        if (!err) {
            err = wkt_read(&info.wkt, argv[optind]);
        }
        if (!err) {
            err = w_plot(&info);
        }
        if (info.has_color) {
            color_cleanup(&info);
        }
        wkt_close(&info.wkt);
    }

    return err;
}
