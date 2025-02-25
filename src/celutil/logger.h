// logger.h
//
// Copyright (C) 2021-present, Celestia Development Team
//
// Logging functions.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <iosfwd>
#include <string_view>

#include <fmt/format.h>

#include <celcompat/filesystem.h>

#if defined(HAVE_STD_FILESYSTEM) && FMT_VERSION >= 100100
// From fmt 10.1.0 onwards, default path formatter is unquoted
#include <fmt/std.h>
#elif defined(_WIN32)
#include <celutil/winutil.h>
#endif

#if !defined(HAVE_STD_FILESYSTEM) || FMT_VERSION < 100100
template <>
struct fmt::formatter<fs::path> : formatter<std::string_view>
{
    auto format(const fs::path &path, format_context &ctx) const
    {
#ifdef _WIN32
        fmt::basic_memory_buffer<char> buffer;
        celestia::util::WideToUTF8(path.native(), buffer);
        return formatter<std::string_view>::format(std::string_view{ buffer.data(), buffer.size() }, ctx);
#else
        return formatter<std::string_view>::format(path.native(), ctx);
#endif
    }
};
#endif

namespace celestia::util
{

enum class Level
{
    Error,
    Warning,
    Info,
    Verbose,
    Debug,
};

class Logger
{
public:
    using Stream = std::basic_ostream<char>;

    /** Create a default logger with Info verbocity writing to std::clog and std::cerr */
    Logger();

    /**
     * Create a logger with custom verbocity and output streams
     * @param level verbosibility level
     * @param log stream used to output normal messages
     * @param err stream used to output errors, warnings and debug messages
     */
    Logger(Level level, Stream &log, Stream &err);

    /**
     * Set verbocity level
     *
     * @param level verbosibility level
     */
    void setLevel(Level level);

    template<typename... Args>
    void debug(std::string_view format, const Args &...args) const;

    template<typename... Args>
    void info(std::string_view format, const Args &...args) const;

    template<typename... Args>
    void verbose(std::string_view format, const Args &...args) const;

    template<typename... Args>
    void warn(std::string_view format, const Args &...args) const;

    template<typename... Args>
    void error(std::string_view format, const Args &...args) const;

    template<typename... Args>
    void log(Level, std::string_view format, const Args &...args) const;

private:
    void vlog(Level level, std::string_view format, fmt::format_args args) const;

    Stream &m_log;
    Stream &m_err;
    Level   m_level;
};

template<typename... Args> inline void
Logger::log(Level level, std::string_view format, const Args &...args) const
{
    if (level <= m_level)
        vlog(level, format, fmt::make_format_args(args...));
}

template<typename... Args> inline void
Logger::debug(std::string_view format, const Args &...args) const
{
    log(Level::Debug, format, args...);
}

template<typename... Args> inline void
Logger::info(std::string_view format, const Args &...args) const
{
    log(Level::Info, format, args...);
}

template<typename... Args> inline void
Logger::verbose(std::string_view format, const Args &...args) const
{
    log(Level::Verbose, format, args...);
}

template<typename... Args> inline void
Logger::warn(std::string_view format, const Args &...args) const
{
    log(Level::Warning, format, args...);
}

template<typename... Args> inline void
Logger::error(std::string_view format, const Args &...args) const
{
    log(Level::Error, format, args...);
}

/** Return a pointer to the default global Logger instance */
Logger *GetLogger();

/**
 * Initiliaze the default global Logger instance writing to std::clog and std::cerr
 *
 * @param level verbosibility level
 * @return pointer to the default global Logger instance
 */
Logger *CreateLogger(Level level = Level::Info);

/**
 * Initiliaze the default global Logger instance writing to custom streams
 *
 * @param level verbosibility level
 * @param log stream used to output normal messages
 * @param err stream used to output errors, warnings and debug messages
*/
Logger *CreateLogger(Level level, Logger::Stream &log, Logger::Stream &err);

/** Destroy the default global Logger instance */
void DestroyLogger();

} // end namespace celestia::util
