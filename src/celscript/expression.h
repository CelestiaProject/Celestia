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

#include <celscript/value.h>

namespace celx
{

class Expression
{
 public:
    Expression() {};
    virtual ~Expression() {};
    virtual Value eval() = 0;
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
    virtual Value eval();

 private:
    const Operator op;
    Expression* left;
    Expression* right;
};


class UnaryExpression : public Expression
{
 public:
    enum Operator {
        Negate,
    };

    UnaryExpression(Operator, Expression*);
    virtual ~UnaryExpression();
    virtual Value eval();

 private:
    const Operator op;
    const Expression* expr;
};


class ConstantExpression : public Expression
{
 public:
    ConstantExpression(const Value&);
    virtual ~ConstantExpression();
    virtual Value eval();

 private:
    Value value;
};


class NameExpression : public Expression
{
 public:
    NameExpression(const std::string&);
    virtual ~NameExpression();
    virtual Value eval();

 private:
    const std::string name;
};

} // namespace celx

#endif // CELSCRIPT_EXPRESSION_H_
