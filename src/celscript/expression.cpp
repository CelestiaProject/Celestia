// expression.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celscript/expression.h>

using namespace celx;


BinaryExpression::BinaryExpression(Operator _op,
                                   Expression* _left,
                                   Expression* _right) :
    op(_op),
    left(_left),
    right(_right)
{
}

BinaryExpression::~BinaryExpression()
{
    if (left != NULL)
        delete left;
    if (righ != NULL)
        delete right;
}

Value BinaryExpression::eval()
{
    return Value();
}


UnaryExpression::UnaryExpression(Operator _op,
                                 Expression* _expr) :
    op(_op),
    expr(_expr)
{
}

UnaryExpression::~UnaryExpression()
{
    if (expr != NULL)
        delete expr;
}

Value UnaryExpression::eval()
{
    return Value();
}


ConstantExpression::ConstantExpression(const Value& _value) :
    value(_value)
{
}

ConstantExpression::~ConstantExpression()
{
}

Value ConstantExpression::eval()
{
    return value;
}
