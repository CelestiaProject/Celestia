// scriptparse.h
//
// Copyright (C) 2002 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELSCRIPT_SCRIPTPARSER_H_
#define _CELSCRIPT_SCRIPTPARSER_H_

#include <celscript/scanner.h>
#include <celscript/expression.h>
#include <celscript/statement.h>

namespace celx
{

class Parser
{
public:
    Parser(Scanner&);
    ~Parser();

    Expression* parseExpression();
    Expression* parseFinalExpression();
    Expression* parseSubexpression();
    Expression* parseUnaryExpression();
    Expression* parseMultiplyExpression();
    Expression* parseAddExpression();
    Expression* parseEqualityExpression();
    Expression* parseRelationalExpression();

    Statement* parseStatement();
    Statement* parseCompoundStatement();
    Statement* parseExpressionStatement();
    Statement* parseIfStatement();
    Statement* parseWhileStatement();

private:
    void syntaxError(const std::string&);

    Scanner& scanner;
};

} // namespace celx

#endif // _CELSCRIPT_SCRIPTPARSER_H_
