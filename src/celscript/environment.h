// environment.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef CELSCRIPT_ENVIRONMENT_H_
#define CELSCRIPT_ENVIRONMENT_H_

#include <celscript/celx.h>
#include <string>
#include <map>
#include <celscript/value.h>

namespace celx
{

class Environment
{
 public:
    Environment();
    virtual ~Environment();

    virtual void bind(const std::string&, const Value&) = 0;
    virtual Value* lookup(const std::string&) const = 0;
    virtual Environment* getParent() const = 0;
};


class GlobalEnvironment : public Environment
{
 public:
    GlobalEnvironment();
    virtual ~GlobalEnvironment();

    virtual void bind(const std::string&, const Value&);
    virtual Value* lookup(const std::string&) const;
    virtual Environment* getParent() const;

 private:
    std::map<std::string, Value*> bindings;
};


} // namespace celx

#endif // CELSCRIPT_ENVIRONMENT_H_
