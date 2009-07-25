// parser.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "parser.h"

using namespace Eigen;


/****** Value method implementations *******/

Value::Value(double d)
{
    type = NumberType;
    data.d = d;
}

Value::Value(string s)
{
    type = StringType;
    data.s = new string(s);
}

Value::Value(Array* a)
{
    type = ArrayType;
    data.a = a;
}

Value::Value(Hash* h)
{
    type = HashType;
    data.h = h;
}

Value::Value(bool b)
{
    type = BooleanType;
    data.d = b ? 1.0 : 0.0;
}

Value::~Value()
{
    if (type == StringType)
    {
        delete data.s;
    }
    else if (type == ArrayType)
    {
        if (data.a != NULL)
        {
            for (unsigned int i = 0; i < data.a->size(); i++)
                delete (*data.a)[i];
            delete data.a;
        }
    }
    else if (type == HashType)
    {
        if (data.h != NULL)
        {
#if 0
            Hash::iterator iter = data.h->begin();
            while (iter != data.h->end())
            {
                delete iter->second;
                iter++;
            }
#endif
            delete data.h;
        }
    }
}

Value::ValueType Value::getType() const
{
    return type;
}

double Value::getNumber() const
{
    // ASSERT(type == NumberType);
    return data.d;
}

string Value::getString() const
{
    // ASSERT(type == StringType);
    return *data.s;
}

Array* Value::getArray() const
{
    // ASSERT(type == ArrayType);
    return data.a;
}

Hash* Value::getHash() const
{
    // ASSERT(type == HashType);
    return data.h;
}

bool Value::getBoolean() const
{
    // ASSERT(type == BooleanType);
    return (data.d != 0.0);
}


/****** Parser method implementation ******/

Parser::Parser(Tokenizer* _tokenizer) :
    tokenizer(_tokenizer)
{
}


Array* Parser::readArray()
{
    Tokenizer::TokenType tok = tokenizer->nextToken();
    if (tok != Tokenizer::TokenBeginArray)
    {
        tokenizer->pushBack();
        return NULL;
    }

    Array* array = new Array();

    Value* v = readValue();
    while (v != NULL)
    {
        array->insert(array->end(), v);
        v = readValue();
    }
    
    tok = tokenizer->nextToken();
    if (tok != Tokenizer::TokenEndArray)
    {
        tokenizer->pushBack();
        delete array;
        return NULL;
    }

    return array;
}


Hash* Parser::readHash()
{
    Tokenizer::TokenType tok = tokenizer->nextToken();
    if (tok != Tokenizer::TokenBeginGroup)
    {
        tokenizer->pushBack();
        return NULL;
    }

    Hash* hash = new Hash();

    tok = tokenizer->nextToken();
    while (tok != Tokenizer::TokenEndGroup)
    {
        if (tok != Tokenizer::TokenName)
        {
            tokenizer->pushBack();
            delete hash;
            return NULL;
        }
        string name = tokenizer->getNameValue();

        Value* value = readValue();
        if (value == NULL)
        {
            delete hash;
            return NULL;
        }
        
        hash->addValue(name, *value);

        tok = tokenizer->nextToken();
    }

    return hash;
}


Value* Parser::readValue()
{
    Tokenizer::TokenType tok = tokenizer->nextToken();
    switch (tok)
    {
    case Tokenizer::TokenNumber:
        return new Value(tokenizer->getNumberValue());

    case Tokenizer::TokenString:
        return new Value(tokenizer->getStringValue());

    case Tokenizer::TokenName:
        if (tokenizer->getNameValue() == "false")
            return new Value(false);
        else if (tokenizer->getNameValue() == "true")
            return new Value(true);
        else
        {
            tokenizer->pushBack();
            return NULL;
        }

    case Tokenizer::TokenBeginArray:
        tokenizer->pushBack();
        {
            Array* array = readArray();
            if (array == NULL)
                return NULL;
            else
                return new Value(array);
        }

    case Tokenizer::TokenBeginGroup:
        tokenizer->pushBack();
        {
            Hash* hash = readHash();
            if (hash == NULL)
                return NULL;
            else
                return new Value(hash);
        }

    default:
        tokenizer->pushBack();
        return NULL;
    }
}


AssociativeArray::AssociativeArray()
{
}

AssociativeArray::~AssociativeArray()
{
#if 0
    Hash::iterator iter = data.h->begin();
    while (iter != data.h->end())
    {
        delete iter->second;
        iter++;
    }
#endif
    for (map<string, Value*>::iterator iter = assoc.begin(); iter != assoc.end(); iter++)
        delete iter->second;
}

Value* AssociativeArray::getValue(string key) const
{
    map<string, Value*>::const_iterator iter = assoc.find(key);
    if (iter == assoc.end())
        return NULL;
    else
        return iter->second;
}

void AssociativeArray::addValue(string key, Value& val)
{
    assoc.insert(map<string, Value*>::value_type(key, &val));
}

bool AssociativeArray::getNumber(const string& key, double& val) const
{
    Value* v = getValue(key);
    if (v == NULL || v->getType() != Value::NumberType)
        return false;

    val = v->getNumber();

    return true;
}

bool AssociativeArray::getNumber(const string& key, float& val) const
{
    double dval;

    if (!getNumber(key, dval))
    {
        return false;
    }
    else
    {
        val = (float) dval;
        return true;
    }
}

bool AssociativeArray::getNumber(const string& key, int& val) const
{
    double ival;

    if (!getNumber(key, ival))
    {
        return false;
    }
    else
    {
        val = (int) ival;
        return true;
    }
}

bool AssociativeArray::getNumber(const string& key, uint32& val) const
{
    double ival;

    if (!getNumber(key, ival))
    {
        return false;
    }
    else
    {
        val = (uint32) ival;
        return true;
    }
}

bool AssociativeArray::getString(const string& key, string& val) const
{
    Value* v = getValue(key);
    if (v == NULL || v->getType() != Value::StringType)
        return false;

    val = v->getString();

    return true;
}

bool AssociativeArray::getBoolean(const string& key, bool& val) const
{
    Value* v = getValue(key);
    if (v == NULL || v->getType() != Value::BooleanType)
        return false;

    val = v->getBoolean();

    return true;
}

bool AssociativeArray::getVector(const string& key, Vec3d& val) const
{
    Value* v = getValue(key);
    if (v == NULL || v->getType() != Value::ArrayType)
        return false;

    Array* arr = v->getArray();
    if (arr->size() != 3)
        return false;

    Value* x = (*arr)[0];
    Value* y = (*arr)[1];
    Value* z = (*arr)[2];

    if (x->getType() != Value::NumberType ||
        y->getType() != Value::NumberType ||
        z->getType() != Value::NumberType)
        return false;

    val = Vec3d(x->getNumber(), y->getNumber(), z->getNumber());

    return true;
}

bool AssociativeArray::getVector(const string& key, Vector3d& val) const
{
    Value* v = getValue(key);
    if (v == NULL || v->getType() != Value::ArrayType)
        return false;

    Array* arr = v->getArray();
    if (arr->size() != 3)
        return false;

    Value* x = (*arr)[0];
    Value* y = (*arr)[1];
    Value* z = (*arr)[2];

    if (x->getType() != Value::NumberType ||
        y->getType() != Value::NumberType ||
        z->getType() != Value::NumberType)
        return false;

    val = Vector3d(x->getNumber(), y->getNumber(), z->getNumber());

    return true;
}

bool AssociativeArray::getVector(const string& key, Vec3f& val) const
{
    Vec3d vecVal;

    if (!getVector(key, vecVal))
        return false;

    val = Vec3f((float) vecVal.x, (float) vecVal.y, (float) vecVal.z);

    return true;
}


bool AssociativeArray::getVector(const string& key, Vector3f& val) const
{
    Vector3d vecVal;

    if (!getVector(key, vecVal))
        return false;

    val = vecVal.cast<float>();

    return true;
}


bool AssociativeArray::getRotation(const string& key, Quatf& val) const
{
    Value* v = getValue(key);
    if (v == NULL || v->getType() != Value::ArrayType)
        return false;

    Array* arr = v->getArray();
    if (arr->size() != 4)
        return false;

    Value* w = (*arr)[0];
    Value* x = (*arr)[1];
    Value* y = (*arr)[2];
    Value* z = (*arr)[3];

    if (w->getType() != Value::NumberType ||
        x->getType() != Value::NumberType ||
        y->getType() != Value::NumberType ||
        z->getType() != Value::NumberType)
        return false;

    Vec3f axis((float) x->getNumber(),
               (float) y->getNumber(),
               (float) z->getNumber());
    axis.normalize();
    float angle = degToRad((float) w->getNumber());
    val.setAxisAngle(axis, angle);

    return true;
}


bool AssociativeArray::getRotation(const string& key, Quaternionf& val) const
{
    Value* v = getValue(key);
    if (v == NULL || v->getType() != Value::ArrayType)
        return false;

    Array* arr = v->getArray();
    if (arr->size() != 4)
        return false;

    Value* w = (*arr)[0];
    Value* x = (*arr)[1];
    Value* y = (*arr)[2];
    Value* z = (*arr)[3];

    if (w->getType() != Value::NumberType ||
        x->getType() != Value::NumberType ||
        y->getType() != Value::NumberType ||
        z->getType() != Value::NumberType)
        return false;

    Vector3f axis((float) x->getNumber(),
                  (float) y->getNumber(),
                  (float) z->getNumber());
    float angle = degToRad((float) w->getNumber());

    val = Quaternionf(AngleAxisf(angle, axis.normalized()));

    return true;
}


bool AssociativeArray::getColor(const string& key, Color& val) const
{
    Vec3d vecVal;

    if (!getVector(key, vecVal))
        return false;

    val = Color((float) vecVal.x, (float) vecVal.y, (float) vecVal.z);

    return true;
}


HashIterator
AssociativeArray::begin()
{
    return assoc.begin();
}


HashIterator
AssociativeArray::end()
{
    return assoc.end();
}
