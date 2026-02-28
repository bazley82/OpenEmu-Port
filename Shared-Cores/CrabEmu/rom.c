/*
    This file is part of CrabEmu.

    Copyright (C) 2012, 2013, 2015 Lawrence Sebald

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _arch_dreamcast
#include <zlib/zlib.h>
#include <bzlib/bzlib.h>
#else
#ifndef NO_ZLIB
#include <zlib.h>
#endif

#ifndef NO_BZ2
#include <bzlib.h>
#endif
#endif

#include "rom.h"
#include "sms.h"
#include "colecovision.h"
#include "nes.h"

#ifndef NO_ZLIB
#include "unzip.h"
#endif

/* Special "console" values */
#define TYPE_GZIP   -100
#define TYPE_BZIP2  -101
#define TYPE_ZIP    -102

static int guess_on_ext(const char *fn, int chop) {
    char *fn2 = NULL;
    char *ext;
    int rv = -1;

    /* Grab the extension of the file... */
    if(chop) {
        if(!(fn2 = strdup(fn)))
            return -1;

        if(!(ext = strrchr(fn2, '.'))) {
            free(fn2);
            return -1;
        }

        *ext = '\0';

        ext = strrchr(fn2, '.');
    }
    else {
        ext = strrchr(fn, '.');
    }

    /* If there's no extension, we can't really take a guess... */
    if(!ext)
        rv = -1;
    else if(!strcasecmp(ext, ".sms"))
        return CONSOLE_SMS;
    else if(!strcasecmp(ext, ".gg"))
        return CONSOLE_GG;
    else if(!strcasecmp(ext, ".sc"))
        return CONSOLE_SC3000;
    else if(!strcasecmp(ext, ".sg"))
        return CONSOLE_SG1000;
    else if(!strcasecmp(ext, ".rom") || !strcasecmp(ext, ".col"))
        return CONSOLE_COLECOVISION;
    else if(!strcasecmp(ext, ".nes"))
        return CONSOLE_NES;
    else if(!strcasecmp(ext, ".ch8") || !strcasecmp(ext, ".c8"))
        return CONSOLE_CHIP8;
#ifndef NO_ZLIB
    else if(!strcasecmp(ext, ".gz"))
        return TYPE_GZIP;
    else if(!strcasecmp(ext, ".zip"))
        return TYPE_ZIP;
#endif
#ifndef NO_BZ2
    else if(!strcasecmp(ext, ".bz2"))
        return TYPE_BZIP2;
#endif

    if(fn2)
        free(fn2);

    return rv;
}

#ifndef NO_ZLIB
static int detect_gz(const char *fn) {
    FILE *fp;
    uint8 buf[2];
    int seek_amt, fn_len = 0, max_fn_len = 4096, ch;
    char *filename;
    void *tmp;

    /* Open up the file and try to see if the original filename is in the file's
       header. If it is, use that to try to guess further. Otherwise, we'll try
       to guess based on the current filename... */
    if(!(fp = fopen(fn, "rb"))) {
        return -2;
    }

    /* Read the first two bytes to make sure this is actually a gzip file. */
    if(fread(buf, 1, 2, fp) != 2) {
        fclose(fp);
        return -2;
    }

    if(buf[0] != 0x1F || buf[1] != 0x8B) {
        fclose(fp);
        return -2;
    }

    /* Read in the next two bytes so we can grab the filename, if its there. */
    if(fread(buf, 1, 2, fp) != 2) {
        fclose(fp);
        return -2;
    }

    /* Is the FNAME bit set? */
    if(buf[1] & 0x08) {
        /* Skip the rest of the header... */
        if(fseek(fp, 6, SEEK_CUR)) {
            fclose(fp);
            return -2;
        }

        /* If the FEXTRA bit is set, we have to skip past that too. */
        if(buf[1] & 0x04) {
            if(fread(buf, 1, 2, fp) != 2) {
                fclose(fp);
                return -2;
            }

            seek_amt = buf[0] | (buf[1] << 8);

            if(fseek(fp, seek_amt, SEEK_CUR)) {
                fclose(fp);
                return -2;
            }
        }

        /* We should be at the filename field now. Read it in... */
        if(!(filename = (char *)malloc(4096))) {
            fclose(fp);
            return -2;
        }

        for(;;) {
            ch = fgetc(fp);

            if(ch == EOF) {
                free(filename);
                fclose(fp);
                return -2;
            }
            else if(ch == 0) {
                filename[fn_len] = 0;
                break;
            }
            else {
                filename[fn_len++] = (char)ch;

                if(fn_len == max_fn_len) {
                    if(!(tmp = realloc(filename, max_fn_len << 1))) {
                        free(filename);
                        fclose(fp);
                        return -2;
                    }

                    max_fn_len <<= 1;
                }
            }
        }

        /* We've got the filename, take our guess and clean up. */
        ch = guess_on_ext(filename, 0);
        free(filename);
        fclose(fp);
        return ch;
    }

    fclose(fp);

    /* If we're still here, we have to punt and use the method of trying to
       parse out the current filename... */
    return guess_on_ext(fn, 1);
}
#endif

#ifndef NO_BZ2
static int detect_bz2(const char *fn) {
    /* We really don't have a choice here... Guess based on the extension before
       .bz2. */
    return guess_on_ext(fn, 1);
}
#endif

#ifndef NO_ZLIB
static int detect_zip(const char *fn) {
    int console = 0;
    unzFile fp;
    unz_file_info inf;
    char fn2[4096];

    /* Open up the file in question. */
    if(!(fp = unzOpen(fn))) {
#ifdef DEBUG
        fprintf(stderr, "rom_detect_console: Could not open ROM: %s!\n", fn);
#endif
        return -2;
    }

    if(unzGoToFirstFile(fp) != UNZ_OK) {
#ifdef DEBUG
        fprintf(stderr, "rom_detect_console: Couldn't find file in zip!\n");
#endif
        unzClose(fp);
        return -2;
    }

    /* Keep looking at files until we find a rom. */
    while(console <= 0) {
        if(unzGetCurrentFileInfo(fp, &inf, fn2, 4096, NULL, 0, NULL,
                                 0) != UNZ_OK) {
#ifdef DEBUG
            fprintf(stderr, "rom_detect_console: Error parsing zip file!\n");
#endif
            unzClose(fp);
            return -2;
        }

        /* Check if this file looks like a rom. */
        console = guess_on_ext(fn2, 0);

        /* If we haven't found a rom yet, move to the next file. */
        if(console <= 0) {
            if(unzGoToNextFile(fp) != UNZ_OK) {
#ifdef DEBUG
                fprintf(stderr, "rom_detect_console: End of zip file!\n");
#endif
                unzClose(fp);
                return -2;
            }
        }
    }

    /* If we get here, we have our console. Return it! */
    return console;
}
#endif

int rom_detect_console(const char *fn) {
    int rv;

    rv = guess_on_ext(fn, 0);
    switch(rv) {
#ifndef NO_ZLIB
        case TYPE_GZIP:
            return detect_gz(fn);

        case TYPE_ZIP:
            return detect_zip(fn);
#endif

#ifndef NO_BZ2
        case TYPE_BZIP2:
            return detect_bz2(fn);
#endif

        default:
            return rv;
    }
}

uint32 rom_crc32(const uint8 *data, int size) {
    int i;
    uint32 rv = 0xFFFFFFFF;

    for(i = 0; i < size; ++i) {
        rv ^= data[i];
        rv = (0xEDB88320 & (-(rv & 1))) ^(rv >> 1);
        rv = (0xEDB88320 & (-(rv & 1))) ^(rv >> 1);
        rv = (0xEDB88320 & (-(rv & 1))) ^(rv >> 1);
        rv = (0xEDB88320 & (-(rv & 1))) ^(rv >> 1);
        rv = (0xEDB88320 & (-(rv & 1))) ^(rv >> 1);
        rv = (0xEDB88320 & (-(rv & 1))) ^(rv >> 1);
        rv = (0xEDB88320 & (-(rv & 1))) ^(rv >> 1);
        rv = (0xEDB88320 & (-(rv & 1))) ^(rv >> 1);
    }

    return ~rv;
}

/* Public-domain adler32 checksum. Borrowed from stb-2.23, which has the
   following comment at the top of it:

   stb-2.23 - Sean's Tool Box -- public domain -- http://nothings.org/stb.h
          no warranty is offered or implied; use this code at your own risk

   This is a single header file with a bunch of useful utilities
   for getting stuff done in C/C++.

   Email bug reports, feature requests, etc. to 'sean' at the same site.


   Documentation: http://nothings.org/stb/stb_h.html
   Unit tests:    http://nothings.org/stb/stb.c
*/
#include <stdint.h>

#define STB_ADLER32_SEED   1

uint32 rom_adler32(const uint8 *buffer, uint32 buflen) {
    const unsigned long ADLER_MOD = 65521;
    unsigned long s1 = STB_ADLER32_SEED & 0xffff, s2 = STB_ADLER32_SEED >> 16;
    unsigned long blocklen, i;

    blocklen = buflen % 5552;
    while (buflen) {
        for (i=0; i + 7 < blocklen; i += 8) {
            s1 += buffer[0], s2 += s1;
            s1 += buffer[1], s2 += s1;
            s1 += buffer[2], s2 += s1;
            s1 += buffer[3], s2 += s1;
            s1 += buffer[4], s2 += s1;
            s1 += buffer[5], s2 += s1;
            s1 += buffer[6], s2 += s1;
            s1 += buffer[7], s2 += s1;

            buffer += 8;
        }

        for (; i < blocklen; ++i)
            s1 += *buffer++, s2 += s1;

        s1 %= ADLER_MOD, s2 %= ADLER_MOD;
        buflen -= blocklen;
        blocklen = 5552;
    }

    return (s2 << 16) + s1;
}
