// scanner.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef CELSCRIPT_SCANNER_H_
#define CELSCRIPT_SCANNER_H_

#include <string>
#include <iostream>

namespace celx
{

class Scanner
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
        TokenEqual          = 10,
        TokenNotEqual       = 11,
        TokenBar            = 12,
        TokenOpen           = 13,
        TokenClose          = 14,
        TokenPlus           = 15,
        TokenMinus          = 16,
        TokenMultiply       = 17,
        TokenDivide         = 18,
        TokenEndStatement   = 19,
        TokenAssign         = 20,
        TokenError          = 255,
    };

    Scanner(std::istream*);

    TokenType nextToken();
    TokenType getTokenType();
    void pushBack();
    double getNumberValue();
    std::string getNameValue();
    std::string getStringValue();

    int getLineNumber();

private:
    enum State
    {
        StartState          = 0,
        NameState           = 1,
        NumberState         = 2,
        FractionState       = 3,
        ExponentState       = 4,
        ExponentFirstState  = 5,
        DotState            = 6,
        CommentState        = 7,
        StringState         = 8,
        ErrorState          = 9,
        StringEscapeState   = 10,
        MinusState          = 11,
        PlusState           = 12,
        EqualState          = 13,
        AsteriskState       = 14,
        SlashState          = 15,
    };

    std::istream* in;

    int nextChar;
    TokenType tokenType;
    bool haveValidNumber;
    bool haveValidName;
    bool haveValidString;

    bool pushedBack;

    int readChar();
    void syntaxError(char*);

    double numberValue;

    std::string textToken;
};

} // namespace celx

#endif // _SCANNER_H_
