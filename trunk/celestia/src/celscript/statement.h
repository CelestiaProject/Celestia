// statement.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef STATEMENT_H_
#define STATEMENT_H_

#include <celscript/expression.h>


namespace celx 
{

class Statement
{
 public:
    Statement();
    virtual ~Statement();

    enum Control
    {
        ControlAdvance,
        ControlReturn,
        ControlBreak,
        ControlContinue,
    };

    virtual Control execute() { return ControlAdvance; };
};


class EmptyStatement : public Statement
{
};


class ExpressionStatement : public Statement
{
 public:
    ExpressionStatement(Expression*);
    virtual ~ExpressionStatement();

    virtual Control execute();

 private:
    Expression* expr;
};


class IfStatement : public Statement
{
 public:
    IfStatement(Expression*, Statement*, Statement*);
    virtual ~IfStatement();

    virtual Control execute();

 private:
    Expression* condition;
    Statement* ifClause;
    Statement* elseClause;
};


class CompoundStatement : public Statement
{
};


class WhileStatement : public Statement
{
 public:
    WhileStatement(Expression*, Statement*);
    virtual ~WhileStatement();

    virtual Control execute();

 private:
    Expression* condition;
    Statement* body;
};


} // namespace celx

#endif // STATEMENT_H_
