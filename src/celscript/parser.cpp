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


Expression* Parser::parseFinalExpression()
{
    Scanner::TokenType tok = scanner.nextToken();
    if (tok == Scanner::TokenString)
    {
        return new ConstantExpression(Value(scanner.getStringValue()));
    }
    else if (tok == Scanner::TokenNumber)
    {
        return new ConstantExpression(Value(scanner.getNumberValue()));
    }
    else if (tok == Scanner::TokenName)
    {
        return new NameExpression(scanner.getNameValue());
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
        return parseFinalExpression();
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
    Expression* left = parseMultiplyExpression();
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
            Expression* right = parseMultiplyExpression();
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

Expression* Parser::parseRelationalExpression()
{
    Expression* left = parseAddExpression();
    if (left == NULL)
        return NULL;

    for (;;)
    {
        Scanner::TokenType tok = scanner.nextToken();
        BinaryExpression::Operator op;
        switch (tok)
        {
        case Scanner::TokenEqual:
            op = BinaryExpression::Equal;
            break;
        case Scanner::TokenNotEqual:
            op = BinaryExpression::NotEqual;
            break;
        case Scanner::TokenLesser:
            op = BinaryExpression::Lesser;
            break;
        case Scanner::TokenGreater:
            op = BinaryExpression::Greater;
            break;
        case Scanner::TokenLesserEqual:
            op = BinaryExpression::LesserEqual;
            break;
        case Scanner::TokenGreaterEqual:
            op = BinaryExpression::GreaterEqual;
            break;
        default:
            op = BinaryExpression::InvalidOp;
            break;
        }

        if (op != BinaryExpression::InvalidOp)
        {
            BinaryExpression::Operator op = BinaryExpression::Add;
            Expression* right = parseAddExpression();
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
    return parseRelationalExpression();
}


Statement* Parser::parseExpressionStatement()
{
    Expression* expr = parseExpression();
    if (expr == NULL)
    {
        syntaxError("Expression expected");
        return NULL;
    }

    if (scanner.nextToken() != Scanner::TokenEndStatement)
    {
        syntaxError("; expected");
        delete expr;
        return NULL;
    }

    return new ExpressionStatement(expr);
}


Statement* Parser::parseIfStatement()
{
    Expression* condition = NULL;
    Statement* ifClause = NULL;
    Statement* elseClause = NULL;

    if (scanner.nextToken() != Scanner::KeywordIf)
    {
        syntaxError("if statement expected");
        return NULL;
    }

    if (scanner.nextToken() != Scanner::TokenOpen)
    {
        syntaxError("( expected");
        return NULL;
    }

    condition = parseExpression();
    if (condition == NULL)
        return NULL;

    if (scanner.nextToken() != Scanner::TokenClose)
    {
        syntaxError(") expected");
        delete condition;
        return NULL;
    }

    ifClause = parseStatement();
    if (ifClause == NULL)
    {
        delete condition;
        return NULL;
    }

    if (scanner.nextToken() != Scanner::KeywordElse)
    {
        scanner.pushBack();
        return new IfStatement(condition, ifClause, new EmptyStatement());
    }

    elseClause = parseStatement();
    if (elseClause == NULL)
    {
        delete condition;
        delete ifClause;
        return NULL;
    }

    return new IfStatement(condition, ifClause, elseClause);
}


Statement* Parser::parseCompoundStatement()
{
    if (scanner.nextToken() != Scanner::TokenBeginGroup)
    {
        syntaxError("{ expected");
        return NULL;
    }

    while (scanner.nextToken() != Scanner::TokenEndGroup)
    {
        scanner.pushBack();
        Statement* statement = parseStatement();
        if (statement == NULL)
            return NULL;
    }

    return new CompoundStatement();
}


Statement* Parser::parseStatement()
{
    Scanner::TokenType tok = scanner.nextToken();
    if (tok == Scanner::TokenEndStatement)
        return new EmptyStatement();
    
    scanner.pushBack();

    switch (tok)
    {
    case Scanner::KeywordIf:
        return parseIfStatement();

    case Scanner::TokenBeginGroup:
        return parseCompoundStatement();

    default:
        return parseExpressionStatement();
    }
}


void Parser::syntaxError(const string& s)
{
    cout << s << '\n';
}
