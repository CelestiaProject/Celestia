// statement.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef CELSCRIPT_STATEMENT_H_
#define CELSCRIPT_STATEMENT_H_

#include <celscript/celx.h>
#include <vector>
#include <celscript/expression.h>
#include <celscript/execution.h>


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

    virtual Control execute(ExecutionContext&) { return ControlAdvance; };
};


class EmptyStatement : public Statement
{
};


class ExpressionStatement : public Statement
{
 public:
    ExpressionStatement(Expression*);
    virtual ~ExpressionStatement();

    virtual Control execute(ExecutionContext&);

 private:
    Expression* expr;
};


class IfStatement : public Statement
{
 public:
    IfStatement(Expression*, Statement*, Statement*);
    virtual ~IfStatement();

    virtual Control execute(ExecutionContext&);

 private:
    Expression* condition;
    Statement* ifClause;
    Statement* elseClause;
};


class VarStatement : public Statement
{
 public:
    VarStatement(const std::string&, Expression*);
    virtual ~VarStatement();

    virtual Control execute(ExecutionContext&);

 private:
    std::string name;
    Expression* initializer;
};


class CompoundStatement : public Statement
{
 public:
    CompoundStatement();
    virtual ~CompoundStatement();

    virtual Control execute(ExecutionContext&);

    void addStatement(Statement*);

 private:
    std::vector<Statement*> statements;
};


class ReturnStatement : public Statement
{
 public:
    ReturnStatement(Expression*);
    virtual ~ReturnStatement();

    virtual Control execute(ExecutionContext&);

 private:
    Expression* expr;
};


class WhileStatement : public Statement
{
 public:
    WhileStatement(Expression*, Statement*);
    virtual ~WhileStatement();

    virtual Control execute(ExecutionContext&);

 private:
    Expression* condition;
    Statement* body;
};

} // namespace celx

#endif // CELSCRIPT_STATEMENT_H_
