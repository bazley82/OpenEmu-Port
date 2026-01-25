/*
    This file is part of CrabEmu.

    Copyright (C) 2009, 2014 Lawrence Sebald

    CrabEmu is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    CrabEmu is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CrabEmu; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "scale2x.h"

#define PIXEL1(a, b, c, d, p) (c == a && c != d && a != b) ? a : p;
#define PIXEL2(a, b, c, d, p) (a == b && a != c && b != d) ? b : p;
#define PIXEL3(a, b, c, d, p) (d == c && d != b && d != c) ? c : p;
#define PIXEL4(a, b, c, d, p) (b == d && b != a && d != c) ? d : p;

/* This is ugly (hooray pointer arithmetic), but at least it's nice and
   efficient (at least more so than by using vanilla array accesses)... */
void scale2x_scale(const pixel_t *inbuf, pixel_t *outbuf, int width,
                   int height) {
    int i;
    pixel_t a, b, c, d, p;
    int neww = (width << 1) - 2;

    /* Top row comes first. */
    /* Upper-left pixel */
    p = *inbuf;
    d = *(inbuf + width);
    b = *(++inbuf);

    *(outbuf++) = PIXEL2(0, b, 0, d, p);    /* Duplicate. */
    *(outbuf++) = PIXEL2(0, b, 0, d, p);
    *(outbuf + neww + 1) = PIXEL4(0, b, 0, d, p);
    *(outbuf + neww) = PIXEL3(0, b, 0, d, p);

    i = width - 1;

    while(--i) {
        c = p;
        p = b;
        d = *(inbuf + width);
        b = *(++inbuf);

        *(outbuf++) = PIXEL1(0, b, c, d, p);
        *(outbuf++) = PIXEL2(0, b, c, d, p);
        *(outbuf + neww + 1) = PIXEL4(0, b, c, d, p);
        *(outbuf + neww) = PIXEL3(0, b, c, d, p);
    }

    /* Upper-right pixel */
    c = p;
    p = b;
    d = *(inbuf++ + width);

    *(outbuf++) = PIXEL1(0, 0, c, d, p);
    *(outbuf++) = PIXEL1(0, 0, c, d, p);    /* Duplicate. */
    *(outbuf + neww + 1) = PIXEL4(0, 0, c, d, p);
    *(outbuf + neww) = PIXEL3(0, 0, c, d, p);
    outbuf += neww + 2;

    /* The whole middle of the image. */
    --height;

    while(--height) {
        /* First pixel on the line. */
        a = *(inbuf - width);
        p = *inbuf;
        d = *(inbuf + width);
        b = *(++inbuf);

        *(outbuf++) = PIXEL1(a, b, 0, d, p);
        *(outbuf++) = PIXEL2(a, b, 0, d, p);
        *(outbuf + neww + 1) = PIXEL4(a, b, 0, d, p);
        *(outbuf + neww) = PIXEL3(a, b, 0, d, p);

        i = width - 1;

        while(--i) {
            c = p;
            p = b;
            a = *(inbuf - width);
            d = *(inbuf + width);
            b = *(++inbuf);

            *(outbuf++) = PIXEL1(a, b, c, d, p);
            *(outbuf++) = PIXEL2(a, b, c, d, p);
            *(outbuf + neww + 1) = PIXEL4(a, b, c, d, p);
            *(outbuf + neww) = PIXEL3(a, b, c, d, p);
        }

        /* Last pixel on the line. */
        c = p;
        p = b;
        a = *(inbuf - width);
        d = *(inbuf + width);

        *(outbuf++) = PIXEL1(a, 0, c, d, p);
        *(outbuf++) = PIXEL2(a, 0, c, d, p);
        *(outbuf + neww + 1) = PIXEL4(a, 0, c, d, p);
        *(outbuf + neww) = PIXEL3(a, 0, c, d, p);
        ++inbuf;
        outbuf += neww + 2;
    }

    /* First pixel on the last line. */
    a = *(inbuf - width);
    p = *(inbuf);
    b = *(++inbuf);

    *(outbuf++) = PIXEL1(a, b, 0, 0, p);
    *(outbuf++) = PIXEL2(a, b, 0, 0, p);
    *(outbuf + neww + 1) = PIXEL4(a, b, 0, 0, p);
    *(outbuf + neww) = PIXEL3(a, b, 0, 0, p);

    i = width - 1;

    while(--i) {
        c = p;
        p = b;
        a = *(inbuf - width);
        b = *(++inbuf);

        *(outbuf++) = PIXEL1(a, b, c, 0, p);
        *(outbuf++) = PIXEL2(a, b, c, 0, p);
        *(outbuf + neww + 1) = PIXEL4(a, b, c, 0, p);
        *(outbuf + neww) = PIXEL3(a, b, c, 0, p);
    }

    /* Last pixel on the line. */
    c = p;
    p = b;
    a = *(inbuf - width);

    *(outbuf++) = PIXEL1(a, 0, c, 0, p);
    *(outbuf++) = PIXEL2(a, 0, c, 0, p);
    *(outbuf + neww + 1) = PIXEL3(a, 0, c, 0, p);   /* Duplicate. */
    *(outbuf + neww) = PIXEL3(a, 0, c, 0, p);
}

#undef PIXEL1
#undef PIXEL2
#undef PIXEL3
#undef PIXEL4
