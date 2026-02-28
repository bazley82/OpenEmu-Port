/*
    This file is part of CrabEmu.

    Copyright (C) 2009, 2012, 2014 Lawrence Sebald

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

#ifndef CRABEMU_H
#define CRABEMU_H

#define VERSION "0.2.1"

#ifdef __cplusplus
#define CLINKAGE extern "C" {
#define ENDCLINK }
#else
#define CLINKAGE
#define ENDCLINK
#endif /* __cplusplus */

#ifndef CRABEMU_TYPEDEFS
#define CRABEMU_TYPEDEFS

#ifdef _arch_dreamcast
#include <arch/types.h>
#include <stdint.h>
#else
#include <stdint.h>
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
#endif /* _arch_dreamcast */
#endif /* CRABEMU_TYPEDEFS */

#if defined(SMS_VDP_32BIT_COLOR) && !defined(CRABEMU_32BIT_COLOR)
#define CRABEMU_32BIT_COLOR
#endif

#ifdef CRABEMU_32BIT_COLOR
typedef uint32 pixel_t;
#else
typedef uint16 pixel_t;
#endif

#if defined(__GNUC__) || defined(__GNUG__)
#ifndef __UNUSED__
#define __UNUSED__ __attribute__((unused))
#define __INLINE__ inline
#endif
#else
#ifndef __UNUSED__
#define __UNUSED__
#define __INLINE__
#endif
#endif /* __GNUC__ || __GNUG__ */

#ifdef _MSC_VER
#define snprintf _snprintf
#define strcasecmp _stricmp

#pragma warning(disable : 4996) /* "unsafe" standard functions */
#pragma warning(disable : 4244) /* loss of precision */
#pragma warning(disable : 4146) /* negation of unsigned type */
#endif

#define UINT32_TO_BUF(n, buf) { \
    uint32 n_ = (n); \
    uint8 *buf_ = (buf); \
    buf_[3] = (uint8)(n_ >> 24); \
    buf_[2] = (uint8)(n_ >> 16); \
    buf_[1] = (uint8)(n_ >> 8); \
    buf_[0] = (uint8)(n_); \
}

#define BUF_TO_UINT32(buf, res) { \
    const uint8 *buf_ = (const uint8 *)(buf); \
    res = (((uint32)buf_[3]) << 24) | (((uint32)buf_[2]) << 16) | \
        (((uint32)buf_[1]) << 8) | ((uint32)buf_[0]); \
}

#define UINT16_TO_BUF(n, buf) { \
    uint16 n_ = (n); \
    uint8 *buf_ = (buf); \
    buf_[1] = (uint8)(n_ >> 8); \
    buf_[0] = (uint8)(n_); \
}

#define BUF_TO_UINT16(buf, res) { \
    const uint8 *buf_ = (const uint8 *)(buf); \
    res = (((uint16)buf_[1]) << 8) | ((uint16)buf_[0]); \
}

#define FOURCC_TO_UINT32(c1, c2, c3, c4) \
    ((uint32)c1) | (((uint32)c2) << 8) | (((uint32)c3) << 16) | \
        (((uint32)c4) << 24)

/* Return values for the various rom loading functions. */
#define ROM_LOAD_SUCCESS        0               /* Successfully loaded rom */
#define ROM_LOAD_E_ERRNO        -1              /* Check errno for error code */
#define ROM_LOAD_E_OPEN         -2              /* Cannot open file */
#define ROM_LOAD_E_NO_ROM       -3              /* File doesn't contain a rom */
#define ROM_LOAD_E_CORRUPT      -4              /* File appears to be corrupt */
#define ROM_LOAD_E_NO_MEM       -5              /* Out of memory */
#define ROM_LOAD_E_BAD_SZ       -6              /* Rom not of valid size */
#define ROM_LOAD_E_UNK_MAPPER   -7              /* Mapper is unsupported */
#define ROM_LOAD_E_HAS_TRAINER  -8              /* Has unsupported trainer */

/* Video systems. */
#define VIDEO_NTSC      0x10
#define VIDEO_PAL       0x20

CLINKAGE
extern void gui_set_aspect(float x, float y);
extern void gui_set_title(const char *str);

/* Forward declaration... */
struct crabemu_console;

extern void gui_set_console(struct crabemu_console *c);
ENDCLINK

#endif /* !CRABEMU_H */
