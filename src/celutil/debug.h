// debug.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef _MSC_VER
#include <windows.h>
#endif
#include <iostream>
#include <fmt/printf.h>

#ifdef DPRINTF
#undef DPRINTF // OSX has DPRINTF
#endif // DPRINTF

#if !defined(_DEBUG) && !defined(DEBUG)
#define DPRINTF(level, format, ...)
#else
extern int debugVerbosity;
template <typename... T>
void DPRINTF(int level, const char *format, const T & ... args)
{
    if (level <= debugVerbosity)
    {
#ifdef _MSC_VER
        if (IsDebuggerPresent())
        {
            OutputDebugStringA(fmt::sprintf(format, args...).c_str());
        }
        else
        {
            fmt::fprintf(std::cerr, format, args...);
        }
#else
        fmt::fprintf(std::cerr, format, args...);
#endif
    }
}
#endif /* DEBUG */

extern void SetDebugVerbosity(int);
extern int GetDebugVerbosity();

#endif // _DEBUG_H_
