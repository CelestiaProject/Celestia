// tokenizer.h
//
// Copyright (C) 2001-2009, the Celestia Development Team
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
#include <memory>
#include <optional>
#include <string_view>


class TokenizerImpl;

class Tokenizer
{
public:
    enum TokenType
    {
        TokenName           = 0,
        TokenString         = 1,
        TokenNumber         = 2,
        TokenBegin          = 3,
        TokenEnd            = 4,
        TokenNull           = 5,
        TokenBeginGroup     = 6,
        TokenEndGroup       = 7,
        TokenBeginArray     = 8,
        TokenEndArray       = 9,
        TokenEquals         = 10,
        TokenError          = 11,
        TokenBar            = 12,
        TokenBeginUnits     = 13,
        TokenEndUnits       = 14,
    };

    static constexpr std::size_t DEFAULT_BUFFER_SIZE = 4096;

    Tokenizer(std::istream*, std::size_t = DEFAULT_BUFFER_SIZE);
    ~Tokenizer();

    TokenType nextToken();
    TokenType getTokenType() const;
    void pushBack();
    std::optional<double> getNumberValue() const;
    std::optional<std::int32_t> getIntegerValue() const;
    std::optional<std::string_view> getNameValue() const;
    std::optional<std::string_view> getStringValue() const;

    int getLineNumber() const;

private:
    std::unique_ptr<TokenizerImpl> impl;
    TokenType tokenType{ TokenType::TokenBegin };
    bool isPushedBack{ false };
};
