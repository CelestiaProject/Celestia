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

typedef unsigned int   uint;

// Fixed size types
typedef int            int32;
typedef unsigned int   uint32;
typedef short          int16;
typedef unsigned short uint16;
typedef char           int8;
typedef unsigned char  uint8;

#ifdef _MSC_VER
// MS Visual C++ does not include stdint.h
typedef __int64          int64;
typedef unsigned __int64 uint64;
#define INT64_MAX  LLONG_MAX
#define UINT64_MAX ULLONG_MAX
#else
#include <stdint.h>
#include <limits>
typedef          int64_t int64;
typedef         uint64_t uint64;
#define INT64_MAX 9223372036854775807LL
#ifndef UINT64_MAX
#define UINT64_MAX 0xffffffffffffffffULL
#endif
//#define INT64_MAX  std::numeric_limits<int64_t>::max()
//#define UINT64_MAX std::numeric_limits<uint64_t>::max()
#endif

#endif // _BASICTYPES_H_

