// debug.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifdef _MSC_VER
#include <windows.h>
#endif
#include <cstdio>
#include <cstdarg>
#include <config.h>
#include <fmt/printf.h>

static int debugVerbosity = 0;

#if defined(DEBUG) || defined(_DEBUG)
void DebugPrint(int level, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    if (level <= debugVerbosity)
    {
#ifdef _MSC_VER
        if (IsDebuggerPresent())
        {
            char buf[1024];
            fmt::vsprintf(buf, format, args);
            OutputDebugStringA(buf);
        }
        else
        {
            fmt::vfprintf(stdout, format, args);
        }
#else
        fmt::vfprintf(stderr, format, args);
#endif
    }

    va_end(args);
}
#endif /* DEBUG */


void SetDebugVerbosity(int dv)
{
    if(dv<0)
        dv=0;
    debugVerbosity = dv;
}


int GetDebugVerbosity()
{
    return debugVerbosity;
}
