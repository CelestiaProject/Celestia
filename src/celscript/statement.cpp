// statement.cpp
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
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

Statement::Control ExpressionStatement::execute(ExecutionContext& context)
{
    cout << expr->eval(context) << '\n';
    return Statement::ControlAdvance;
}



VarStatement::VarStatement(const string& _name, Expression*_initializer) :
    name(_name), initializer(_initializer)
{
}

VarStatement::~VarStatement()
{
}

Statement::Control VarStatement::execute(ExecutionContext& context)
{
    Environment* env = context.getEnvironment();
    assert(env != NULL);

    env->bind(name, Value(initializer->eval(context)));

    return ControlAdvance;
}



CompoundStatement::CompoundStatement()
{
}

CompoundStatement::~CompoundStatement()
{
    for (vector<Statement*>::iterator iter = statements.begin();
         iter != statements.end(); iter++)
    {
        delete *iter;
    }
}

Statement::Control CompoundStatement::execute(ExecutionContext& context)
{
    for (vector<Statement*>::iterator iter = statements.begin();
         iter != statements.end(); iter++)
    {
        Control control = (*iter)->execute(context);
        switch (control)
        {
        case ControlReturn:
        case ControlBreak:
        case ControlContinue:
            return control;
        default:
            break;
        }
    }

    return ControlAdvance;
}

void CompoundStatement::addStatement(Statement* st)
{
    statements.insert(statements.end(), st);
}



ReturnStatement::ReturnStatement(Expression* _expr) :
    expr(_expr)
{
};

ReturnStatement::~ReturnStatement()
{
    delete expr;
}

Statement::Control ReturnStatement::execute(ExecutionContext& context)
{
    Value val = expr->eval(context);
    context.pushReturnValue(val);

    return ControlReturn;
}


IfStatement::IfStatement(Expression* _cond, Statement* _ifClause,
                         Statement* _elseClause) :
    condition(_cond), ifClause(_ifClause), elseClause(_elseClause)
{
}

IfStatement::~IfStatement()
{
}


Statement::Control IfStatement::execute(ExecutionContext& context)
{
    if (condition->eval(context).toBoolean())
        return ifClause->execute(context);
    else
        return elseClause->execute(context);
}


WhileStatement::WhileStatement(Expression* _cond, Statement* _body) :
    condition(_cond), body(_body)
{
}

WhileStatement::~WhileStatement()
{
}

Statement::Control WhileStatement::execute(ExecutionContext& context)
{
    Control control = ControlAdvance;
    while (condition->eval(context).toBoolean())
    {
        control = body->execute(context);
        if (control == ControlReturn || control == ControlBreak)
            break;
    }

    if (control == ControlReturn)
        return ControlReturn;
    else
        return ControlAdvance;
}


