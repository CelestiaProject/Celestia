// formatnum.cpp
//
// Copyright (C) 2023, the Celestia Development Team
// Rewritten from the original version:
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "formatnum.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <system_error>

#include <celcompat/charconv.h>
#include "utf8.h"

#ifdef USE_ICU
#include <celutil/includeicu.h>
#include <celutil/uniquedel.h>
#endif

using namespace std::string_view_literals;

namespace
{

struct ExtendedSubstring
{
    ExtendedSubstring(std::string_view, std::string_view::size_type, std::string_view::size_type);

    std::string_view source;
    std::string_view::size_type start;
    std::string_view::size_type length;
};

ExtendedSubstring::ExtendedSubstring(std::string_view _source,
                                     std::string_view::size_type _start,
                                     std::string_view::size_type _length) :
    source(_source),
    start(_start),
    length(_length)
{
}

} // end unnamed namespace

template<>
struct fmt::formatter<ExtendedSubstring>
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

    auto format(const ExtendedSubstring& value, format_context& ctx) const
    {
        if (value.start >= value.source.size())
            return format_to(ctx.out(), "{:0<{}}", ""sv, value.length);

        if (value.source.size() - value.start >= value.length)
            return format_to(ctx.out(), "{}", value.source.substr(value.start, value.length));

        return format_to(ctx.out(), "{:0<{}}", value.source.substr(value.start), value.length);
    }
};

namespace celestia::util
{

namespace
{

inline fmt::format_context::iterator
checkSign(fmt::format_context::iterator out, std::string_view& source)
{
    if (source.front() == '-')
    {
        source = source.substr(1);
        return fmt::format_to(out, "-");
    }

    return out;
}

struct ParseExpResult
{
    std::string_view digits;
    int exponent{ 0 };
    bool isNegative{ false };
};

bool
parseExponential(fmt::memory_buffer& buffer, ParseExpResult& result)
{
    const auto begin = buffer.begin();
    const auto end = buffer.end();
    if (begin == end)
        return false;

    auto start = begin;
    if (*start == '-')
    {
        result.isNegative = true;
        ++start;
    }

    auto exponent = std::find(start, end, 'e');
    if (exponent == end)
        return false;

    if (const auto decimal = std::find(start, exponent, '.'); decimal != exponent)
    {
        std::copy_backward(start, decimal, decimal + 1);
        ++start;
    }

    result.digits = std::string_view{ buffer.data() + (start - begin), static_cast<std::size_t>(exponent - start) };
    if (result.digits.empty())
        return false;

    ++exponent;
    std::string_view exponentSv{ buffer.data() + (exponent - begin), static_cast<std::size_t>(end - exponent) };
    if (exponentSv.empty())
        return false;

    if (exponentSv.front() == '+')
    {
        exponentSv = exponentSv.substr(1);
        if (exponentSv.empty())
            return false;
    }

    const auto exponentEnd = exponentSv.data() + exponentSv.size();
    auto [ptr, ec] = compat::from_chars(exponentSv.data(), exponentEnd, result.exponent);
    return ec == std::errc{} && ptr == exponentEnd;
}

#ifdef USE_ICU
bool getNumberSymbol(const UNumberFormat *numFormat, UNumberFormatSymbol symbol, std::string &output)
{
    UErrorCode status = U_ZERO_ERROR;
    auto requiredSize = unum_getSymbol(numFormat, symbol, nullptr, 0, &status);
    if (U_FAILURE(status) && status != U_BUFFER_OVERFLOW_ERROR)
        return false;

    status = U_ZERO_ERROR;
    std::u16string symbolString;
    symbolString.resize(requiredSize);
    unum_getSymbol(numFormat, symbol, symbolString.data(), requiredSize, &status);

    if (U_FAILURE(status))
        return false;

    u_strToUTF8(nullptr, 0, &requiredSize, symbolString.data(), symbolString.size(), &status);
    if (U_FAILURE(status) && status != U_BUFFER_OVERFLOW_ERROR)
        return false;

    status = U_ZERO_ERROR;
    output.resize(requiredSize);

    u_strToUTF8(output.data(), requiredSize, nullptr, symbolString.data(), symbolString.size(), &status);
    return U_SUCCESS(status);
}
#endif

} // end unnamed namespace

#ifdef USE_ICU
NumberFormatter::NumberFormatter()
{
    using UniqueNumberFormat = UniquePtrDel<UNumberFormat, unum_close>;

    UErrorCode status = U_ZERO_ERROR;
    UniqueNumberFormat numFormat(unum_open(UNUM_DECIMAL, nullptr, 0, nullptr, nullptr, &status));
    if (U_FAILURE(status))
        return;

    getNumberSymbol(numFormat.get(), UNUM_DECIMAL_SEPARATOR_SYMBOL, m_decimal);

    // attribute == -1 means the attribute is missing
    std::int32_t groupingUsed = unum_getAttribute(numFormat.get(), UNUM_GROUPING_USED);
    std::int32_t groupingSize = unum_getAttribute(numFormat.get(), UNUM_GROUPING_SIZE);
    if (groupingUsed != -1 && groupingUsed != 0 &&
        groupingSize != -1 && groupingSize != 0 &&
        getNumberSymbol(numFormat.get(), UNUM_GROUPING_SEPARATOR_SYMBOL, m_thousands))
    {
        m_grouping.push_back(static_cast<char>(groupingSize));

        std::int32_t secondaryGroupingSize = unum_getAttribute(numFormat.get(), UNUM_SECONDARY_GROUPING_SIZE);
        if (secondaryGroupingSize != -1 && secondaryGroupingSize != 0)
            m_grouping.push_back(static_cast<char>(secondaryGroupingSize));
    }
}
#else
NumberFormatter::NumberFormatter(const std::locale& loc)
{
    if (!std::has_facet<std::numpunct<wchar_t>>(loc))
        return;

    const auto& facet = std::use_facet<std::numpunct<wchar_t>>(loc);
    if (auto dp = facet.decimal_point(); dp != L'\0')
    {
        m_decimal.clear();
        UTF8Encode(static_cast<std::uint32_t>(dp), m_decimal);
    }

    m_grouping = facet.grouping();
    if (m_grouping.empty())
        return;

    if (auto sep = facet.thousands_sep(); sep != L'\0')
        UTF8Encode(static_cast<std::uint32_t>(sep), m_thousands);
    else
        m_grouping.clear();
}
#endif

fmt::format_context::iterator
NumberFormatter::format_fixed(fmt::format_context::iterator out,
                              std::string_view source,
                              NumberFormat format) const
{
    auto pointPos = source.find('.');
    auto intPart = pointPos == std::string_view::npos
        ? source
        : source.substr(0, pointPos);

    if (intPart.empty())
        return out;

    out = checkSign(out, intPart);

    out = is_set(format, NumberFormat::GroupThousands)
        ? format_grouped(out, intPart)
        : fmt::format_to(out, "{}", intPart);

    if (pointPos == std::string_view::npos)
        return out;

    return fmt::format_to(out, "{}{}", m_decimal, source.substr(pointPos + 1));
}

fmt::format_context::iterator
NumberFormatter::format_sigfigs(fmt::format_context::iterator out,
                                fmt::memory_buffer& buffer,
                                NumberFormat format) const
{
    ParseExpResult expData;
    if (!parseExponential(buffer, expData))
        return out;

    if (expData.isNegative)
        out = fmt::format_to(out, "-");

    // Case 1: value is less than zero
    if (expData.exponent < 0)
        return fmt::format_to(out, "0{}{:0<{}}{}", m_decimal, ""sv, -1 - expData.exponent, expData.digits);

    // Case 2: decimal point is within or at the end of the digits sequence
    auto decimalPos = static_cast<std::size_t>(expData.exponent + 1);
    if (decimalPos < expData.digits.size())
    {
        std::string_view intPart = expData.digits.substr(0, decimalPos);
        out = is_set(format, NumberFormat::GroupThousands)
            ? format_grouped(out, intPart)
            : fmt::format_to(out, "{}", intPart);
        return fmt::format_to(out, "{}{}", m_decimal, expData.digits.substr(decimalPos));
    }

    // Case 3: need to insert trailing zeros in the integer part

    return is_set(format, NumberFormat::GroupThousands)
        ? format_grouped(out, expData.digits, decimalPos)
        : fmt::format_to(out, "{:0<{}}", expData.digits, decimalPos);
}

fmt::format_context::iterator
NumberFormatter::format_grouped(fmt::format_context::iterator out,
                                std::string_view source,
                                std::string_view::size_type size) const
{
    if (size == std::string_view::npos)
        size = source.size();

    if (m_grouping.empty() || size <= static_cast<unsigned char>(m_grouping.front()))
        return fmt::format_to(out, "{}", ExtendedSubstring(source, 0, size));

    auto pos = size - static_cast<unsigned char>(m_grouping.front());
    std::size_t groupIndex = 1;
    while (groupIndex < m_grouping.size() && pos > static_cast<unsigned char>(m_grouping[groupIndex]))
    {
        pos -= static_cast<unsigned char>(m_grouping[groupIndex]);
        ++groupIndex;
    }

    // last group repeats indefinitely
    if (auto lastGroup = static_cast<unsigned char>(m_grouping.back());
        groupIndex == m_grouping.size() && pos > lastGroup)
    {
        auto offset = pos % lastGroup;
        if (offset == 0)
            offset += lastGroup;
        out = fmt::format_to(out, "{}", ExtendedSubstring(source, 0, offset));

        while (offset < pos)
        {
            out = fmt::format_to(out, "{}{}", m_thousands, ExtendedSubstring(source, offset, lastGroup));
            offset += lastGroup;
        }
    }
    else
    {
        out = fmt::format_to(out, "{}", ExtendedSubstring(source, 0, pos));
    }

    do
    {
        --groupIndex;
        auto groupSize = static_cast<unsigned char>(m_grouping[groupIndex]);
        out = fmt::format_to(out, "{}{}", m_thousands, ExtendedSubstring(source, pos, groupSize));
        pos += groupSize;
    } while (groupIndex > 0);

    return out;
}

} // end namespace celestia::util
