// formatnum.h
//
// Copyright (C) 2023, the Celestia Development Team
// Rewritten from the original version:
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cassert>
#include <iterator>
#ifndef USE_ICU
#include <locale>
#endif
#include <string_view>
#include <type_traits>

#include <fmt/format.h>

#include <celutil/flag.h>

namespace celestia::util
{

enum class NumberFormat : unsigned int
{
    None = 0,
    GroupThousands = 1,
    SignificantFigures = 2,
};

ENUM_CLASS_BITWISE_OPS(NumberFormat)

class NumberFormatter;

template<typename T>
class FormattedFloat
{
private:
    FormattedFloat(const NumberFormatter* formatter, T value, unsigned int precision, NumberFormat format) :
        m_formatter(formatter),
        m_value(value),
        m_precision(precision),
        m_format(format)
    {
    }

    fmt::format_context::iterator format(fmt::format_context& ctx) const;

    const NumberFormatter* m_formatter;
    T m_value;
    unsigned int m_precision;
    NumberFormat m_format;

    friend NumberFormatter;
    friend fmt::formatter<FormattedFloat<T>>;
};

class NumberFormatter
{
public:
#ifdef USE_ICU
    NumberFormatter();
#else
    explicit NumberFormatter(const std::locale& loc);
#endif

    template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
    inline FormattedFloat<T> format(T value,
                                    unsigned int precision,
                                    NumberFormat format = NumberFormat::GroupThousands) const
    {
        if (is_set(format, NumberFormat::SignificantFigures) && precision > 0)
            --precision;
        return FormattedFloat<T>(this, value, precision, format);
    }

private:
    fmt::format_context::iterator format_fixed(fmt::format_context::iterator, std::string_view, NumberFormat) const;
    fmt::format_context::iterator format_sigfigs(fmt::format_context::iterator, fmt::memory_buffer&, NumberFormat) const;
    fmt::format_context::iterator format_grouped(fmt::format_context::iterator,
                                                 std::string_view,
                                                 std::string_view::size_type = std::string_view::npos) const;

    std::string m_decimal{ "." };
    std::string m_thousands;
    std::string m_grouping;

    template<typename T>
    friend class FormattedFloat;
};

template<typename T>
fmt::format_context::iterator
FormattedFloat<T>::format(fmt::format_context& ctx) const
{
    fmt::memory_buffer buffer;
    if (celestia::util::is_set(m_format, celestia::util::NumberFormat::SignificantFigures))
    {
        fmt::format_to(std::back_inserter(buffer), "{:.{}e}", m_value, m_precision);
        return m_formatter->format_sigfigs(ctx.out(), buffer, m_format);
    }

    format_to(std::back_inserter(buffer), "{:.{}f}", m_value, m_precision);
    return m_formatter->format_fixed(ctx.out(),
                                     std::string_view{ buffer.data(), buffer.size() },
                                     m_format);
}

} // end namespace celestia::util

template<typename T>
struct fmt::formatter<celestia::util::FormattedFloat<T>>
{
    constexpr auto parse(const format_parse_context& ctx) const
    {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}')
        {
#if FMT_VERSION >= 110000
            report_error("invalid format");
#elif FMT_VERSION >= 100100
            throw_format_error("invalid format");
#else
            assert(0);
            return ctx.end();
#endif
        }

        return it;
    }

    auto format(const celestia::util::FormattedFloat<T>& f, format_context& ctx) const
    {
        return f.format(ctx);
    }
};
