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

#include <celscript/celx.h>
#include <string>
#include <iostream>
#include <celscript/type.h>


namespace celx
{

class Function;

class Value
{
 public:
    Value();
    Value(const Value&);
    Value(double);
    Value(const std::string&);
    Value(bool);
    Value(Function*);

    ~Value();

    Value& operator=(const Value&);

    inline Type getType() const;
    inline bool numberValue(double&) const;
    inline bool booleanValue(bool&) const;
    inline bool stringValue(std::string&) const;
    inline bool functionValue(Function*&) const;

    bool toBoolean() const;
    double toNumber() const;
    std::string toString() const;

    void output(std::ostream&) const;

    bool operator==(const Value&) const;
    bool operator!=(const Value&) const;

 private:
    Type type;
    union
    {
        bool boolVal;
        double numVal;
        const std::string* strVal;
        Function* funcVal;
    } val;
};

Type Value::getType() const
{
    return type;
}

bool Value::booleanValue(bool& x) const
{
    if (type != BooleanType)
    {
        return false;
    }
    else
    {
        x = val.boolVal;
        return true;
    }
}

bool Value::numberValue(double& x) const
{
    if (type != NumberType)
    {
        return false;
    }
    else
    {
        x = val.numVal;
        return true;
    }
}

bool Value::stringValue(std::string& x) const
{
    if (type != StringType)
    {
        return false;
    }
    else
    {
        x = *val.strVal;
        return true;
    }
}

bool Value::functionValue(Function*& f) const
{
    if (type != FunctionType)
    {
        return false;
    }
    else
    {
        f = val.funcVal;
        return true;
    }
}

} // namespace celx

std::ostream& operator<<(std::ostream&, const celx::Value&);

#endif // CELSCRIPT_VALUE_H_
