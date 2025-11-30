// tokenizer.cpp
//
// Copyright (C) 2001-2025, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "tokenizer.h"

#include <cassert>

#include "utf8.h"

namespace celestia::util
{

namespace
{

constexpr bool
isAsciiDigit(char ch)
{
    return ch >= '0' && ch <= '9';
}

constexpr bool
isNumberSign(char ch)
{
    return ch == '-' || ch == '+';
}

constexpr bool
isNumberStart(char ch)
{
    return isAsciiDigit(ch) || isNumberSign(ch) || ch == '.';
}

constexpr bool
isExponentSymbol(char ch)
{
    return ch == 'E' || ch == 'e';
}

constexpr bool
isNameStart(char ch)
{
    static_assert('Z' - 'A' == 25, "EBCDIC is not supported");
    return (ch >= 'A' && ch <= 'Z') ||
           (ch >= 'a' && ch <= 'z') ||
           ch == '_';
}

constexpr bool
isNameContinuation(char ch)
{
    return isNameStart(ch) || isAsciiDigit(ch);
}

constexpr bool
isHexDigit(std::int32_t cp)
{
    return (cp >= static_cast<std::int32_t>(U'0') && cp <= static_cast<std::int32_t>(U'9')) ||
           (cp >= static_cast<std::int32_t>(U'A') && cp <= static_cast<std::int32_t>(U'F')) ||
           (cp >= static_cast<std::int32_t>(U'a') && cp <= static_cast<std::int32_t>(U'f'));
}

class NumberStateMachine
{
public:
    explicit NumberStateMachine(BufferedFile&);
    TokenType parse();

private:
    enum class State
    {
        Start,
        End,
        Error,
        IntegerSign,
        Integer,
        FractionalPoint,
        Fractional,
        ExponentSymbol,
        ExponentSign,
        Exponent,
    };

    void processStart(int);
    void processIntegerSign(int);
    void processInteger(int);
    void processFractionalPoint(int);
    void processFractional(int);
    void processExponentSymbol(int);
    void processExponentSign(int);
    void processExponent(int);

    bool isInvalidEndState() const noexcept;

    BufferedFile* m_file;
    std::size_t m_expPos{ 0 };
    State m_state{ State::Start };
    bool m_hasDigits{ false };
};

NumberStateMachine::NumberStateMachine(BufferedFile& file) :
    m_file(&file)
{
}

TokenType
NumberStateMachine::parse()
{
    while (m_state != State::End && m_state != State::Error)
    {
        int next = m_file->next();
        switch (m_state)
        {
        case State::Start:
            processStart(next);
            break;
        case State::IntegerSign:
            processIntegerSign(next);
            break;
        case State::Integer:
            processInteger(next);
            break;
        case State::FractionalPoint:
            processFractionalPoint(next);
            break;
        case State::Fractional:
            processFractional(next);
            break;
        case State::ExponentSymbol:
            processExponentSymbol(next);
            break;
        case State::ExponentSign:
            processExponentSign(next);
            break;
        case State::Exponent:
            processExponent(next);
            break;
        default:
            assert(0);
            break;
        }
    }

    return m_state == State::Error ? TokenType::Error : TokenType::Number;
}

void
NumberStateMachine::processStart(int next)
{
    if (next == std::char_traits<char>::eof())
    {
        m_state = State::Error;
        return;
    }

    auto ch = std::char_traits<char>::to_char_type(next);
    switch (ch)
    {
    case '-':
        m_state = State::IntegerSign;
        m_file->advance(false);
        break;

    case '+':
        m_state = State::IntegerSign;
        m_file->advance(true); // exclude for charconv purposes
        break;

    case '.':
        m_state = State::FractionalPoint;
        m_file->advance(false);
        break;

    default:
        if (isAsciiDigit(ch))
        {
            m_state = State::Integer;
            m_hasDigits = true;
            m_file->advance(false);
        }
        else
        {
            m_state = State::Error;
            m_file->advance(true);
        }
        break;
    }
}

void
NumberStateMachine::processIntegerSign(int next)
{
    if (next == std::char_traits<char>::eof())
    {
        m_state = State::Error;
        return;
    }

    auto ch = std::char_traits<char>::to_char_type(next);
    if (ch == '.')
    {
        m_state = State::FractionalPoint;
        m_file->advance(false);
    }
    else if (isAsciiDigit(ch))
    {
        m_state = State::Integer;
        m_hasDigits = true;
        m_file->advance(false);
    }
    else
    {
        m_state = State::Error;
    }
}

void
NumberStateMachine::processInteger(int next)
{
    if (next == std::char_traits<char>::eof())
    {
        m_state = State::End;
        return;
    }

    auto ch = std::char_traits<char>::to_char_type(next);
    if (ch == '.')
    {
        m_state = State::FractionalPoint;
        m_file->advance(false);
    }
    else if (isExponentSymbol(ch))
    {
        m_expPos = m_file->valueSize();
        m_state = State::ExponentSymbol;
        m_file->advance(false);
    }
    else if (isAsciiDigit(ch))
    {
        m_hasDigits = true;
        m_file->advance(false);
    }
    else
    {
        m_state = State::End;
    }
}

void
NumberStateMachine::processFractionalPoint(int next)
{
    if (next == std::char_traits<char>::eof())
    {
        m_state = m_hasDigits ? State::End : State::Error;
        return;
    }

    auto ch = std::char_traits<char>::to_char_type(next);
    if (isAsciiDigit(ch))
    {
        m_state = State::Fractional;
        m_hasDigits = true;
        m_file->advance(false);
    }
    else if (m_hasDigits)
    {
        if (isExponentSymbol(ch))
        {
            m_expPos = m_file->valueSize();
            m_state = State::ExponentSymbol;
            m_file->advance(false);
        }
        else
        {
            m_state = State::End;
        }
    }
    else
    {
        m_state = State::Error;
    }
}

void
NumberStateMachine::processFractional(int next)
{
    if (next == std::char_traits<char>::eof())
    {
        m_state = State::End;
        return;
    }

    auto ch = std::char_traits<char>::to_char_type(next);
    if (isExponentSymbol(ch))
    {
        m_expPos = m_file->valueSize();
        m_state = State::ExponentSymbol;
        m_file->advance(false);
    }
    else if (isAsciiDigit(ch))
    {
        m_hasDigits = true;
        m_file->advance(false);
    }
    else
    {
        m_state = State::End;
    }
}

void
NumberStateMachine::processExponentSymbol(int next)
{
    if (next == std::char_traits<char>::eof())
    {
        m_file->resizeValue(m_expPos);
        m_state = State::End;
        return;
    }

    auto ch = std::char_traits<char>::to_char_type(next);
    if (isNumberSign(ch))
    {
        m_state = State::ExponentSign;
        m_file->advance(false);
    }
    else if (isAsciiDigit(ch))
    {
        m_state = State::Exponent;
        m_file->advance(false);
    }
    else
    {
        m_file->resizeValue(m_expPos);
        m_state = State::End;
    }
}

void
NumberStateMachine::processExponentSign(int next)
{
    if (next == std::char_traits<char>::eof())
    {
        m_file->resizeValue(m_expPos);
        m_state = State::End;
        return;
    }

    auto ch = std::char_traits<char>::to_char_type(next);
    if (isAsciiDigit(ch))
    {
        m_state = State::Exponent;
        m_file->advance(false);
    }
    else
    {
        m_file->resizeValue(m_expPos);
        m_state = State::End;
    }
}

void
NumberStateMachine::processExponent(int next)
{
    if (next == std::char_traits<char>::eof())
    {
        m_state = State::End;
        return;
    }

    auto ch = std::char_traits<char>::to_char_type(next);
    if (isAsciiDigit(ch))
        m_file->advance(false);
    else
        m_state = State::End;
}

bool
NumberStateMachine::isInvalidEndState() const noexcept
{
    return m_state == State::Start ||
           m_state == State::IntegerSign ||
           (m_state == State::FractionalPoint && !m_hasDigits) ||
           m_state == State::ExponentSymbol ||
           m_state == State::ExponentSign;
}

class StringStateMachine
{
public:
    StringStateMachine(BufferedFile&, bool&);
    TokenType parse();

    bool needsEscape() const noexcept { return m_needsEscape; }

private:
    enum class State
    {
        Normal,
        Escape,
        Unicode,
        End,
        Error,
    };

    void processNormal(std::int32_t);
    void processEscape(std::int32_t);
    void processUnicode(std::int32_t);

    BufferedFile* m_file;
    State m_state{ State::Normal };
    unsigned int m_digits{ 0 };
    bool* m_needsEscape;
};

StringStateMachine::StringStateMachine(BufferedFile& file, bool& needsEscape) :
    m_file(&file),
    m_needsEscape(&needsEscape)
{
}

TokenType
StringStateMachine::parse()
{
    // skip initial quote
    m_file->advance(true);

    *m_needsEscape = false;
    UTF8Validator validator;
    while (m_state != State::End && m_state != State::Error)
    {
        int next = m_file->next();
        if (next == std::char_traits<char>::eof())
            return TokenType::Error;

        m_file->advance(false);
        std::int32_t cp = validator.check(std::char_traits<char>::to_char_type(next));
        if (cp == UTF8Validator::PartialSequence)
            continue;

        if (cp == UTF8Validator::InvalidStarter ||
            cp == UTF8Validator::InvalidTrailing ||
            cp == static_cast<std::int32_t>(U'\r'))
        {
            *m_needsEscape = true;
            continue;
        }

        switch (m_state)
        {
        case State::Normal:
            processNormal(cp);
            break;
        case State::Escape:
            processEscape(cp);
            break;
        case State::Unicode:
            processUnicode(cp);
            break;
        default:
            assert(0);
            break;
        }
    }

    return m_state == State::End ? TokenType::String : TokenType::Error;
}

void
StringStateMachine::processNormal(std::int32_t cp)
{
    switch (cp)
    {
    case static_cast<std::int32_t>(U'"'):
        m_state = State::End;
        break;

    case static_cast<std::int32_t>(U'\\'):
        *m_needsEscape = true;
        m_state = State::Escape;
        break;

    default:
        break;
    }
}

void
StringStateMachine::processEscape(std::int32_t cp)
{
    switch (cp)
    {
    case static_cast<std::int32_t>(U'"'):
    case static_cast<std::int32_t>(U'\\'):
    case static_cast<std::int32_t>(U'n'):
        m_state = State::Normal;
        break;

    case static_cast<std::int32_t>(U'u'):
        m_digits = 4;
        m_state = State::Unicode;
        break;

    default:
        m_state = State::Error;
        break;
    }
}

void
StringStateMachine::processUnicode(std::int32_t cp)
{
    if (!isHexDigit(cp))
    {
        m_state = State::Error;
        return;
    }

    --m_digits;
    if (m_digits == 0)
        m_state = State::Normal;
}

void
processEscape(std::string_view src, std::int32_t& pos, std::int32_t end, std::string& result)
{
    if (pos >= end)
    {
        result += UTF8_REPLACEMENT_CHAR;
        return;
    }

    switch (src[pos])
    {
    case '"':
        result += '"';
        ++pos;
        return;

    case '\\':
        result += '\\';
        ++pos;
        return;

    case 'n':
        result += '\n';
        ++pos;
        return;

    case 'u':
        break;

    default:
        result += UTF8_REPLACEMENT_CHAR;
        ++pos;
        return;
    }

    constexpr std::int32_t digits = 4;
    if (end - pos < digits + 1)
    {
        result += UTF8_REPLACEMENT_CHAR;
        pos = end;
        return;
    }

    std::uint32_t uch;
    const auto uStart = src.data() + (pos + 1);
    const auto uEnd = uStart + digits;
    pos += digits + 1;
    if (auto [ptr, ec] = compat::from_chars(uStart, uEnd, uch, 16);
        ec == std::errc{} && ptr == uEnd)
    {
        UTF8Encode(uch, result);
    }
    else
    {
        result += UTF8_REPLACEMENT_CHAR;
    }
}

std::string
unescapeString(std::string_view src, std::string& result)
{
    enum class StringState { Normal, LF, CR };

    result.reserve(src.size());
    auto state = StringState::Normal;
    const auto end = static_cast<std::int32_t>(src.size());
    for (std::int32_t pos = 0; pos < end;)
    {
        std::int32_t cp;
        if (!UTF8Decode(src, pos, cp))
        {
            result += UTF8_REPLACEMENT_CHAR;
            state = StringState::Normal;
            continue;
        }

        if (cp == static_cast<std::int32_t>(U'\n'))
        {
            if (state != StringState::CR)
                result += '\n';
            state = StringState::LF;
        }
        else if (cp == static_cast<std::int32_t>(U'\r'))
        {
            if (state != StringState::LF)
                result += '\n';
            state = StringState::CR;
        }
        else
        {
            state = StringState::Normal;
            if (cp == static_cast<std::int32_t>(U'\\'))
                processEscape(src, pos, end, result);
            else
                UTF8Encode(static_cast<std::uint32_t>(cp), result);
        }
    }

    return result;
}

} // end unnamed namespace

Tokenizer::Tokenizer(std::istream& in, std::size_t bufferSize) :
    m_file(in, bufferSize)
{}

TokenType
Tokenizer::nextToken()
{
    if (m_pushedBack)
    {
        m_pushedBack = false;
        return m_tokenType;
    }

    if (m_tokenType == TokenType::End || m_tokenType == TokenType::Error)
        return TokenType::End;

    m_needsEscape = false;
    m_escaped.clear();

    m_file.consume();
    bool isComment = false;
    for (;;)
    {
        int next = m_file.next();
        if (next == std::char_traits<char>::eof())
        {
            m_tokenType = m_file.error() ? TokenType::Error : TokenType::End;
            return m_tokenType;
        }

        auto ch = std::char_traits<char>::to_char_type(next);
        if (isComment)
        {
            if (ch == '\r' || ch == '\n')
                isComment = false;
        }
        else if (ch == '#')
        {
            isComment = true;
        }
        else if (ch != '\t' && ch != '\n' && ch != '\r' && ch != ' ')
        {
            m_tokenType = processToken(ch);
            return m_tokenType;
        }

        m_file.advance(true);
    }
}

TokenType
Tokenizer::processToken(char ch)
{
    switch (ch)
    {
    case '{':
        m_file.advance(true);
        return TokenType::BeginGroup;

    case '}':
        m_file.advance(true);
        return TokenType::EndGroup;

    case '[':
        m_file.advance(true);
        return TokenType::BeginArray;

    case ']':
        m_file.advance(true);
        return TokenType::EndArray;

    case '<':
        m_file.advance(true);
        return TokenType::BeginUnits;

    case '>':
        m_file.advance(true);
        return TokenType::EndUnits;

    case '=':
        m_file.advance(true);
        return TokenType::Equals;

    case '|':
        m_file.advance(true);
        return TokenType::Bar;

    case '"':
        return StringStateMachine(m_file, m_needsEscape).parse();

    default:
        if (isNumberStart(ch))
            return NumberStateMachine(m_file).parse();
        if (isNameStart(ch))
            return processName();
        m_file.advance(true);
        return TokenType::Error;
    }
}

TokenType
Tokenizer::processName()
{
    // Handle first letter
    m_file.advance(false);

    for (;;)
    {
        int next = m_file.next();
        if (next == std::char_traits<char>::eof())
            return m_file.error() ? TokenType::Error : TokenType::Name;

        if (!isNameContinuation(std::char_traits<char>::to_char_type(next)))
            return TokenType::Name;

        m_file.advance(false);
    }
}

std::optional<std::string_view>
Tokenizer::getNameValue() const
{
    if (m_tokenType != TokenType::Name)
        return std::nullopt;
    return m_file.value();
}

std::optional<std::string_view>
Tokenizer::getStringValue() const
{
    if (m_tokenType != TokenType::String)
        return std::nullopt;

    auto value = m_file.value();
    if (value.empty())
    {
        assert(0);
        return std::nullopt;
    }

    // Remove trailing quote
    value = value.substr(0, value.size() - 1U);
    if (!m_needsEscape)
        return value;

    if (m_escaped.empty())
        unescapeString(value, m_escaped);

    return m_escaped;
}

} // end namespace celestia::util
