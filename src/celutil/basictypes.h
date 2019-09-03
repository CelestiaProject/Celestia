// basictypes.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _BASICTYPES_H_
#define _BASICTYPES_H_

#include <stdint.h>

typedef unsigned int   uint;

// Fixed size types
typedef int32_t        int32;
typedef uint32_t       uint32;
typedef int16_t        int16;
typedef uint16_t       uint16;
typedef int8_t         int8;
typedef uint8_t        uint8;
typedef int64_t        int64;
typedef uint64_t       uint64;

#ifndef INT64_MAX
#define INT64_MAX 9223372036854775807LL
#endif
#ifndef UINT64_MAX
#define UINT64_MAX 0xffffffffffffffffULL
#endif

#endif // _BASICTYPES_H_

