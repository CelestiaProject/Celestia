// function.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef CELSCRIPT_FUNCTION_H_
#define CELSCRIPT_FUNCTION_H_

#include <celscript/celx.h>
#include <vector>
#include <string>
#include <celscript/value.h>

namespace celx
{

class Statement;
class ExecutionContext; 

class Function
{
 public:
    Function(std::vector<std::string>*, Statement*);
    Function(const Function&);
    ~Function();

    Value call(ExecutionContext&);

 private:
    std::vector<std::string>* arguments;
    Statement* body;
};

} // namespace celx

#endif // CELSCRIPT_FUNCTION_H_
