// function.cpp
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celscript/function.h>
#include <celscript/statement.h>
#include <celscript/execution.h>

using namespace celx;
using namespace std;


Function::Function(vector<string>* _args, Statement* _body) :
    arguments(_args), body(_body)
{
}

Function::Function(const Function& f) :
    arguments(f.arguments), body(f.body)
{
}

Function::~Function()
{
    // delete arguments;
    // delete body;
}


Value Function::call(ExecutionContext& context)
{
    Statement::Control control = body->execute(context);
    if (control == Statement::ControlReturn)
    {
        return context.popReturnValue();
    }
    else
    {
        return Value();
    }
}

