// tokenizer.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _TOKENIZER_H_
#define _TOKENIZER_H_

#include <string>
#include <iostream>

using namespace std;


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
    };

    Tokenizer(istream*);

    TokenType nextToken();
    TokenType getTokenType();
    void pushBack();
    double getNumberValue();
    string getNameValue();
    string getStringValue();

    int getLineNumber() const;

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
        UnicodeEscapeState  = 11,
    };

    istream* in;

    int nextChar;
    TokenType tokenType;
    bool haveValidNumber;
    bool haveValidName;
    bool haveValidString;

    unsigned int unicodeValue;
    unsigned int unicodeEscapeDigits;

    bool pushedBack;

    int readChar();
    void syntaxError(const char*);

    double numberValue;

    string textToken;

    int lineNum;
};

#endif // _TOKENIZER_H_
