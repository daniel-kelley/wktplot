/*
   wkt_close.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include "wkt.h"

int wkt_close(struct wkt *wkt)
{
    int err = 1;

    if (wkt->wktr) {
        GEOSWKTReader_destroy_r(wkt->handle, wkt->wktr);
    }

    if (wkt->wkbr) {
        GEOSWKBReader_destroy_r(wkt->handle, wkt->wkbr);
    }

    GEOS_finish_r(wkt->handle);

    return err;
}

