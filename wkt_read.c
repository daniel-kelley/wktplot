/*
   wkt_read.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include "wkt.h"

int wkt_read(struct wkt *wkt, const char *file)
{
    int err = 1;

    err = wkt_snag(wkt, file);

    if (err) {
        wkt->reader = WKT_IO_NONE;
    }

    switch (wkt->reader) {
    case WKT_IO_ASCII:
        wkt->geom = GEOSWKTReader_read_r(wkt->handle, wkt->wktr, wkt->input);
        err = (wkt->geom == NULL);
        break;
    case WKT_IO_BINARY:
        wkt->geom = GEOSWKBReader_read_r(
            wkt->handle,
            wkt->wkbr,
            (const unsigned char *)wkt->input,
            wkt->input_len);
        err = (wkt->geom == NULL);
        break;
    case WKT_IO_HEX:
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
