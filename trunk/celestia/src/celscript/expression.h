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
        Add,
        Subtract,
        Multiply,
        Divide,
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

} // namespace celx

#endif // CELSCRIPT_EXPRESSION_H_
