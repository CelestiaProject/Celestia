// bytes.h
//
// Copyright (C) 2001, Colin Walters <walters@verbum.org>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELUTIL_BYTES_H_
#define _CELUTIL_BYTES_H_

#include <config.h>

#ifndef __BYTE_ORDER__
# ifdef _WIN32
// FIXME: we assume that windows runs on LE hw only
# define __BYTE_ORDER__ 1234
# else
# error "Unknown system or compiler"
# endif
#endif

/* Use the system byteswap.h definitions if we have them */
#if defined(HAVE_BYTESWAP_H)
#include <byteswap.h>
#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define bswap_16 OSSwapInt16
#define bswap_32 OSSwapInt32
#define bswap_64 OSSwapInt64
#elif defined(_MSC_VER) && (_MSC_VER >= 1400)
#include <stdlib.h>
#define bswap_32 _byteswap_ulong
#define bswap_16 _byteswap_ushort
#define bswap_64 _byteswap_uint64
#else
static unsigned short bswap_16 (uint16_t val)
{
  return ((((val) >> 8) & 0xff) | (((val) & 0xff) << 8));
}

static unsigned int bswap_32(uint32_t val) {
  return (((val) & 0xff000000) >> 24) | (((val) & 0x00ff0000) >>  8) |
    (((val) & 0x0000ff00) <<  8) | (((val) & 0x000000ff) << 24);
}

static unsigned int bswap_64(uint64_t val) {
  return (((val & 0xff00000000000000ull) >> 56) |
          ((val & 0x00ff000000000000ull) >> 40) |
          ((val & 0x0000ff0000000000ull) >> 24) |
          ((val & 0x000000ff00000000ull) >> 8)  |
          ((val & 0x00000000ff000000ull) << 8)  |
          ((val & 0x0000000000ff0000ull) << 24) |
          ((val & 0x000000000000ff00ull) << 40) |
          ((val & 0x00000000000000ffull) << 56));
}
#endif

inline double bswap_double(double d)
{
    static_assert(sizeof(double) == 8, "Size of double must be 8.");

    union DoubleBytes
    {
        char bytes[8];
        double d;
    };
    DoubleBytes db;
    db.d = d;

    char c;
    c = db.bytes[0]; db.bytes[0] = db.bytes[7]; db.bytes[7] = c;
    c = db.bytes[1]; db.bytes[1] = db.bytes[6]; db.bytes[6] = c;
    c = db.bytes[2]; db.bytes[2] = db.bytes[5]; db.bytes[5] = c;
    c = db.bytes[3]; db.bytes[3] = db.bytes[4]; db.bytes[4] = c;

    return db.d;
}

#define SWAP_FLOAT(x, y) do { *(unsigned int *)&x = bswap_32( *(unsigned int *)&y ); } while (0)

#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)

#define LE_TO_CPU_INT16(ret, val) (ret = bswap_16(val))

#define LE_TO_CPU_INT32(ret, val) (ret = bswap_32(val))

#define LE_TO_CPU_FLOAT(ret, val) SWAP_FLOAT(ret, val)

#define LE_TO_CPU_DOUBLE(ret, val) (ret = bswap_double(d))

#define BE_TO_CPU_INT16(ret, val) (ret = val)

#define BE_TO_CPU_INT32(ret, val) (ret = val)

#define BE_TO_CPU_FLOAT(ret, val) (ret = val)

#define BE_TO_CPU_DOUBLE(ret, val) (ret = val)

#else

#define BE_TO_CPU_INT16(ret, val) (ret = bswap_16(val))

#define BE_TO_CPU_INT32(ret, val) (ret = bswap_32(val))

#define BE_TO_CPU_FLOAT(ret, val) SWAP_FLOAT(ret, val)

#define BE_TO_CPU_DOUBLE(ret, val) (ret = bswap_double(d))

#define LE_TO_CPU_INT16(ret, val) (ret = val)

#define LE_TO_CPU_INT32(ret, val) (ret = val)

#define LE_TO_CPU_FLOAT(ret, val) (ret = val)

#define LE_TO_CPU_DOUBLE(ret, val) (ret = val)

#endif

#endif // _CELUTIL_BYTES_H_
