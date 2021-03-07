/*
   wkt.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   _WKT_H_
#define   _WKT_H_

#include <geos_c.h>

typedef enum {
    WKT_IO_NONE,
    WKT_IO_ASCII,
    WKT_IO_BINARY,
    WKT_IO_HEX,
} wkt_io_t;

struct wkt {
    wkt_io_t reader;
    wkt_io_t writer;
    const char *input;
    size_t input_len;
    GEOSGeometry *geom;
    GEOSWKTReader *wktr;
    GEOSWKBReader *wkbr;
    GEOSWKTWriter *wktw;
    GEOSWKBWriter *wkbw;
    GEOSContextHandle_t handle;
};

typedef int (*wkt_iterator_t)(
    struct wkt *wkt,
    const GEOSGeometry *geom,
    const char *gtype,
    void *user_data);

extern int wkt_open(struct wkt *wkt);
extern int wkt_read(struct wkt *wkt, const char *file);
extern int wkt_snag(struct wkt *wkt, const char *file);
extern int wkt_close(struct wkt *wkt);
extern int wkt_iterate_coord_seq(
    struct wkt *wkt,
    const GEOSGeometry *geom,
    int (*handler)(struct wkt *, unsigned, unsigned, double, double, void *),
    void *user_data);
extern int wkt_bounds(
    struct wkt *wkt,
    double *xmin,
    double *xmax,
    double *ymin,
    double *ymax);
extern int wkt_iterate(
    struct wkt *wkt,
    wkt_iterator_t iterator,
    void *user_data);
extern int wkt_write(
    struct wkt *wkt,
    const char *file,
    const GEOSGeometry *geom);
extern int wkt_stash(
    const char *file,
    const char *data,
    size_t len);

#endif /* _WKT_H_ */
