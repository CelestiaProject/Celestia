// parser.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _PARSER_H_
#define _PARSER_H_

#include <vector>
#include <map>
#include <celmath/vecmath.h>
#include <celutil/color.h>
#include <celengine/tokenizer.h>

class Value;

class AssociativeArray
{
 public:
    AssociativeArray();
    ~AssociativeArray();

    Value* getValue(string) const;
    void addValue(string, Value&);

    bool getNumber(string, double&) const;
    bool getNumber(string, float&) const;
    bool getString(string, string&) const;
    bool getBoolean(string, bool&) const;
    bool getVector(string, Vec3d&) const;
    bool getVector(string, Vec3f&) const;
    bool getColor(string, Color&) const;
    
 private:
    map<string, Value*> assoc;
};

typedef vector<Value*> Array;
typedef AssociativeArray Hash;

class Value
{
public:
    enum ValueType {
        NumberType     = 0,
        StringType     = 1,
        ArrayType      = 2,
        HashType       = 3,
        BooleanType    = 4
    };

    Value(double);
    Value(string);
    Value(Array*);
    Value(Hash*);
    Value(bool);
    ~Value();

    ValueType getType() const;

    double getNumber() const;
    string getString() const;
    Array* getArray() const;
    Hash* getHash() const;
    bool getBoolean() const;

private:
    ValueType type;

    union {
        string* s;
        double d;
        Array* a;
        Hash* h;
    } data;
};


class Parser
{
public:
    Parser(Tokenizer*);

    Array* readArray();
    Hash* readHash();
    Value* readValue();

private:
    Tokenizer* tokenizer;
};

#endif // _PARSER_H_
