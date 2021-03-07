/*
   wkt_open.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include "wkt.h"

int wkt_open(struct wkt *wkt, const char *file)
{
    int err = 1;

    wkt->handle = GEOS_init_r();

    if (wkt->handle == NULL) {
        return err;
    }

    switch (wkt->reader) {
    case WKT_READER_ASCII:
        wkt->wktr = GEOSWKTReader_create_r(wkt->handle);
        err = (wkt->wktr == NULL);
        break;
    case WKT_READER_BINARY:
    case WKT_READER_HEX:
        wkt->wkbr = GEOSWKBReader_create_r(wkt->handle);
        err = (wkt->wkbr == NULL);
        break;
    default:
        err = 1;
        break;
    }

    if (!err) {
        err = wkt_snag(wkt, file);
    }

    if (err) {
        wkt->reader = WKT_READER_NONE;
    }

    switch (wkt->reader) {
    case WKT_READER_ASCII:
        wkt->geom = GEOSWKTReader_read_r(wkt->handle, wkt->wktr, wkt->input);
        err = (wkt->geom == NULL);
        break;
    case WKT_READER_BINARY:
        wkt->geom = GEOSWKBReader_read_r(
            wkt->handle,
            wkt->wkbr,
            (const unsigned char *)wkt->input,
            wkt->input_len);
        err = (wkt->geom == NULL);
        break;
    case WKT_READER_HEX:
        wkt->geom = GEOSWKBReader_readHEX_r(
            wkt->handle,
            wkt->wkbr,
            (const unsigned char *)wkt->input,
            wkt->input_len);
        err = (wkt->geom == NULL);
        break;
    default:
        err = 1;
        break;
    }

    return err;
}
