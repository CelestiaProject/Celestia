// tokenizer.cpp
//
// Copyright (C) 2001-2021, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "celutil/utf8.h"

#include "tokenizer.h"

namespace
{
enum class State
{
    Start,
    Number,
    Fraction,
    ExponentStart,
    Exponent,
    Name,
    String,
    StringEscape,
    UnicodeEscape,
    Comment,
};

void reportError(const char* message)
{
    std::cerr << message << '\n';
}

bool isSeparator(char c)
{
    return !std::isdigit(c) && !std::isalpha(c) && c != '.';
}
}


Tokenizer::Tokenizer(std::istream* _in) :
    in(_in)
{
}


Tokenizer::TokenType Tokenizer::nextToken()
{
    if (isPushedBack)
    {
        isPushedBack = false;
        return tokenType;
    }

    textToken.clear();
    tokenValue = std::nan("");
    State state = State::Start;
    TokenType newToken = TokenBegin;
    int unicodeDigits = 0;
    char unicode[5] = {};

    while (newToken == TokenBegin)
    {
        bool isEof = false;
        if (reprocess)
        {
            reprocess = false;
        }
        else
        {
            in->get(nextChar);
            if (in->eof())
            {
                isEof = true;
            }
            else if (in->fail())
            {
                reportError("Unexpected error reading stream");
                newToken = TokenError;
                break;
            }
            else if (nextChar == '\n')
            {
                ++lineNumber;
            }
        }

        switch (state)
        {
        case State::Start:
            if (isEof)
            {
                newToken = TokenEnd;
            }
            else if (std::isspace(nextChar))
            {
                // no-op
            }
            else if (std::isdigit(nextChar) || nextChar == '-')
            {
                textToken.push_back(nextChar);
                state = State::Number;
            }
            else if (nextChar == '+')
            {
                state = State::Number;
            }
            else if (nextChar == '.')
            {
                textToken.append("0.");
                state = State::Fraction;
            }
            else if (std::isalpha(nextChar) || nextChar == '_')
            {
                textToken.push_back(nextChar);
                state = State::Name;
            }
            else if (nextChar == '"')
            {
                state = State::String;
            }
            else if (nextChar == '#')
            {
                state = State::Comment;
            }
            else if (nextChar == '{')
            {
                newToken = TokenBeginGroup;
            }
            else if (nextChar == '}')
            {
                newToken = TokenEndGroup;
            }
            else if (nextChar == '[')
            {
                newToken = TokenBeginArray;
            }
            else if (nextChar == ']')
            {
                newToken = TokenEndArray;
            }
            else if (nextChar == '=')
            {
                newToken = TokenEquals;
            }
            else if (nextChar == '|')
            {
                newToken = TokenBar;
            }
            else if (nextChar == '<')
            {
                newToken = TokenBeginUnits;
            }
            else if (nextChar == '>')
            {
                newToken = TokenEndUnits;
            }
            else
            {
                reportError("Bad character in stream");
                newToken = TokenError;
            }
            break;

        case State::Number:
            if (isEof)
            {
                newToken = TokenNumber;
            }
            else if (std::isdigit(nextChar))
            {
                textToken.push_back(nextChar);
            }
            else if (nextChar == '.')
            {
                textToken.push_back(nextChar);
                state = State::Fraction;
            }
            else if (nextChar == 'e' || nextChar == 'E')
            {
                textToken.push_back('e');
                state = State::ExponentStart;
            }
            else if (isSeparator(nextChar))
            {
                newToken = TokenNumber;
                reprocess = true;
            }
            else
            {
                reportError("Bad character in number");
                newToken = TokenError;
            }
            break;

        case State::Fraction:
            if (isEof)
            {
                newToken = TokenNumber;
            }
            else if (std::isdigit(nextChar))
            {
                textToken.push_back(nextChar);
            }
            else if (nextChar == 'e' || nextChar == 'E')
            {
                textToken.push_back('e');
                state = State::ExponentStart;
            }
            else if (isSeparator(nextChar))
            {
                newToken = TokenNumber;
                reprocess = true;
            }
            else
            {
                reportError("Bad character in number");
                newToken = TokenError;
            }
            break;

        case State::ExponentStart:
            if (isEof)
            {
                reportError("Unexpected EOF");
                newToken = TokenError;
            }
            else if (std::isdigit(nextChar) || nextChar == '+' || nextChar == '-')
            {
                textToken.push_back(nextChar);
                state = State::Exponent;
            }
            else
            {
                reportError("Bad character in number");
                newToken = TokenError;
            }
            break;

        case State::Exponent:
            if (isEof)
            {
                newToken = TokenNumber;
            }
            else if (std::isdigit(nextChar))
            {
                textToken.push_back(nextChar);
            }
            else if (isSeparator(nextChar))
            {
                newToken = TokenNumber;
                reprocess = true;
            }
            else
            {
                reportError("Bad character in number");
                newToken = TokenError;
            }
            break;

        case State::Name:
            if (isEof)
            {
                newToken = TokenName;
            }
            else if (std::isalpha(nextChar) || std::isdigit(nextChar) || nextChar == '_')
            {
                textToken.push_back(nextChar);
            }
            else
            {
                newToken = TokenName;
                reprocess = true;
            }
            break;

        case State::String:
            if (isEof)
            {
                reportError("Unterminated string");
                newToken = TokenError;
            }
            else if (nextChar == '\\')
            {
                state = State::StringEscape;
            }
            else if (nextChar == '"')
            {
                newToken = TokenString;
            }
            else
            {
                textToken.push_back(nextChar);
            }
            break;

        case State::StringEscape:
            if (isEof)
            {
                reportError("Unterminated string");
                newToken = TokenError;
            }
            else if (nextChar == '\\')
            {
                textToken.push_back('\\');
                state = State::String;
            }
            else if (nextChar == 'n')
            {
                textToken.push_back('\n');
                state = State::String;
            }
            else if (nextChar == '"')
            {
                textToken.push_back('"');
                state = State::String;
            }
            else if (nextChar == 'u')
            {
                state = State::UnicodeEscape;
                unicodeDigits = 0;
            }
            break;

        case State::UnicodeEscape:
            if (isEof)
            {
                reportError("Unterminated string");
                newToken = TokenError;
            }
            else if (std::isxdigit(nextChar))
            {
                unicode[unicodeDigits++] = nextChar;
                if (unicodeDigits == 4)
                {
                    auto unicodeValue = static_cast<std::uint32_t>(std::strtoul(unicode, nullptr, 16));
                    UTF8Encode(unicodeValue, textToken);
                    state = State::String;
                }
            }
            else
            {
                reportError("Bad character in Unicode escape");
                newToken = TokenError;
            }
            break;

        case State::Comment:
            if (isEof)
            {
                newToken = TokenEnd;
            }
            else if (nextChar == '\n' || nextChar == '\r')
            {
                state = State::Start;
            }
            break;
        }
    }

    if (newToken == TokenNumber)
    {
        const char* start = textToken.c_str();
        char* end;
        errno = 0;
        double value = std::strtod(start, &end);
        if (end == start)
        {
            reportError("Could not parse number");
            newToken = TokenError;
        }
        else if (errno == ERANGE)
        {
            reportError("Number out of range");
            newToken = TokenError;
            errno = 0;
        }
        else
        {
            tokenValue = value;
        }
    }

    tokenType = newToken;
    return tokenType;
}


Tokenizer::TokenType Tokenizer::getTokenType() const
{
    return tokenType;
}


void Tokenizer::pushBack()
{
    isPushedBack = true;
}


double Tokenizer::getNumberValue() const
{
    return tokenValue;
}


std::string Tokenizer::getNameValue() const
{
    return textToken;
}


std::string Tokenizer::getStringValue() const
{
    return textToken;
}


int Tokenizer::getLineNumber() const
{
    return lineNumber;
}
