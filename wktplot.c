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

typedef enum {
    READER_NONE,
    READER_ASCII,       /* default */
    READER_BINARY,      /* -b      */
    READER_HEX,         /* -B      */
} reader_t;

struct info {
    int verbose;
    double width;
    const char *pen;
    reader_t reader;
    const char *format;
    plPlotter *plotter;
    plPlotterParams *param;
    const char *input;
    size_t input_len;
    GEOSGeometry *geom;
    GEOSWKTReader *wktr;
    GEOSWKBReader *wkbr;
    GEOSContextHandle_t handle;
};

static int w_iterate_coord_seq(
    struct info *info,
    const GEOSGeometry *geom,
    int (*handler)(struct info *, unsigned, unsigned, double, double, void *),
    void *user_data)
{
    int err = 1;
    int rc;
    unsigned int n;
    unsigned int d;
    unsigned int i;
    const GEOSCoordSequence* s;

    do {
        s = GEOSGeom_getCoordSeq_r(info->handle, geom);
        if (s == NULL) {
            break;
        }

        rc = GEOSCoordSeq_getSize_r(info->handle, s, &n);
        if (!rc) {
            break;
        }

        rc = GEOSCoordSeq_getDimensions_r(info->handle, s, &d);
        if (!rc) {
            break;
        }

        if (d != 2) {
            fprintf(stderr, "Unsupported Point dimension %u\n", d);
            break;
        }

        err = 0;

        for (i=0; i<n; i++) {
            double x;
            double y;

            rc = GEOSCoordSeq_getXY_r(info->handle, s, i, &x, &y);
            if (!rc) {
                err = 1;
                break;
            }
            err = handler(info, i, n, x, y, user_data);
            if (err) {
                break;
            }
        }

    } while (0);

    return err;
}

static int w_point_iterator(
    struct info *info,
    unsigned i,
    unsigned n,
    double x,
    double y,
    void *user_data)
{
    (void)user_data;
    if (info->verbose) {
        fprintf(stderr, "Point %u/%u [%g,%g]\n", i, n, x, y);
    }
    pl_fpoint_r(info->plotter, x, y);

    return 0;
}

static int w_line_iterator(
    struct info *info,
    unsigned i,
    unsigned n,
    double x,
    double y,
    void *user_data)
{
    double *prev = user_data;

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
    return w_iterate_coord_seq(info, geom, w_point_iterator, NULL);
}

static int w_handle_polygon(struct info *info, const GEOSGeometry *geom)
{
    int err = 1;
    int n;
    int i;
    const GEOSGeometry *g;
    double prev[2];

    do {
        g = GEOSGetExteriorRing_r(info->handle, geom);
        if (g==NULL) {
            break;
        }

        err = w_iterate_coord_seq(info, g, w_line_iterator, prev);
        if (err) {
            break;
        }

        n = GEOSGetNumInteriorRings_r(info->handle, geom);

        for (i=0; i<n; i++) {
            g = GEOSGetInteriorRingN_r(info->handle, geom, i);
            if (g==NULL) {
                break;
            }

            err = w_iterate_coord_seq(info, g, w_line_iterator, prev);
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
    double prev[2];

    err = w_iterate_coord_seq(info, geom, w_line_iterator, prev);

    return err;
}

static int w_handle(struct info *info, const GEOSGeometry *geom)
{
    int err = 1;
    char *gtype;

    gtype = GEOSGeomType_r(info->handle, geom);

    if (!strcmp(gtype, "Point")) {
        err = w_handle_point(info, geom);
    } else if (!strcmp(gtype, "Polygon")) {
        err = w_handle_polygon(info, geom);
    } else if (!strcmp(gtype, "LineString")) {
        err = w_handle_linestring(info, geom);
    } else {
        fprintf(stderr,"Missing handler for %s\n", gtype);
    }

    free(gtype);

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
        /* get bounds */
        rc = GEOSGeom_getXMin_r(info->handle, info->geom, &xmin);
        if (!rc) {
            break;
        }

        rc = GEOSGeom_getXMax_r(info->handle, info->geom, &xmax);
        if (!rc) {
            break;
        }

        rc = GEOSGeom_getYMin_r(info->handle, info->geom, &ymin);
        if (!rc) {
            break;
        }

        rc = GEOSGeom_getYMax_r(info->handle, info->geom, &ymax);
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
    int i;
    int n;

    /* interpret */
    n = GEOSGetNumGeometries_r(info->handle, info->geom);
    for (i=0; i<n; i++) {
        err = w_handle(info, GEOSGetGeometryN_r(info->handle, info->geom, i));
        if (err) {
            break;
        }
    }

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

static int w_snag(struct info *info, const char *file)
{
    int err = 1;
    int fd;
    struct stat stat;

    do {
        fd = open(file, O_RDONLY);
        /* open */
        if (fd < 0) {
            fprintf(stderr, "%s: %s", file, strerror(errno));
            break;
        }

        /* fstat */
        err = fstat(fd, &stat);

        if (err) {
            fprintf(stderr, "%s: %s", file, strerror(errno));
            break;
        }

        info->input_len = stat.st_size;

        /* mmap */
        info->input = mmap(
            NULL,
            info->input_len,
            PROT_READ,
            MAP_PRIVATE,
            fd,
            0);

        if (!info->input) {
            fprintf(stderr, "%s: %s", file, strerror(errno));
            err = -1;
            break;
        }


    } while (0);

    if (fd >= 0 ) {
        close(fd); /* fd not needed any more */
    }

    return err;
}
static int w_open(struct info *info, const char *file)
{
    int err = 1;

    info->handle = GEOS_init_r();
    assert(info->handle != NULL);

    switch (info->reader) {
    case READER_ASCII:
        info->wktr = GEOSWKTReader_create_r(info->handle);
        err = (info->wktr == NULL);
        break;
    case READER_BINARY:
    case READER_HEX:
        info->wkbr = GEOSWKBReader_create_r(info->handle);
        err = (info->wkbr == NULL);
        break;
    default:
        err = 1;
        break;
    }

    if (!err) {
        err = w_snag(info, file);
    }

    if (err) {
        info->reader = READER_NONE;
    }

    switch (info->reader) {
    case READER_ASCII:
        info->geom = GEOSWKTReader_read_r(info->handle, info->wktr, info->input);
        err = (info->geom == NULL);
        break;
    case READER_BINARY:
        info->geom = GEOSWKBReader_read_r(
            info->handle,
            info->wkbr,
            (const unsigned char *)info->input,
            info->input_len);
        err = (info->geom == NULL);
        break;
    case READER_HEX:
        info->geom = GEOSWKBReader_readHEX_r(
            info->handle,
            info->wkbr,
            (const unsigned char *)info->input,
            info->input_len);
        err = (info->geom == NULL);
        break;
    default:
        err = 1;
        break;
    }

    return err;
}

static int w_close(struct info *info)
{
    int err = 1;

    if (info->wktr) {
        GEOSWKTReader_destroy_r(info->handle, info->wktr);
    }

    if (info->wkbr) {
        GEOSWKBReader_destroy_r(info->handle, info->wkbr);
    }

    GEOS_finish_r(info->handle);

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
    info.reader = READER_ASCII;
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
            info.reader = READER_BINARY;
            break;
        case 'B':
            info.reader = READER_HEX;
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
        err = w_open(&info, argv[optind]);
        if (!err) {
            err = w_plot(&info);
        }
        w_close(&info);
    }

    return err;
}
