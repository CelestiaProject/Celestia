// expression.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celscript/expression.h>

using namespace std;
using namespace celx;


typedef Value (BinaryOperatorFunc)(const Value&, const Value&);
typedef Value (UnaryOperatorFunc)(const Value&);

static BinaryOperatorFunc AddFunc;
static BinaryOperatorFunc SubtractFunc;
static BinaryOperatorFunc MultiplyFunc;
static BinaryOperatorFunc DivideFunc;
static BinaryOperatorFunc EqualFunc;
static BinaryOperatorFunc NotEqualFunc;
static BinaryOperatorFunc LesserFunc;
static BinaryOperatorFunc GreaterFunc;
static BinaryOperatorFunc LesserEqualFunc;
static BinaryOperatorFunc GreaterEqualFunc;

static Value ErrorValue = Value();


static BinaryOperatorFunc* BinaryOperatorFunctions[BinaryExpression::OperatorCount] =
{
    AddFunc,
    SubtractFunc,
    MultiplyFunc,
    DivideFunc,
    EqualFunc,
    NotEqualFunc,
    LesserFunc,
    GreaterFunc,
    LesserEqualFunc,
    GreaterEqualFunc,
    NULL,
};


static UnaryOperatorFunc NegateFunc;
static UnaryOperatorFunc LogicalNotFunc;
static UnaryOperatorFunc* UnaryOperatorFunctions[UnaryExpression::OperatorCount] =
{
    NegateFunc,
    LogicalNotFunc,
    NULL,
};


Expression::Expression()
{
}

Expression::~Expression()
{
}

bool Expression::isLValue() const
{
    return false;
}

Value* Expression::leval(ExecutionContext&)
{
    return NULL;
}


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
    if (right != NULL)
        delete right;
}

Value BinaryExpression::eval(ExecutionContext& context)
{
    Value a = left->eval(context);
    Value b = right->eval(context);
    return BinaryOperatorFunctions[op](a, b);
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

Value UnaryExpression::eval(ExecutionContext& context)
{
    Value v = expr->eval(context);
    return UnaryOperatorFunctions[op](v);
}


ConstantExpression::ConstantExpression(const Value& _value) :
    value(_value)
{
}

ConstantExpression::~ConstantExpression()
{
}

Value ConstantExpression::eval(ExecutionContext&)
{
    return value;
}


IdentifierExpression::IdentifierExpression(const string& _name) :
    name(_name)
{
}

IdentifierExpression::~IdentifierExpression()
{
}

bool IdentifierExpression::isLValue() const
{
    return true;
}

Value IdentifierExpression::eval(ExecutionContext& context)
{
    Value* val = context.getEnvironment()->lookup(name);
    if (val == NULL)
        return Value();
    else
        return *val;
}

Value* IdentifierExpression::leval(ExecutionContext& context)
{
    return context.getEnvironment()->lookup(name);
}



AssignmentExpression::AssignmentExpression(Expression* _left,
                                           Expression* _right) :
    left(_left), right(_right)
{
}

AssignmentExpression::~AssignmentExpression()
{
    delete left;
    delete right;
}

bool AssignmentExpression::isLValue() const
{
    return true;
}

Value AssignmentExpression::eval(ExecutionContext& context)
{
    Value* v = left->leval(context);
    if (v == NULL)
    {
        context.runtimeError();
        return Value();
    }

    *v = right->eval(context);

    return *v;
}

Value* AssignmentExpression::leval(ExecutionContext& context)
{
    Value* v = left->leval(context);
    if (v == NULL)
    {
        context.runtimeError();
        return NULL;
    }

    *v = right->eval(context);

    return v;
}


FunctionCallExpression::FunctionCallExpression(Expression* _func) :
    func(_func)
{
}

FunctionCallExpression::~FunctionCallExpression()
{
    if (func != NULL)
        delete func;
    for (vector<Expression*>::iterator iter = arguments.begin();
         iter != arguments.end(); iter++)
    {
        delete *iter;
    }
}

Value FunctionCallExpression::eval(ExecutionContext& context)
{
    Value funcVal = func->eval(context);
    Function* f;
    if (!funcVal.functionValue(f))
    {
        // Not a function
        context.runtimeError();
        return Value();
    }

    return Value();
}

void FunctionCallExpression::addArgument(Expression* expr)
{
    arguments.insert(arguments.end(), expr);
}



Value AddFunc(const Value& a, const Value& b)
{
    if (a.getType() == NumberType && b.getType() == NumberType)
    {
        double x0 = 0.0;
        double x1 = 0.0;
        a.numberValue(x0);
        b.numberValue(x1);
        return Value(x0 + x1);
    }
    else
    {
        return ErrorValue;
    }
}

Value SubtractFunc(const Value& a, const Value& b)
{
    if (a.getType() == NumberType && b.getType() == NumberType)
    {
        double x0 = 0.0;
        double x1 = 0.0;
        a.numberValue(x0);
        b.numberValue(x1);
        return Value(x0 - x1);
    }
    else
    {
        return ErrorValue;
    }
}

Value MultiplyFunc(const Value& a, const Value& b)
{
    if (a.getType() == NumberType && b.getType() == NumberType)
    {
        double x0 = 0.0;
        double x1 = 0.0;
        a.numberValue(x0);
        b.numberValue(x1);
        return Value(x0 * x1);
    }
    else
    {
        return ErrorValue;
    }
}

Value DivideFunc(const Value& a, const Value& b)
{
    if (a.getType() == NumberType && b.getType() == NumberType)
    {
        double x0 = 0.0;
        double x1 = 0.0;
        a.numberValue(x0);
        b.numberValue(x1);
        return Value(x0 / x1);
    }
    else
    {
        return ErrorValue;
    }
}


Value EqualFunc(const Value& a, const Value& b)
{
    return Value(a == b);
}


Value NotEqualFunc(const Value& a, const Value& b)
{
    return Value(a != b);
}


Value LesserFunc(const Value& a, const Value& b)
{
    return Value(a.toNumber() < b.toNumber());
}


Value GreaterFunc(const Value& a, const Value& b)
{
    return Value(a.toNumber() > b.toNumber());
}


Value LesserEqualFunc(const Value& a, const Value& b)
{
    return Value(a.toNumber() <= b.toNumber());
}


Value GreaterEqualFunc(const Value& a, const Value& b)
{
    return Value(a.toNumber() >= b.toNumber());
}



Value NegateFunc(const Value& v)
{
    return Value(-v.toNumber());
}

Value LogicalNotFunc(const Value& v)
{
    return Value(!v.toBoolean());
}
