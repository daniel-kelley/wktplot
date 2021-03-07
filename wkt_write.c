/*
   wkt_write.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <string.h>
#include "wkt.h"

int wkt_write(struct wkt *wkt, const char *file, const GEOSGeometry *geom)
{
    int err = 1;
    const char *data;
    size_t len;

    switch (wkt->writer) {
    case WKT_IO_ASCII:
        data = GEOSWKTWriter_write_r(wkt->handle, wkt->wktw, geom);
        if (data) {
            len = strlen(data);
            err = 0;
        }
        break;
    case WKT_IO_BINARY:
        data = (char *)GEOSWKBWriter_write_r(
            wkt->handle,
            wkt->wkbw,
            geom,
            &len);
        err = (data == NULL);
        break;
    case WKT_IO_HEX:
        data = (char *)GEOSWKBWriter_writeHEX_r(
            wkt->handle,
            wkt->wkbw,
            geom,
            &len);
        err = (data == NULL);
        break;
    default:
        err = 1;
        break;
    }

    if (!err) {
        err = wkt_stash(file, data, len);
        GEOSFree_r(wkt->handle, (void *)data);
    }

    return err;
}
