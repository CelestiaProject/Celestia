// statement.cpp
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celscript/statement.h>

using namespace celx;
using namespace std;


Statement::Statement()
{
}

Statement::~Statement()
{
}



ExpressionStatement::ExpressionStatement(Expression* _expr) :
    expr(_expr)
{
}

ExpressionStatement::~ExpressionStatement()
{
    delete expr;
}

Statement::Control ExpressionStatement::execute()
{
    cout << expr->eval() << '\n';
    return Statement::ControlAdvance;
}



IfStatement::IfStatement(Expression* _cond, Statement* _ifClause,
                         Statement* _elseClause) :
    condition(_cond), ifClause(_ifClause), elseClause(_elseClause)
{
}

IfStatement::~IfStatement()
{
}

Statement::Control IfStatement::execute()
{
    if (condition->eval().toBoolean())
        return ifClause->execute();
    else
        return elseClause->execute();
}


WhileStatement::WhileStatement(Expression* _cond, Statement* _body) :
    condition(_cond), body(_body)
{
}

WhileStatement::~WhileStatement()
{
}

Statement::Control WhileStatement::execute()
{
    Control control = ControlAdvance;
    while (condition->eval().toBoolean())
    {
        control = body->execute();
        if (control == ControlReturn || control == ControlBreak)
            break;
    }

    if (control == ControlReturn)
        return ControlReturn;
    else
        return ControlAdvance;
}


