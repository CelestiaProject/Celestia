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
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "logger.h"
#include "utf8.h"
#include "tokenizer.h"

using celestia::util::GetLogger;

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

    GetLogger()->error("Token too long\n");
    return false;
}


bool handleUtf8Error(std::string& s, UTF8Status status)
{
    if (status == UTF8Status::InvalidTrailingByte)
    {
        // remove the partial UTF-8 sequence
        std::string::size_type pos = s.size();
        while (pos > 0)
        {
            unsigned char u = static_cast<unsigned char>(s[--pos]);
            if (u >= 0xc0) { break; }
        }

        s.resize(pos);
    }

    if (s.size() <= maxTokenLength - std::strlen(UTF8_REPLACEMENT_CHAR))
    {
        s.append(UTF8_REPLACEMENT_CHAR);
        return true;
    }

    GetLogger()->error("Token too long\n");
    return false;
}
} // end unnamed namespace


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
    UTF8Status utf8Status = UTF8Status::Ok;

    while (newToken == TokenBegin)
    {
        bool isEof = false;
        if (reprocess)
        {
            reprocess = false;
        }
        else
        {
            utf8Status = UTF8Status::Ok;
            in->get(nextChar);
            if (in->eof())
            {
                isEof = true;
            }
            else if (in->fail())
            {
                GetLogger()->error("Unexpected error reading stream\n");
                newToken = TokenError;
                break;
            }
            else
            {
                utf8Status = validator.check(nextChar);
                if (utf8Status != UTF8Status::Ok && !hasUtf8Errors)
                {
                    GetLogger()->error("Invalid UTF-8 sequence detected\n");
                    hasUtf8Errors = true;
                }
                else if (nextChar == '\n')
                {
                    ++lineNumber;
                }
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
                GetLogger()->error("Bad character in stream\n");
                newToken = TokenError;
            }
            break;

        case State::NumberSigned:
            if (isEof)
            {
                GetLogger()->error("Unexpected EOF in number\n");
                newToken = TokenError;
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
                    GetLogger()->error("Token too long\n");
                    newToken = TokenError;
                }
            }
            else
            {
                GetLogger()->error("Bad character in number\n");
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
                GetLogger()->error("Bad character in number\n");
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
                GetLogger()->error("Bad character in number\n");
                newToken = TokenError;
            }
            break;

        case State::ExponentStart:
            if (isEof)
            {
                GetLogger()->error("Unexpected EOF in number\n");
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
                GetLogger()->error("Bad character in number\n");
                newToken = TokenError;
            }
            break;

        case State::ExponentSigned:
            if (isEof)
            {
                GetLogger()->error("Unexpected EOF in number\n");
                newToken = TokenError;
            }
            else if (std::isdigit(nextChar))
            {
                if (!tryPushBack(textToken, nextChar)) { newToken = TokenError; }
                state = State::Exponent;
            }
            else
            {
                GetLogger()->error("Bad character in number\n");
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
                GetLogger()->error("Bad character in number\n");
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
                GetLogger()->error("Unexpected EOF in string\n");
                newToken = TokenError;
            }
            else if (utf8Status != UTF8Status::Ok)
            {
                if (!handleUtf8Error(textToken, utf8Status)) { newToken = TokenError; }
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
                GetLogger()->error("Unexpected EOF in string\n");
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
                GetLogger()->error("Invalid string escape sequence\n");
                newToken = TokenError;
            }
            break;

        case State::UnicodeEscape:
            if (isEof)
            {
                GetLogger()->error("Unexpected EOF in string\n");
                newToken = TokenError;
            }
            else if (std::isxdigit(nextChar))
            {
                unicode[unicodeDigits++] = nextChar;
                if (unicodeDigits == 4)
                {
                    auto unicodeValue = static_cast<std::uint32_t>(std::strtoul(unicode, nullptr, 16));
                    if (textToken.size() <= maxTokenLength - UTF8EncodedSizeChecked(unicodeValue))
                    {
                        UTF8Encode(unicodeValue, textToken);
                        state = State::String;
                    }
                    else
                    {
                        GetLogger()->error("Token too long\n");
                        newToken = TokenError;
                    }
                }
            }
            else
            {
                GetLogger()->error("Bad character in Unicode escape\n");
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
            GetLogger()->error("Could not parse number\n");
            newToken = TokenError;
        }
        else if (errno == ERANGE)
        {
            GetLogger()->error("Number out of range\n");
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

            GetLogger()->error("Incomplete UTF-8 sequence\n");
            return false;
        }
        else if (in->fail())
        {
            GetLogger()->error("Unexpected error reading stream\n");
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
            GetLogger()->error("Bad character in stream\n");
            return false;
        }
    }

    return true;
}
