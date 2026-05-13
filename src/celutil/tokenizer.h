// tokenizer.h
//
// Copyright (C) 2001-2025, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

#include <celcompat/charconv.h>
#include "buffile.h"

namespace celestia::util
{

enum class TokenType
{
    Begin,
    End,
    Error,
    BeginGroup,
    EndGroup,
    BeginArray,
    EndArray,
    BeginUnits,
    EndUnits,
    Name,
    String,
    Number,
    Equals,
    Bar,
};

class Tokenizer
{
public:
    static constexpr std::size_t DEFAULT_BUFFER_SIZE = 4096;

    explicit Tokenizer(std::istream&, std::size_t = DEFAULT_BUFFER_SIZE);

    TokenType nextToken();
    TokenType getTokenType() const noexcept { return m_tokenType; }
    void pushBack() noexcept { m_pushedBack = true; }
    std::optional<std::string_view> getNameValue() const;
    std::optional<std::string_view> getStringValue() const;

    template<typename T>
    std::optional<T> getNumberValue() const;

    std::uint64_t getLineNumber() const noexcept { return m_file.lineNumber(); }

private:
    TokenType processToken(char);
    TokenType processName();

    BufferedFile m_file;
    mutable std::string m_escaped;
    TokenType m_tokenType{ TokenType::Begin };
    bool m_pushedBack{ false };
    bool m_needsEscape{ false };
};

template<typename T>
std::optional<T>
Tokenizer::getNumberValue() const
{
    if (m_tokenType != TokenType::Number)
        return std::nullopt;

    std::string_view value = m_file.value();
    const auto end = value.data() + value.size();
    T result;
    if (auto [ptr, ec] = compat::from_chars(value.data(), end, result);
        ec == std::errc{} && ptr == end)
    {
        return result;
    }

    return std::nullopt;
}

} // end namespace celestia::util
