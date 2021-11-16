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
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "utf8.h"
#include "tokenizer.h"

namespace
{
constexpr std::string::size_type maxTokenLength = 1024;

enum class State
{
    Start,
    NumberSigned,
    Number,
    Fraction,
    ExponentStart,
    ExponentSigned,
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

bool tryPushBack(std::string& s, char c)
{
    if (s.size() < maxTokenLength)
    {
        s.push_back(c);
        return true;
    }

    reportError("Token too long");
    return false;
}
}


Tokenizer::Tokenizer(std::istream* _in) :
    in(_in)
{
    textToken.reserve(maxTokenLength);
}


Tokenizer::TokenType Tokenizer::nextToken()
{
    if (isPushedBack)
    {
        isPushedBack = false;
        return tokenType;
    }

    if (isStart)
    {
        isStart = false;
        if (!skipUtf8Bom())
        {
            tokenType = TokenError;
            return tokenType;
        }
    }

    UTF8Validator validator;
    textToken.clear();
    tokenValue = std::nan("");
    State state = State::Start;
    TokenType newToken = TokenBegin;
    int unicodeDigits = 0;
    char unicode[5] = {};
    bool isInvalidUtf8 = false;

    while (newToken == TokenBegin)
    {
        bool isEof = false;
        if (reprocess)
        {
            reprocess = false;
        }
        else
        {
            isInvalidUtf8 = false;
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
            else if (!validator.check(nextChar))
            {
                // avoid spamming the log with UTF-8 errors
                if (!hasUtf8Errors)
                {
                    reportError("Invalid UTF-8 sequence detected");
                    hasUtf8Errors = true;
                }
                isInvalidUtf8 = true;
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
            else if (std::isdigit(nextChar))
            {
                textToken.push_back(nextChar);
                state = State::Number;
            }
            else if (nextChar == '-')
            {
                textToken.push_back(nextChar);
                state = State::NumberSigned;
            }
            else if (nextChar == '+')
            {
                state = State::NumberSigned;
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

        case State::NumberSigned:
            if (isEof)
            {
                reportError("Unexpected EOF in number");
            }
            else if (std::isdigit(nextChar))
            {
                if (!tryPushBack(textToken, nextChar)) { newToken = TokenError; }
                state = State::Number;
            }
            else if (nextChar == '.')
            {
                if (textToken.size() + 2 <= maxTokenLength)
                {
                    textToken.append("0.");
                    state = State::Fraction;
                }
                else
                {
                    reportError("Token too long");
                    newToken = TokenError;
                }
            }
            else
            {
                reportError("Bad character in number");
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
                if (!tryPushBack(textToken, nextChar)) { newToken = TokenError; }
            }
            else if (nextChar == '.')
            {
                if (!tryPushBack(textToken, '.')) { newToken = TokenError; }
                state = State::Fraction;
            }
            else if (nextChar == 'e' || nextChar == 'E')
            {
                if (!tryPushBack(textToken, 'e')) { newToken = TokenError; }
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
                if (!tryPushBack(textToken, nextChar)) { newToken = TokenError; }
            }
            else if (nextChar == 'e' || nextChar == 'E')
            {
                if (!tryPushBack(textToken, 'e')) { newToken = TokenError; }
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
                reportError("Unexpected EOF in number");
                newToken = TokenError;
            }
            else if (std::isdigit(nextChar))
            {
                if (!tryPushBack(textToken, nextChar)) { newToken = TokenError; }
                state = State::Exponent;
            }
            else if (nextChar == '+' || nextChar == '-')
            {
                if (!tryPushBack(textToken, nextChar)) { newToken = TokenError; }
                state = State::ExponentSigned;
            }
            else
            {
                reportError("Bad character in number");
                newToken = TokenError;
            }
            break;

        case State::ExponentSigned:
            if (isEof)
            {
                reportError("Unexpected EOF in number");
                newToken = TokenError;
            }
            else if (std::isdigit(nextChar))
            {
                if (!tryPushBack(textToken, nextChar)) { newToken = TokenError; }
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
                if (!tryPushBack(textToken, nextChar)) { newToken = TokenError; }
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
                if (!tryPushBack(textToken, nextChar)) { newToken = TokenError; }
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
                reportError("Unexpected EOF in string");
                newToken = TokenError;
            }
            else if (isInvalidUtf8)
            {
                if (textToken.size() + std::strlen(UTF8_REPLACEMENT_CHAR) <= maxTokenLength)
                {
                    textToken.append(UTF8_REPLACEMENT_CHAR);
                }
                else
                {
                    reportError("Token too long");
                    newToken = TokenError;
                }
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
                if (!tryPushBack(textToken, nextChar)) { newToken = TokenError; }
            }
            break;

        case State::StringEscape:
            if (isEof)
            {
                reportError("Unexpected EOF in string");
                newToken = TokenError;
            }
            else if (nextChar == '\\')
            {
                if (!tryPushBack(textToken, '\\')) { newToken = TokenError; }
                state = State::String;
            }
            else if (nextChar == 'n')
            {
                if (!tryPushBack(textToken, '\n')) { newToken = TokenError; }
                state = State::String;
            }
            else if (nextChar == '"')
            {
                if (!tryPushBack(textToken, '"')) { newToken = TokenError; }
                state = State::String;
            }
            else if (nextChar == 'u')
            {
                state = State::UnicodeEscape;
                unicodeDigits = 0;
            }
            else
            {
                reportError("Invalid string escape sequence");
                newToken = TokenError;
            }
            break;

        case State::UnicodeEscape:
            if (isEof)
            {
                reportError("Unexpected EOF in string");
                newToken = TokenError;
            }
            else if (std::isxdigit(nextChar))
            {
                unicode[unicodeDigits++] = nextChar;
                if (unicodeDigits == 4)
                {
                    auto unicodeValue = static_cast<std::uint32_t>(std::strtoul(unicode, nullptr, 16));
                    if (textToken.size() + UTF8EncodedSize(unicodeValue) <= maxTokenLength)
                    {
                        UTF8Encode(unicodeValue, textToken);
                        state = State::String;
                    }
                    else
                    {
                        reportError("Token too long");
                        newToken = TokenError;
                    }
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
    return tokenType == TokenNumber ? tokenValue : std::nan("");
}


bool Tokenizer::isInteger() const
{
    return tokenType == TokenNumber
        && textToken.find_first_of(".eE") == std::string::npos
        && tokenValue >= INT32_MIN && tokenValue <= INT32_MAX;
}


std::int32_t Tokenizer::getIntegerValue() const
{
    return static_cast<std::int32_t>(tokenValue);
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


bool Tokenizer::skipUtf8Bom()
{
    for (int i = 0; i < 3; ++i)
    {
        in->get(nextChar);
        if (in->eof())
        {
            if (i == 0)
            {
                return true;
            }

            reportError("Incomplete UTF-8 sequence");
            return false;
        }
        else if (in->fail())
        {
            reportError("Unexpected error reading stream");
            return false;
        }
        else if (i == 0)
        {
            if (nextChar != '\357')
            {
                lineNumber += nextChar == '\n' ? 1 : 0;
                reprocess = true;
                return true;
            }
        }
        else if ((i == 1 && nextChar != '\273') || (i == 2 && nextChar != '\277'))
        {
            reportError("Bad character in stream");
            return false;
        }
    }

    return true;
}
