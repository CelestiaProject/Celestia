// value.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef CELSCRIPT_VALUE_H_
#define CELSCRIPT_VALUE_H_

#include <string>
#include <celscript/type.h>

namespace celx
{

class Value
{
 public:
    Value();
    Value(double);
    Value(const std::string&);

    ~Value();

    inline Type getType();

 private:
    Type type;
    union
    {
        double numVal;
        const std::string* strVal;
    } val;
};

Type Value::getType()
{
    return type;
}
    
} // namespace celx

#endif // CELSCRIPT_VALUE_H_
