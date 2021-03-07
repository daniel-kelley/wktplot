/*
   wkt_open.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include "wkt.h"

int wkt_open(struct wkt *wkt)
{
    int err = 1;

    wkt->handle = GEOS_init_r();

    if (wkt->handle == NULL) {
        return err;
    }

    switch (wkt->reader) {
    case WKT_IO_ASCII:
        wkt->wktr = GEOSWKTReader_create_r(wkt->handle);
        err = (wkt->wktr == NULL);
        break;
    case WKT_IO_BINARY:
    case WKT_IO_HEX:
        wkt->wkbr = GEOSWKBReader_create_r(wkt->handle);
        err = (wkt->wkbr == NULL);
        break;
    case WKT_IO_NONE:
        err = 0;
        break;
    default:
        err = 1;
        break;
    }

    if (err) {
        return err;
    }

    switch (wkt->writer) {
    case WKT_IO_ASCII:
        wkt->wktw = GEOSWKTWriter_create_r(wkt->handle);
        err = (wkt->wktw == NULL);
        break;
    case WKT_IO_BINARY:
    case WKT_IO_HEX:
        wkt->wkbw = GEOSWKBWriter_create_r(wkt->handle);
        err = (wkt->wkbw == NULL);
        break;
    case WKT_IO_NONE:
        err = 0;
        break;
    default:
        err = 1;
        break;
    }

    return err;
}
