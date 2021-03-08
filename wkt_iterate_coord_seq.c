/*
   wkt_iterate_coord_seq.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <stdio.h>
#include "wkt.h"

int wkt_iterate_coord_seq(
    struct wkt *wkt,
    const GEOSGeometry *geom,
    int (*handler)(struct wkt *, unsigned, unsigned, double, double, void *),
    void *user_data)
{
    int err = 1;
    int rc;
    unsigned int n;
    unsigned int d;
    unsigned int i;
    const GEOSCoordSequence* s;

    do {
        s = GEOSGeom_getCoordSeq_r(wkt->handle, geom);
        if (s == NULL) {
            break;
        }

        rc = GEOSCoordSeq_getSize_r(wkt->handle, s, &n);
        if (!rc) {
            break;
        }

        rc = GEOSCoordSeq_getDimensions_r(wkt->handle, s, &d);
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

            rc = GEOSCoordSeq_getXY_r(wkt->handle, s, i, &x, &y);
            if (!rc) {
                err = 1;
                break;
            }
            err = handler(wkt, i, n, x, y, user_data);
            if (err) {
                break;
            }
        }

    } while (0);

    return err;
}

