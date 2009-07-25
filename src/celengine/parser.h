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
#include <celmath/quaternion.h>
#include <celutil/color.h>
#include <celutil/basictypes.h>
#include <celengine/tokenizer.h>
#include <Eigen/Core>
#include <Eigen/Geometry>

class Value;

typedef map<string, Value*>::const_iterator HashIterator;

class AssociativeArray
{
 public:
    AssociativeArray();
    ~AssociativeArray();

    Value* getValue(string) const;
    void addValue(string, Value&);

    bool getNumber(const std::string&, double&) const;
    bool getNumber(const std::string&, float&) const;
    bool getNumber(const std::string&, int&) const;
    bool getNumber(const std::string&, uint32&) const;
    bool getString(const std::string&, std::string&) const;
    bool getBoolean(const std::string&, bool&) const;
    bool getVector(const std::string&, Eigen::Vector3d&) const;
    bool getVector(const std::string&, Eigen::Vector3f&) const;
    bool getVector(const std::string&, Vec3d&) const;
    bool getVector(const std::string&, Vec3f&) const;
    bool getRotation(const std::string&, Quatf&) const;
    bool getRotation(const std::string&, Eigen::Quaternionf&) const;
    bool getColor(const std::string&, Color&) const;

    HashIterator begin();
    HashIterator end();
    
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
