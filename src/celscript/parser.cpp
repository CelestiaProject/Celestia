// parser.cpp
//
// Copyright (C) 2002 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include <celscript/parser.h>

using namespace std;
using namespace celx;


Parser::Parser(Scanner& _scanner) :
    scanner(_scanner),
    loopDepth(0),
    funcDepth(0)
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
    else if (tok == Scanner::KeywordNull)
    {
        return new ConstantExpression(Value());
    }
    else if (tok == Scanner::KeywordTrue)
    {
        return new ConstantExpression(Value(true));
    }
    else if (tok == Scanner::KeywordFalse)
    {
        return new ConstantExpression(Value(false));
    }
    else if (tok == Scanner::KeywordLambda)
    {
        Function* func = parseFunction();
        if (func == NULL)
            return NULL;
        else
            return new ConstantExpression(Value(func));
    }
    else if (tok == Scanner::TokenName)
    {
        return new IdentifierExpression(scanner.getNameValue());
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

Expression* Parser::parseFunctionCallExpression()
{
    Expression* expr = parseSubexpression();
    if (expr == NULL)
        return NULL;

    // TODO: Should check to see if we have something like a number
    // or string constant that can't possibly be a function call.  As it
    // is now, we treat all expressions equally.

    if (scanner.nextToken() != Scanner::TokenOpen)
    {
        scanner.pushBack();
        return expr;
    }

    FunctionCallExpression* func = new FunctionCallExpression(expr);

    Scanner::TokenType tok;
    for (;;)
    {
        tok = scanner.nextToken();
        if (tok == Scanner::TokenClose)
            break;
        scanner.pushBack();
        
        Expression* arg = parseExpression();
        if (arg == NULL)
        {
            delete func;
            return NULL;
        }

        func->addArgument(arg);
        if (scanner.nextToken() != Scanner::TokenComma)
            scanner.pushBack();
    }

    if (tok != Scanner::TokenClose)
    {
        syntaxError(") expected in function call");
        delete func;
        return NULL;
    }

    return func;
}

Expression* Parser::parseUnaryExpression()
{
    UnaryExpression::Operator op;
    Scanner::TokenType tok = scanner.nextToken();
    switch (tok)
    {
    case Scanner::TokenNot:
        op = UnaryExpression::LogicalNot;
        break;
    case Scanner::TokenMinus:
        op = UnaryExpression::Negate;
        break;
    default:
        op = UnaryExpression::InvalidOp;
        break;
    }

    if (op == UnaryExpression::InvalidOp)
    {
        scanner.pushBack();
        return parseFunctionCallExpression();
    }
    else
    {
        Expression* expr = parseUnaryExpression();
        if (expr == NULL)
            return NULL;
        else
            return new UnaryExpression(op, expr);
    }
}

Expression* Parser::parseMultiplyExpression()
{
    Expression* left = parseUnaryExpression();
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
            Expression* right = parseUnaryExpression();
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

Expression* Parser::parseEqualityExpression()
{
    Expression* left = parseAddExpression();
    if (left == NULL)
        return NULL;

    for (;;)
    {
        Scanner::TokenType tok = scanner.nextToken();
        if (tok == Scanner::TokenEqual || tok == Scanner::TokenNotEqual)
        {
            BinaryExpression::Operator op = BinaryExpression::Equal;
            if (tok == Scanner::TokenNotEqual)
                op = BinaryExpression::NotEqual;
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

Expression* Parser::parseRelationalExpression()
{
    Expression* left = parseEqualityExpression();
    if (left == NULL)
        return NULL;

    for (;;)
    {
        Scanner::TokenType tok = scanner.nextToken();
        BinaryExpression::Operator op;
        switch (tok)
        {
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
            Expression* right = parseEqualityExpression();
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

Expression* Parser::parseAssignmentExpression()
{
    Expression* left = parseRelationalExpression();
    if (left == NULL)
        return NULL;

    Scanner::TokenType tok = scanner.nextToken();
    if (tok == Scanner::TokenAssign)
    {
        // Assignment operator is right associative
        Expression* right = parseAssignmentExpression();
        if (right == NULL)
        {
            delete left;
            return NULL;
        }

        if (!left->isLValue())
        {
            syntaxError("lvalue expected for assignment");
            delete left;
            delete right;
            return NULL;
        }

        return new AssignmentExpression(left, right);
    }
    else
    {
        scanner.pushBack();
        return left;
    }
}


Expression* Parser::parseExpression()
{
    return parseAssignmentExpression();
}


Function* Parser::parseFunction()
{
    if (scanner.nextToken() != Scanner::TokenOpen)
    {
        syntaxError("Argument list expected for function");
        return NULL;
    }

    vector<string>* args = new vector<string>;

    Scanner::TokenType tok;
    for (;;)
    {
        tok = scanner.nextToken();
        if (tok == Scanner::TokenClose)
            break;

        if (tok != Scanner::TokenName)
        {
            delete args;
            return NULL;
        }

        args->insert(args->end(), scanner.getNameValue());
        if (scanner.nextToken() != Scanner::TokenComma)
            scanner.pushBack();
    }

    if (tok != Scanner::TokenClose)
    {
        syntaxError(") expected in function definition");
        delete args;
        return NULL;
    }

    // TODO: Verify that there are no duplicate names in the argument list

    if (scanner.nextToken() != Scanner::TokenBeginGroup)
    {
        syntaxError("function body expected");
        delete args;
        return NULL;
    }
    scanner.pushBack();

    funcDepth++;
    Statement* body = parseCompoundStatement();
    funcDepth--;
    assert(funcDepth >= 0);

    if (body == NULL)
    {
        delete args;
        return NULL;
    }
    else
    {
        return new Function(args, body);
    }
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


Statement* Parser::parseVarStatement()
{
    if (scanner.nextToken() != Scanner::KeywordVar)
    {
        syntaxError("var expected");
        return NULL;
    }

    if (scanner.nextToken() != Scanner::TokenName)
    {
        syntaxError("identifier expected");
        return NULL;
    }

    string name = scanner.getNameValue();
    Expression* initializer;

    if (scanner.nextToken() != Scanner::TokenEndStatement)
    {
        if (scanner.getTokenType() != Scanner::TokenAssign)
        {
            syntaxError("variable initialiazer expected");
            return NULL;
        }

        initializer = parseExpression();
        if (initializer == NULL)
        {
            return NULL;
        }

        scanner.nextToken();
    }
    else
    {
        initializer = new ConstantExpression(Value());
    }

    if (scanner.getTokenType() != Scanner::TokenEndStatement)
    {
        delete initializer;
        return NULL;
    }
    else
    {
        defineLocal(name);
        return new VarStatement(name, initializer);
    }
}


Statement* Parser::parseCompoundStatement()
{
    if (scanner.nextToken() != Scanner::TokenBeginGroup)
    {
        syntaxError("{ expected");
        return NULL;
    }

    beginFrame();
    CompoundStatement* compound = new CompoundStatement();
    while (scanner.nextToken() != Scanner::TokenEndGroup)
    {
        scanner.pushBack();
        Statement* statement = parseStatement();
        if (statement == NULL)
        {
            delete compound;
            endFrame();
            return NULL;
        }
        
        compound->addStatement(statement);
    }
    endFrame();

    return compound;
}


Statement* Parser::parseWhileStatement()
{
    Expression* condition = NULL;
    Statement* body = NULL;

    if (scanner.nextToken() != Scanner::KeywordWhile)
    {
        syntaxError("while statement expected");
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

    loopDepth++;
    body = parseStatement();
    loopDepth--;
    assert(loopDepth >= 0);

    if (body == NULL)
    {
        delete condition;
        return NULL;
    }

    return new WhileStatement(condition, body);
}

Statement* Parser::parseReturnStatement()
{
    if (scanner.nextToken() != Scanner::KeywordReturn)
    {
        syntaxError("return statement expected");
        return NULL;
    }
    
    Expression* expr = NULL;
    if (scanner.nextToken() == Scanner::TokenEndStatement)
    {
        expr = new ConstantExpression(Value());
    }
    else
    {
        scanner.pushBack();
        expr = parseExpression();
        if (expr == NULL)
            return NULL;
        if (scanner.nextToken() != Scanner::TokenEndStatement)
        {
            syntaxError("missing ;");
            delete expr;
            return NULL;
        }
    }

    if (funcDepth == 0)
    {
        syntaxError("return appears outside a function");
        delete expr;
        return NULL;
    }

    return new ReturnStatement(expr);
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

    case Scanner::KeywordVar:
        return parseVarStatement();

    case Scanner::TokenBeginGroup:
        return parseCompoundStatement();

    case Scanner::KeywordWhile:
        return parseWhileStatement();

    case Scanner::KeywordReturn:
        return parseReturnStatement();

    default:
        return parseExpressionStatement();
    }
}


int Parser::resolveName(const std::string& name)
{
    int i = scope.size();
    int stackDepth = 0;
    while (i-- > 0)
    {
        if (!scope[i].empty())
        {
            if (scope[i] == name)
                return stackDepth;
            stackDepth++;
        }
    }

    return -1;
}


void Parser::defineLocal(const std::string& name)
{
    scope.push_back(name);
}


void Parser::beginFrame()
{
    scope.push_back("");
}

void Parser::endFrame()
{
    assert(!scope.empty());
    while (!scope.back().empty())
    {
        scope.pop_back();
        assert(!scope.empty());
    }
    scope.pop_back();
}


void Parser::syntaxError(const string& s)
{
    cout << s << '\n';
}
