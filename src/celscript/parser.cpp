// parser.cpp
//
// Copyright (C) 2002 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celscript/parser.h>

using namespace std;
using namespace celx;


Parser::Parser(Scanner& _scanner) :
    scanner(_scanner)
{
}


Parser::~Parser()
{
}


Expression* Parser::parseConstantExpression()
{
    Scanner::TokenType tok = scanner.nextToken();
    if (tok == Scanner::TokenString)
    {
        return ConstantExpression(Value(scanner.getStringValue()));
    }
    else if (tok == Scanner::TokenNumber)
    {
        return ConstantExpression(Value(scanner.getNumberValue()));
    }
    else
    {
        syntaxError("constant expression expected.");
        return NULL;
    }
}

Expression* Parser::parseSubexpression()
{
    Scanner::TokenType tok = scanner.nextToken();
    if (tok == Scanner::TokenOpen)
    {
        Expression* expr = parseExpression();
        if (expr == NULL)
            return NULL;

        tok = scanner.nextToken();
        if (tok != Scanner::TokenClose)
        {
            syntaxError("')' expected");
            delete expr;
            return NULL;
        }

        return expr;
    }
    else
    {
        scanner.pushBack();
        return parseConstantExpression();
    }
}

Expression* Parser::parseMultiplyExpression()
{
    Expression* left = parseSubexpression();
    if (left == NULL)
        return NULL;

    for (;;)
    {
        Scanner::TokenType tok = scanner.nextToken();
        if (tok == Scanner::TokenMultiply || tok == Scanner::TokenDivide)
        {
            BinaryExpression::Operator op = BinaryExpression::Multiply;
            if (tok == Scanner::TokenDivide)
                op = BinaryExpression::Divide;
            Expression* right = parseSubexpression();
            if (right == NULL)
            {
                delete left;
                return NULL;
            }
            left = new BinaryExpression(op, left, right);
        }
        else
        {
            scanner.pushBack();
            return left;
        }
    }
}

Expression* Parser::parseAddExpression()
{
    Expression* left = parseSubexpression();
    if (left == NULL)
        return NULL;

    for (;;)
    {
        Scanner::TokenType tok = scanner.nextToken();
        if (tok == Scanner::TokenPlus || tok == Scanner::TokenMinus)
        {
            BinaryExpression::Operator op = BinaryExpression::Add;
            if (tok == Scanner::TokenMinus)
                op = BinaryExpression::Subtract;
            Expression* right = parseSubexpression();
            if (right == NULL)
            {
                delete left;
                return NULL;
            }
            left = new BinaryExpression(op, left, right);
        }
        else
        {
            scanner.pushBack();
            return left;
        }
    }
}

Expression* Parser::parseExpression()
{
    return parseAddExpression();
}


