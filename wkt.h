/*
   wkt.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   _WKT_H_
#define   _WKT_H_

#include <geos_c.h>

typedef enum {
    WKT_READER_NONE,
    WKT_READER_ASCII,
    WKT_READER_BINARY,
    WKT_READER_HEX,
} wkt_reader_t;

struct wkt {
    wkt_reader_t reader;
    const char *input;
    size_t input_len;
    GEOSGeometry *geom;
    GEOSWKTReader *wktr;
    GEOSWKBReader *wkbr;
    GEOSContextHandle_t handle;
};

extern int wkt_open(struct wkt *wkt, const char *file);
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

#endif /* _WKT_H_ */
