// tokenizer.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cctype>
#include <cmath>
#include <iomanip>
#include <celutil/utf8.h>
#include "tokenizer.h"


static bool issep(char c)
{
    return !isdigit(c) && !isalpha(c) && c != '.';
}


Tokenizer::Tokenizer(istream* _in) :
    in(_in),
    tokenType(TokenBegin),
    haveValidNumber(false),
    haveValidName(false),
    haveValidString(false),
    pushedBack(false),
    lineNum(1)
{
}


Tokenizer::TokenType Tokenizer::nextToken()
{
    State state = StartState;

    if (pushedBack)
    {
        pushedBack = false;
        return tokenType;
    }

    textToken = "";
    haveValidNumber = false;
    haveValidName = false;
    haveValidString = false;

    if (tokenType == TokenBegin)
    {
        nextChar = readChar();
        if (in->eof())
            return TokenEnd;
    }
    else if (tokenType == TokenEnd)
    {
        return tokenType;
    }

    double integerValue = 0;
    double fractionValue = 0;
    double sign = 1;
    double fracExp = 1;
    double exponentValue = 0;
    double exponentSign = 1;

    TokenType newToken = TokenBegin;
    while (newToken == TokenBegin)
    {
        switch (state)
        {
        case StartState:
            if (isspace(nextChar))
            {
                state = StartState;
            }
            else if (isdigit(nextChar))
            {
                state = NumberState;
                integerValue = (int) nextChar - (int) '0';
            }
            else if (nextChar == '-')
            {
                state = NumberState;
                sign = -1;
                integerValue = 0;
            }
            else if (nextChar == '+')
            {
                state = NumberState;
                sign = +1;
                integerValue = 0;
            }
            else if (nextChar == '.')
            {
                state = FractionState;
                sign = +1;
                integerValue = 0;
            }
            else if (isalpha(nextChar) || nextChar == '_')
            {
                state = NameState;
                textToken += (char) nextChar;
            }
            else if (nextChar == '#')
            {
                state = CommentState;
            }
            else if (nextChar == '"')
            {
                state = StringState;
            }
            else if (nextChar == '{')
            {
                newToken = TokenBeginGroup;
                nextChar = readChar();
            }
            else if (nextChar == '}')
            {
                newToken = TokenEndGroup;
                nextChar = readChar();
            }
            else if (nextChar == '[')
            {
                newToken = TokenBeginArray;
                nextChar = readChar();
            }
            else if (nextChar == ']')
            {
                newToken = TokenEndArray;
                nextChar = readChar();
            }
            else if (nextChar == '=')
            {
                newToken = TokenEquals;
                nextChar = readChar();
            }
            else if (nextChar == '|')
            {
                newToken = TokenBar;
                nextChar = readChar();
            }
            else if (nextChar == -1)
            {
                newToken = TokenEnd;
            }
            else
            {
                newToken = TokenError;
                syntaxError("Bad character in stream");
            }
            break;

        case NameState:
            if (isalpha(nextChar) || isdigit(nextChar) || nextChar == '_')
            {
                state = NameState;
                textToken += (char) nextChar;
            }
            else
            {
                newToken = TokenName;
                haveValidName = true;
            }
            break;

        case CommentState:
            if (nextChar == '\n' || nextChar == '\r' || nextChar == char_traits<char>::eof())
                state = StartState;
            break;

        case StringState:
            if (nextChar == '"')
            {
                newToken = TokenString;
                haveValidString = true;
                nextChar = readChar();
            }
            else if (nextChar == '\\')
            {
                state = StringEscapeState;
            }
            else if (nextChar == char_traits<char>::eof())
            {
                newToken = TokenError;
                syntaxError("Unterminated string");
            }
            else
            {
                state = StringState;
                textToken += (char) nextChar;
            }
            break;

        case StringEscapeState:
            if (nextChar == '\\')
            {
                textToken += '\\';
                state = StringState;
            }
            else if (nextChar == 'n')
            {
                textToken += '\n';
                state = StringState;
            }
            else if (nextChar == '"')
            {
                textToken += '"';
                state = StringState;
            }
            else if (nextChar == 'u')
            {
                unicodeValue = 0;
                unicodeEscapeDigits = 0;
                state = UnicodeEscapeState;
            }
            else
            {
                newToken = TokenError;
                syntaxError("Unknown escape code in string");
                state = StringState;
            }
            break;

        case NumberState:
            if (isdigit(nextChar))
            {
                state = NumberState;
                integerValue = integerValue * 10 + (int) nextChar - (int) '0';
            }
            else if (nextChar == '.')
            {
                state = FractionState;
            }
            else if (nextChar == 'e' || nextChar == 'E')
            {
                state = ExponentFirstState;
            }
            else if (issep(nextChar))
            {
                newToken = TokenNumber;
                haveValidNumber = true;
            }
            else
            {
                newToken = TokenError;
                syntaxError("Bad character in number");
            }
            break;

        case FractionState:
            if (isdigit(nextChar))
            {
                state = FractionState;
                fractionValue = fractionValue * 10 + nextChar - (int) '0';
                fracExp *= 10;
            }
            else if (nextChar == 'e' || nextChar == 'E')
            {
                state = ExponentFirstState;
            }
            else if (issep(nextChar))
            {
                newToken = TokenNumber;
                haveValidNumber = true;
            } else {
                newToken = TokenError;
                syntaxError("Bad character in number");
            }
            break;

        case ExponentFirstState:
            if (isdigit(nextChar))
            {
                state = ExponentState;
                exponentValue = (int) nextChar - (int) '0';
            }
            else if (nextChar == '-')
            {
                state = ExponentState;
                exponentSign = -1;
            }
            else if (nextChar == '+')
            {
                state = ExponentState;
            }
            else
            {
                state = ErrorState;
                syntaxError("Bad character in number");
            }
            break;

        case ExponentState:
            if (isdigit(nextChar))
            {
                state = ExponentState;
                exponentValue = exponentValue * 10 + (int) nextChar - (int) '0';
            }
            else if (issep(nextChar))
            {
                newToken = TokenNumber;
                haveValidNumber = true;
            }
            else
            {
                state = ErrorState;
                syntaxError("Bad character in number");
            }
            break;

        case DotState:
            if (isdigit(nextChar))
            {
                state = FractionState;
                fractionValue = fractionValue * 10 + (int) nextChar - (int) '0';
                fracExp = 10;
            }
            else
            {
                state = ErrorState;
                syntaxError("'.' in stupid place");
            }
            break;

        case UnicodeEscapeState:
            if (isxdigit(nextChar))
            {
                unsigned int digitValue;
                if (nextChar >= 'a' && nextChar <= 'f')
                    digitValue = nextChar - 'a' + 10;
                else if (nextChar >= 'A' && nextChar <= 'F')
                    digitValue = nextChar - 'A' + 10;
                else
                    digitValue = nextChar - '0';
                unicodeValue = (unicodeValue << 4) + digitValue;
                unicodeEscapeDigits++;
                if (unicodeEscapeDigits == 4)
                {
                    char utf8Encoded[7];
                    UTF8Encode((wchar_t) unicodeValue, utf8Encoded);
                    textToken += utf8Encoded;
                    state = StringState;
                }
            }
            else
            {
                state = ErrorState;
                syntaxError("Bad Unicode escape in string");
            }
            break;

        case ErrorState:    break;  // Prevent GCC4 warnings; do nothing

        } // Switch

        if (newToken == TokenBegin)
        {
            nextChar = readChar();
        }
    }

    tokenType = newToken;
    if (haveValidNumber)
    {
        numberValue = integerValue + fractionValue / fracExp;
        if (exponentValue != 0)
            numberValue *= pow(10.0, exponentValue * exponentSign);
        numberValue *= sign;
    }

    return tokenType;
}


Tokenizer::TokenType Tokenizer::getTokenType()
{
    return tokenType;
}


void Tokenizer::pushBack()
{
    pushedBack = true;
}


double Tokenizer::getNumberValue()
{
    return numberValue;
}


string Tokenizer::getNameValue()
{
    return textToken;
}


string Tokenizer::getStringValue()
{
    return textToken;
}


int Tokenizer::readChar()
{
    int c = (int) in->get();
    if (c == '\n')
        lineNum++;

    return c;
}

void Tokenizer::syntaxError(const char* message)
{
    cerr << message << '\n';
}


int Tokenizer::getLineNumber() const
{
    return lineNum;
}

#if 0
// Tokenizer test
int main(int argc, char *argv[])
{
    Tokenizer tokenizer(&cin);
    Tokenizer::TokenType tok = Tokenizer::TokenBegin;

    while (tok != Tokenizer::TokenEnd)
    {
        tok = tokenizer.nextToken();
        switch (tok)
        {
        case Tokenizer::TokenBegin:
            cout << "Begin";
            break;
        case Tokenizer::TokenEnd:
            cout << "End";
            break;
        case Tokenizer::TokenName:
            cout << "Name = " << tokenizer.getNameValue();
            break;
        case Tokenizer::TokenNumber:
            cout << "Number = " << tokenizer.getNumberValue();
            break;
        case Tokenizer::TokenString:
            cout << "String = " << '"' << tokenizer.getStringValue() << '"';
            break;
        case Tokenizer::TokenBeginGroup:
            cout << '{';
            break;
        case Tokenizer::TokenEndGroup:
            cout << '}';
            break;
        case Tokenizer::TokenEquals:
            cout << '=';
            break;
        default:
            cout << "Other";
            break;
        }

        cout << '\n';
    }

    return 0;
}
#endif
