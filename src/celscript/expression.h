// expression.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef CELSCRIPT_EXPRESSION_H_
#define CELSCRIPT_EXPRESSION_H_

#include <celscript/celx.h>
#include <celscript/value.h>
#include <celscript/execution.h>


namespace celx
{

class Expression
{
 public:
    Expression();
    virtual ~Expression();
    virtual bool isLValue() const;

    virtual Value eval(ExecutionContext&) = 0;
    virtual Value* leval(ExecutionContext&);
};


class BinaryExpression : public Expression
{
 public:
    enum Operator
    {
        Add           =  0,
        Subtract      =  1,
        Multiply      =  2,
        Divide        =  3,
        Equal         =  4,
        NotEqual      =  5,
        Lesser        =  6,
        Greater       =  7,
        LesserEqual   =  8,
        GreaterEqual  =  9,
        InvalidOp     = 10,
        OperatorCount = 11,
    };

    BinaryExpression(Operator, Expression*, Expression*);
    virtual ~BinaryExpression();
    virtual Value eval(ExecutionContext&);

 private:
    const Operator op;
    Expression* left;
    Expression* right;
};


class UnaryExpression : public Expression
{
 public:
    enum Operator {
        Negate        = 0,
        LogicalNot    = 1,
        InvalidOp     = 2,
        OperatorCount = 3,
    };

    UnaryExpression(Operator, Expression*);
    virtual ~UnaryExpression();
    virtual Value eval(ExecutionContext&);

 private:
    const Operator op;
    Expression* expr;
};


class ConstantExpression : public Expression
{
 public:
    ConstantExpression(const Value&);
    virtual ~ConstantExpression();
    virtual Value eval(ExecutionContext&);

 private:
    Value value;
};


class IdentifierExpression : public Expression
{
 public:
    IdentifierExpression(const std::string&);
    virtual ~IdentifierExpression();
    virtual bool isLValue() const;
    virtual Value eval(ExecutionContext&);
    virtual Value* leval(ExecutionContext&);

 private:
    const std::string name;
    int stackDepth;
};


class AssignmentExpression : public Expression
{
 public:
    AssignmentExpression(Expression*, Expression*);
    virtual ~AssignmentExpression();
    virtual bool isLValue() const;
    virtual Value eval(ExecutionContext&);
    virtual Value* leval(ExecutionContext&);

 private:
    Expression* left;
    Expression* right;
};


} // namespace celx

#endif // CELSCRIPT_EXPRESSION_H_
