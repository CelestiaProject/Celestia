// scanner.cpp
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
#include <celscript/scanner.h>

using namespace std;
using namespace celx;


static bool issep(char c)
{
    return !isdigit(c) && !isalpha(c) && c != '.';
}


Scanner::Scanner(istream* _in) :
    in(_in),
    tokenType(TokenBegin),
    haveValidNumber(false),
    haveValidName(false),
    haveValidString(false),
    pushedBack(false)
{
}


Scanner::TokenType Scanner::nextToken()
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
            else if (nextChar == '(')
            {
                newToken = TokenOpen;
                nextChar = readChar();                
            }
            else if (nextChar == ')')
            {
                newToken = TokenClose;
                nextChar = readChar();                
            }
            else if (nextChar == '+')
            {
                state = PlusState;
            }
            else if (nextChar == '-')
            {
                state = MinusState;
            }
            else if (nextChar == '*')
            {
                state = AsteriskState;
            }
            else if (nextChar == '/')
            {
                state = SlashState;
            }
            else if (isalpha(nextChar))
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
            else if (nextChar == ';')
            {
                newToken = TokenEndStatement;
                nextChar = readChar();
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
                state = EqualState;
            }
            else if (nextChar == '<')
            {
                state = LessState;
            }
            else if (nextChar == '>')
            {
                state = GreaterState;
            }
            else if (nextChar == '!')
            {
                state = BangState;
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
            if (isalpha(nextChar) || isdigit(nextChar))
            {
                state = NameState;
                textToken += (char) nextChar;
            }
            else
            {
                if (textToken == "for")
                    newToken = KeywordFor;
                else if (textToken == "while")
                    newToken = KeywordWhile;
                else if (textToken == "if")
                    newToken = KeywordIf;
                else if (textToken == "else")
                    newToken = KeywordElse;
                else if (textToken == "var")
                    newToken = KeywordVar;
                else if (textToken == "null")
                    newToken = KeywordNull;
                else if (textToken == "true")
                    newToken = KeywordTrue;
                else if (textToken == "false")
                    newToken = KeywordFalse;
                else
                {
                    newToken = TokenName;
                    haveValidName = true;
                }
            }
            break;

        case CommentState:
            if (nextChar == '\n' || nextChar == '\r')
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
            }
            else if (nextChar == 'n')
            {
                textToken += '\n';
            }
            else if (nextChar == '"')
            {
                textToken += '"';
            }
            else
            {
                newToken = TokenError;
                syntaxError("Unknown escape code in string");
            }
            state = StringState;
            break;

        case MinusState:
            newToken = TokenMinus;
            state = StartState;
            break;

        case PlusState:
            newToken = TokenPlus;
            state = StartState;
            break;

        case AsteriskState:
            newToken = TokenMultiply;
            state = StartState;
            break;

        case SlashState:
            newToken = TokenDivide;
            state = StartState;
            break;

        case EqualState:
            if (nextChar == '=')
            {
                newToken = TokenEqual;
                nextChar = readChar();
                state = StartState;
            }
            else
            {
                newToken = TokenAssign;
                state = StartState;
            }
            break;

        case LessState:
            if (nextChar == '=')
            {
                newToken = TokenLesserEqual;
                nextChar = readChar();
                state = StartState;
            }
            else
            {
                newToken = TokenLesser;
                state = StartState;
            }
            break;

        case GreaterState:
            if (nextChar == '=')
            {
                newToken = TokenGreaterEqual;
                nextChar = readChar();
                state = StartState;
            }
            else
            {
                newToken = TokenGreater;
                state = StartState;
            }
            break;

        case BangState:
            if (nextChar == '=')
            {
                newToken = TokenNotEqual;
                nextChar = readChar();
                state = StartState;
            }
            else
            {
                newToken = TokenNot;
                state = StartState;
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
        }

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
            numberValue *= pow(10, exponentValue * exponentSign);
        numberValue *= sign;
    }

    return tokenType;
}


Scanner::TokenType Scanner::getTokenType()
{
    return tokenType;
}


void Scanner::pushBack()
{
    pushedBack = true;
}


double Scanner::getNumberValue()
{
    return numberValue;
}


string Scanner::getNameValue()
{
    return textToken;
}


string Scanner::getStringValue()
{
    return textToken;
}


int Scanner::readChar()
{
    return (int) in->get();
}

void Scanner::syntaxError(char* message)
{
    cerr << message << '\n';
}


int Scanner::getLineNumber()
{
    return 0;
}

#if 0
// Scanner test
int main(int argc, char *argv[])
{
    Scanner scanner(&cin);
    Scanner::TokenType tok = Scanner::TokenBegin;

    while (tok != Scanner::TokenEnd && tok != Scanner::TokenError)
    {
        tok = scanner.nextToken();
        switch (tok)
        {
        case Scanner::TokenBegin:
            cout << "Begin";
            break;
        case Scanner::TokenEnd:
            cout << "End";
            break;
        case Scanner::TokenName:
            cout << "Name = " << scanner.getNameValue();
            break;
        case Scanner::TokenNumber:
            cout << "Number = " << scanner.getNumberValue();
            break;
        case Scanner::TokenString:
            cout << "String = " << '"' << scanner.getStringValue() << '"';
            break;
        case Scanner::TokenOpen:
            cout << '(';
            break;
        case Scanner::TokenClose:
            cout << ')';
            break;
        case Scanner::TokenBeginGroup:
            cout << '{';
            break;
        case Scanner::TokenEndGroup:
            cout << '}';
            break;
        case Scanner::TokenEqual:
            cout << "==";
            break;
        case Scanner::TokenAssign:
            cout << '=';
            break;
        case Scanner::TokenPlus:
            cout << '+';
            break;
        case Scanner::TokenMinus:
            cout << '-';
            break;
        case Scanner::TokenMultiply:
            cout << '*';
            break;
        case Scanner::TokenEndStatement:
            cout << ';';
            break;
        case Scanner::TokenDivide:
            cout << '/';
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
