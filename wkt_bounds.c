/*
   wkt_bounds.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include "wkt.h"

int wkt_bounds(
    struct wkt *wkt,
    double *xmin,
    double *xmax,
    double *ymin,
    double *ymax)
{
    int rc = 0;

    do {
        /* get bounds */
        rc = GEOSGeom_getXMin_r(wkt->handle, wkt->geom, xmin);
        if (!rc) {
            break;
        }

        rc = GEOSGeom_getXMax_r(wkt->handle, wkt->geom, xmax);
        if (!rc) {
            break;
        }

        rc = GEOSGeom_getYMin_r(wkt->handle, wkt->geom, ymin);
        if (!rc) {
            break;
        }

        rc = GEOSGeom_getYMax_r(wkt->handle, wkt->geom, ymax);
        if (!rc) {
            break;
        }

    } while (0);

    return rc;
}

