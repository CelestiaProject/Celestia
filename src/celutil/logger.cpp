// logger.cpp
//
// Copyright (C) 2021, Celestia Development Team
//
// Logging functions.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstdlib>
#include <iostream>
#include <memory>

#ifdef _MSC_VER
#    include <windows.h>
#endif
#include "logger.h"

#include <fmt/ostream.h>

namespace celestia::util
{

namespace
{
std::unique_ptr<Logger> globalLogger = nullptr;
}

Logger *
GetLogger()
{
    return globalLogger.get();
}

Logger *
CreateLogger(Level level)
{
    return CreateLogger(level, std::clog, std::cerr);
}

Logger *
CreateLogger(Level level, Logger::Stream &log, Logger::Stream &err)
{
    if (globalLogger == nullptr)
    {
        globalLogger = std::make_unique<Logger>(level, log, err);
        std::atexit(DestroyLogger);
    }
    return globalLogger.get();
}

void
DestroyLogger()
{
    globalLogger = nullptr;
}

Logger::Logger() :
    Logger(Level::Info, std::clog, std::cerr)
{
}

Logger::Logger(Level level, Stream &log, Stream &err) :
    m_log(log),
    m_err(err),
    m_level(level)
{
}

void
Logger::setLevel(Level level)
{
    m_level = level;
}

void
Logger::vlog(Level level, std::string_view format, fmt::format_args args) const
{
#ifdef _MSC_VER
    if (level == Level::Debug && IsDebuggerPresent())
    {
        OutputDebugStringA(fmt::vformat(fmt::string_view(format), args).c_str());
        return;
    }
#endif

    auto &stream = (level <= Level::Warning || level == Level::Debug) ? m_err : m_log;
    fmt::vprint(stream, fmt::string_view(format), args);
}

} // end namespace celestia::util
