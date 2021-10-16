// tokenizer.h
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _TOKENIZER_H_
#define _TOKENIZER_H_

#include <cmath>
#include <iosfwd>
#include <string>

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

    Tokenizer(std::istream*);

    TokenType nextToken();
    TokenType getTokenType() const;
    void pushBack();
    double getNumberValue() const;
    std::string getNameValue() const;
    std::string getStringValue() const;

    int getLineNumber() const;

private:
    std::istream* in;
    TokenType tokenType{ TokenBegin };
    bool isStart{ true };
    bool isPushedBack{ false };
    std::string textToken{};
    double tokenValue{ std::nan("") };
    int lineNumber{ 1 };
    char nextChar{ '\0' };
    bool reprocess{ false };

    bool skipUtf8Bom();
};

#endif // _TOKENIZER_H_
