/*
   wkt_iterate.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include "wkt.h"

int wkt_iterate(struct wkt *wkt, wkt_iterator_t iterator, void *user_data)
{
    int err = 1;
    int i;
    int n;
    char *gtype;
    const GEOSGeometry *geom;

    /* iterate across geometries */
    n = GEOSGetNumGeometries_r(wkt->handle, wkt->geom);
    for (i=0; i<n; i++) {
        geom = GEOSGetGeometryN_r(wkt->handle, wkt->geom, i);
        if (geom == NULL) {
            break;
        }

        gtype = GEOSGeomType_r(wkt->handle, geom);
        if (gtype == NULL) {
            break;
        }

        err = iterator(wkt, geom, gtype, user_data);

        GEOSFree_r(wkt->handle, gtype);

        if (err) {
            break;
        }
    }

    return err;
}

