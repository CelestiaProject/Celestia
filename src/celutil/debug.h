// debug.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#ifdef DPRINTF
#undef DPRINTF // OSX has DPRINTF
#endif // DPRINTF

#if defined(_DEBUG) || defined(DEBUG)

#ifdef _MSC_VER
#include <windows.h>
#endif
#include <iostream>
#include <fmt/printf.h>
#include <fmt/ostream.h>

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
            std::cerr << fmt::sprintf(format, args...);
        }
#else
        std::cerr << fmt::sprintf(format, args...);
#endif
    }
}
#else
#define DPRINTF(level, ...) static_cast<void>(level);
#endif /* DEBUG */

// verbosity choices
#define LOG_LEVEL_ERROR           0
#define LOG_LEVEL_WARNING         1
#define LOG_LEVEL_INFO            2
#define LOG_LEVEL_VERBOSE         3
#define LOG_LEVEL_DEBUG           4

extern void SetDebugVerbosity(int);
extern int GetDebugVerbosity();
