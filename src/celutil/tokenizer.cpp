// tokenizer.cpp
//
// Copyright (C) 2001-2021, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <istream>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include <system_error>

#include <celcompat/charconv.h>
#include <celutil/utf8.h>
#include "tokenizer.h"

using namespace std::string_view_literals;

namespace
{
constexpr inline std::string_view UTF8_BOM = "\357\273\277"sv;

constexpr bool
isWhitespace(char ch)
{
    return ch == ' ' || ch == '\t' || ch == '\r';
}

constexpr bool
isAsciiDigit(char ch)
{
    return ch >= '0' && ch <= '9';
}

constexpr bool
isStartName(char ch)
{
    return (ch >= 'A' && ch <= 'Z') ||
           (ch >= 'a' && ch <= 'z') ||
           (ch == '_');
}

constexpr bool
isName(char ch)
{
    return isStartName(ch) || isAsciiDigit(ch);
}

constexpr bool
isSign(char ch)
{
    return ch == '+' || ch == '-';
}


enum class NumberPart
{
    Initial,
    Fraction,
    Exponent,
    End,
    Error,
};


struct NumberState
{
    std::size_t endPosition{ 0 };
    NumberPart part{ NumberPart::Initial };
    bool isInteger{ true };
};


struct StringState
{
    std::size_t runStart{ 1 };
    std::size_t runEnd{ 1 };
    std::string processed{};
    UTF8Validator validator{};

    bool checkUTF8(char, std::string_view);
};


bool
StringState::checkUTF8(char c, std::string_view run)
{
    auto utf8Status = validator.check(c);
    if (utf8Status == UTF8Status::Ok) { return true; }

    if (utf8Status == UTF8Status::InvalidTrailingByte)
    {
        std::size_t pos = run.size();
        while (pos > 0)
        {
            --pos;
            auto uch = static_cast<unsigned char>(run[pos]);
            if (uch >= 0xc0) { break; }
        }

        run = run.substr(0, pos);
    }

    processed.append(run);
    processed.append(UTF8_REPLACEMENT_CHAR);
    runStart = runEnd + 1;
    runEnd = runStart;
    return false;
}


struct VisitorStringView
{
    std::optional<std::string_view> operator()(std::monostate) const { return std::nullopt; }
    std::optional<std::string_view> operator()(std::int32_t) const { return std::nullopt; }
    std::optional<std::string_view> operator()(double) const { return std::nullopt; }
    std::optional<std::string_view> operator()(std::string_view sv) const { return sv; }
    std::optional<std::string_view> operator()(const std::string& s) const { return s; }
};


struct VisitorDouble
{
    std::optional<double> operator()(std::monostate) const { return std::nullopt; }
    std::optional<double> operator()(std::int32_t i) const { return static_cast<double>(i); }
    std::optional<double> operator()(double d) const { return d; }
    std::optional<double> operator()(std::string_view) const { return std::nullopt; }
    std::optional<double> operator()(const std::string&) const { return std::nullopt; }
};


struct VisitorInteger
{
    std::optional<std::int32_t> operator()(std::monostate) const { return std::nullopt; }
    std::optional<std::int32_t> operator()(std::int32_t i) const { return i; }
    std::optional<std::int32_t> operator()(double) const { return std::nullopt; }
    std::optional<std::int32_t> operator()(std::string_view) const { return std::nullopt; }
    std::optional<std::int32_t> operator()(const std::string&) const { return std::nullopt; }
};

} // end unnamed namespace


class TokenizerImpl
{
public:
    using TokenValue = std::variant<std::monostate, std::int32_t, double, std::string_view, std::string>;

    TokenizerImpl(std::istream*, std::size_t);

    Tokenizer::TokenType nextToken();

    const TokenValue& getTokenValue() const { return tokenValue; }
    int getLineNumber() const { return lineNumber; }

private:
    std::istream* in;
    std::vector<char> buffer;
    std::size_t position{ 0 };
    std::size_t length{ 0 };
    TokenValue tokenValue{ std::in_place_type<std::monostate> };

    int lineNumber{ 1 };
    bool isAtStart{ true };
    bool isEnded{ false };

    bool skipUTF8Bom();

    std::variant<char, Tokenizer::TokenType> skipWhitespace();
    Tokenizer::TokenType readToken(char);

    bool readString();
    bool readNumber();
    bool readName();

    NumberState createNumberState();
    static void parseDecimal(NumberState&);
    void parseExponent(NumberState&);
    bool setNumber(const NumberState&);

    bool parseChar(StringState&, char, std::string_view);
    bool parseEscape(StringState&, char);

    bool fillBuffer(std::size_t* = nullptr);
    std::optional<char> peekAt(std::size_t&);
};


TokenizerImpl::TokenizerImpl(std::istream* _in, std::size_t _bufferSize)
    : in(_in),
      buffer(_bufferSize)
{}


Tokenizer::TokenType
TokenizerImpl::nextToken()
{
    tokenValue.emplace<std::monostate>();

    // skip UTF8 BOM
    if (isAtStart && !skipUTF8Bom()) { return Tokenizer::TokenError; }

    auto wsResult = skipWhitespace();
    return std::visit([this](auto& arg)
    {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, Tokenizer::TokenType>)
        {
            return arg;
        }
        else
        {
            static_assert(std::is_same_v<T, char>);
            return readToken(arg);
        }
    }, wsResult);
}


std::variant<char, Tokenizer::TokenType>
TokenizerImpl::skipWhitespace()
{
    for (;;)
    {
        // skip whitespace
        auto bufferEnd = buffer.cbegin() + length;
        auto it = std::find_if_not(buffer.cbegin() + position, bufferEnd, isWhitespace);
        position = it - buffer.cbegin();
        if (it == bufferEnd)
        {
            if (isEnded) { return Tokenizer::TokenEnd; }
            if (!fillBuffer()) { return Tokenizer::TokenError; }
            continue;
        }

        // track lines
        if (*it == '\n')
        {
            ++lineNumber;
            ++position;
            continue;
        }

        if (*it != '#') {
            return *it;
        }

        // skip comments
        for (;;)
        {
            it = std::find(buffer.cbegin() + position, bufferEnd, '\n');
            position = it - buffer.cbegin();
            if (it != bufferEnd)
            {
                ++position;
                break;
            }

            if (isEnded) { return Tokenizer::TokenEnd; }
            if (!fillBuffer()) { return Tokenizer::TokenError; }
            bufferEnd = buffer.cbegin() + length;
        }
    }
}


Tokenizer::TokenType
TokenizerImpl::readToken(char ch)
{
    switch (ch)
    {
    case '{':
        ++position;
        return Tokenizer::TokenBeginGroup;

    case '}':
        ++position;
        return Tokenizer::TokenEndGroup;

    case '[':
        ++position;
        return Tokenizer::TokenBeginArray;

    case ']':
        ++position;
        return Tokenizer::TokenEndArray;

    case '=':
        ++position;
        return Tokenizer::TokenEquals;

    case '|':
        ++position;
        return Tokenizer::TokenBar;

    case '<':
        ++position;
        return Tokenizer::TokenBeginUnits;

    case '>':
        ++position;
        return Tokenizer::TokenEndUnits;

    case '"':
        return readString() ? Tokenizer::TokenString : Tokenizer::TokenError;

    default:
        if (ch == '-' || ch == '+' || ch == '.' || isAsciiDigit(ch))
        {
            return readNumber() ? Tokenizer::TokenNumber : Tokenizer::TokenError;
        }
        if (isStartName(ch))
        {
            return readName() ? Tokenizer::TokenName : Tokenizer::TokenError;
        }

        ++position;
        return Tokenizer::TokenError;
    }
}


bool
TokenizerImpl::skipUTF8Bom()
{
    if (!fillBuffer()) { return false; }
    isAtStart = false;
    if (length >= UTF8_BOM.size() && std::string_view(buffer.data(), UTF8_BOM.size()) == UTF8_BOM)
    {
        position += UTF8_BOM.size();
    }

    return true;
}


bool
TokenizerImpl::readName()
{
    std::size_t endPosition = position + 1;
    do
    {
        auto bufferEnd = buffer.cbegin() + length;
        auto it = std::find_if_not(buffer.cbegin() + endPosition, bufferEnd, isName);
        endPosition = it - buffer.cbegin();
        if (it != bufferEnd || isEnded) { break; }

        if (!fillBuffer(&endPosition))
        {
            position = endPosition;
            return false;
        }
    } while (endPosition < length);

    tokenValue.emplace<std::string_view>(buffer.data() + position, endPosition - position);
    position = endPosition;

    return true;
}


bool
TokenizerImpl::readNumber()
{
    NumberState state = createNumberState();
    if (state.part == NumberPart::Error)
    {
        return false;
    }

    while (state.part != NumberPart::End)
    {
        auto bufferEnd = buffer.cbegin() + length;
        auto it = std::find_if_not(buffer.cbegin() + state.endPosition, bufferEnd, isAsciiDigit);
        state.endPosition = it - buffer.cbegin();
        if (it == bufferEnd)
        {
            if (isEnded)
            {
                state.part = NumberPart::End;
            }
            else if (!fillBuffer(&state.endPosition))
            {
                position = state.endPosition;
                return false;
            }
            else if (state.endPosition == length)
            {
                state.part = NumberPart::End;
            }
            continue;
        }

        switch (*it)
        {
        case '.':
            parseDecimal(state);
            break;

        case 'E':
        case 'e':
            parseExponent(state);
            break;

        default:
            state.part = NumberPart::End;
            break;
        }
    }

    return setNumber(state);
}


NumberState
TokenizerImpl::createNumberState()
{
    NumberState state;
    state.endPosition = position + 1;
    if (buffer[position] == '.')
    {
        // decimal point must be followed by a digit
        if (auto check = peekAt(state.endPosition); !isAsciiDigit(check.value_or('\0')))
        {
            position = state.endPosition;
            state.part = NumberPart::Error;
            return state;
        }
        state.isInteger = false;
        state.part = NumberPart::Fraction;
    }
    else if (isSign(buffer[position]))
    {
        // sign must be followed by either a decimal point or a digit
        if (auto check = peekAt(state.endPosition); check == '.')
        {
            ++state.endPosition;
            state.isInteger = false;
            // decimal point must be followed by a digit
            if (auto check2 = peekAt(state.endPosition); !isAsciiDigit(check2.value_or('\0')))
            {
                position = state.endPosition;
                state.part = NumberPart::Error;
            }
        }
        else if (!isAsciiDigit(check.value_or('\0')))
        {
            position = state.endPosition;
            state.part = NumberPart::Error;
        }
    }

    return state;
}


void
TokenizerImpl::parseDecimal(NumberState& numberState)
{
    if (numberState.part == NumberPart::Initial)
    {
        numberState.isInteger = false;
        numberState.part = NumberPart::Fraction;
        ++numberState.endPosition;
    }
    else
    {
        numberState.part = NumberPart::End;
    }
}


void
TokenizerImpl::parseExponent(NumberState& numberState)
{
    if (numberState.part == NumberPart::Initial || numberState.part == NumberPart::Fraction)
    {
        ++numberState.endPosition;
        if (auto check = peekAt(numberState.endPosition).value_or('\0'); isSign(check))
        {
            ++numberState.endPosition;
            if (auto check2 = peekAt(numberState.endPosition).value_or('\0'); isAsciiDigit(check2))
            {
                numberState.part = NumberPart::Exponent;
            }
            else
            {
                numberState.endPosition -= 2;
                numberState.part = NumberPart::End;
            }
        }
        else if (isAsciiDigit(check))
        {
            numberState.part = NumberPart::Exponent;
        }
        else
        {
            --numberState.endPosition;
            numberState.part = NumberPart::End;
        }
    }
    else
    {
        numberState.part = NumberPart::End;
    }
}


bool
TokenizerImpl::setNumber(const NumberState& numberState)
{
    using celestia::compat::from_chars;

    const char* startPtr = buffer.data() + position;
    if (*startPtr == '+') { ++startPtr; }

    const char* endPtr = buffer.data() + numberState.endPosition;
    position = numberState.endPosition;

    // detect negative zero in order to roundtrip CMOD correctly
    bool isNegative = *startPtr == '-';

    if (numberState.isInteger)
    {
        std::int32_t intResult;
        if (auto [ptr, ec] = from_chars(startPtr, endPtr, intResult);
            ec == std::errc{} && ptr == endPtr && !(intResult == 0 && isNegative))
        {
            tokenValue = intResult;
            return true;
        }
    }

    double dblResult;
    if (auto [ptr, ec] = from_chars(startPtr, endPtr, dblResult);
        ec == std::errc{} && ptr == endPtr)
    {
        tokenValue = dblResult;
        return true;
    }

    return false;
}


bool
TokenizerImpl::readString()
{
    StringState state;
    for (;;)
    {
        char ch;
        auto peekPos = position + state.runEnd;
        if (auto check = peekAt(peekPos); check.has_value())
        {
            ch = *check;
        }
        else
        {
            position = peekPos;
            return false;
        }

        std::string_view run(buffer.data() + position + state.runStart,
                             state.runEnd - state.runStart);

        if (!state.checkUTF8(ch, run)) { continue; }
        if (ch == '"') { break; }
        if (!parseChar(state, ch, run)) { return false;}
    }

    const char* startPtr = buffer.data() + position;
    position += state.runEnd + 1;

    if (state.runStart == 1)
    {
        // string did not need to be modified
        tokenValue.emplace<std::string_view>(startPtr + state.runStart, state.runEnd - state.runStart);
        return true;
    }

    state.processed.append(startPtr + state.runStart, startPtr + state.runEnd);
    tokenValue = state.processed;
    return true;
}


bool
TokenizerImpl::parseChar(StringState& state, char ch, std::string_view run)
{
    switch (ch)
    {
    case '\r':
        state.processed.append(run);
        state.runStart = state.runEnd + 1;
        state.runEnd = state.runStart;
        break;

    case '\\':
        {
            state.processed.append(run);
            std::size_t peekPos = position + state.runEnd + 1;
            if (auto check = peekAt(peekPos); check.has_value())
            {
                return parseEscape(state, *check);
            }

            position = peekPos;
            return false;
        }

    case '\n':
        ++lineNumber;
        [[fallthrough]];

    default:
        ++state.runEnd;
        break;
    }

    return true;
}


bool
TokenizerImpl::parseEscape(StringState& state, char ch)
{
    switch (ch)
    {
    case '"':
        state.processed.push_back('\"');
        state.runStart = state.runEnd + 2;
        state.runEnd = state.runStart;
        break;

    case 'n':
        state.processed.push_back('\n');
        state.runStart = state.runEnd + 2;
        state.runEnd = state.runStart;
        break;

    case '\\':
        state.processed.push_back('\\');
        state.runStart = state.runEnd + 2;
        state.runEnd = state.runStart;
        break;

    case 'u':
        {
            std::size_t peekPos = position + state.runEnd + 5;
            if (auto check = peekAt(peekPos); !check.has_value())
            {
                position = length;
                return false;
            }

            const char* uStart = buffer.data() + position + state.runEnd + 2;
            const char* uEnd = buffer.data() + position + state.runEnd + 6;

            std::uint32_t uch;
            if (auto [ptr, ec] = celestia::compat::from_chars(uStart, uEnd, uch, 16);
                ec == std::errc{} && ptr == uEnd)
            {
                UTF8Encode(uch, state.processed);
                state.runStart = state.runEnd + 6;
                state.runEnd = state.runStart;
            }
            else
            {
                position = ptr - buffer.data();
                return false;
            }
        }

        break;

    default:
        position += state.runEnd + 1;
        return false;
    }

    return true;
}


bool
TokenizerImpl::fillBuffer(std::size_t* alterOffset)
{
    // If we've hit EOF or we've got an overlong token then exit
    if (position == 0 && length > 0) { return false; }

    assert(position <= length);

    std::size_t unprocessed = length - position;
    // Move any unprocessed elements to the front of the buffer
    if (unprocessed > 0)
    {
        std::memmove(buffer.data(), buffer.data() + position, unprocessed);
    }

    if (alterOffset)
    {
        *alterOffset -= position;
    }

    in->read(buffer.data() + unprocessed, buffer.size() - unprocessed);
    if (in->bad())
    {
        return false;
    }

    if (in->eof())
    {
        isEnded = true;
    }

    position = 0;
    length = unprocessed + static_cast<std::size_t>(in->gcount());
    return true;
}


std::optional<char>
TokenizerImpl::peekAt(std::size_t& offset)
{
    if (offset < length) { return buffer[offset]; }
    if (isEnded || !fillBuffer(&offset) || offset >= length) { return std::nullopt; }
    return buffer[offset];
}


Tokenizer::Tokenizer(std::istream* _in, std::size_t buffer_size)
    : impl(std::make_unique<TokenizerImpl>(_in, buffer_size))
{}


Tokenizer::~Tokenizer() = default;


void
Tokenizer::pushBack()
{
    isPushedBack = true;
}


Tokenizer::TokenType
Tokenizer::nextToken()
{
    if (isPushedBack)
    {
        isPushedBack = false;
    }
    else
    {
        tokenType = impl->nextToken();
    }

    return tokenType;
}


Tokenizer::TokenType
Tokenizer::getTokenType() const
{
    return tokenType;
}


std::optional<std::string_view>
Tokenizer::getNameValue() const
{
    if (tokenType != TokenType::TokenName) { return std::nullopt; }
    return std::visit(VisitorStringView(), impl->getTokenValue());
}

std::optional<std::string_view>
Tokenizer::getStringValue() const
{
    if (tokenType != TokenType::TokenString) { return std::nullopt; }
    return std::visit(VisitorStringView(), impl->getTokenValue());
}


std::optional<double>
Tokenizer::getNumberValue() const
{
    return std::visit(VisitorDouble(), impl->getTokenValue());
}


std::optional<std::int32_t>
Tokenizer::getIntegerValue() const
{
    return std::visit(VisitorInteger(), impl->getTokenValue());
}


int
Tokenizer::getLineNumber() const
{
    return impl->getLineNumber();
}


