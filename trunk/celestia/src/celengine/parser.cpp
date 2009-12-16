// parser.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "parser.h"
#include "astro.h"

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
        
#ifndef USE_POSTFIX_UNITS
        readUnits(name, hash);
#endif

        Value* value = readValue();
        if (value == NULL)
        {
            delete hash;
            return NULL;
        }

        hash->addValue(name, *value);
        
#ifdef USE_POSTFIX_UNITS
        readUnits(name, hash);
#endif

        tok = tokenizer->nextToken();
    }

    return hash;
}


/**
 * Reads a units section into the hash.
 * @param[in] propertyName Name of the current property.
 * @param[in] hash Hash to add units quantities into.
 * @return True if a units section was successfully read, false otherwise.
 */
bool Parser::readUnits(const string& propertyName, Hash* hash)
{
    Tokenizer::TokenType tok = tokenizer->nextToken();
    if (tok != Tokenizer::TokenBeginUnits)
    {
        tokenizer->pushBack();
        return false;
    }

    tok = tokenizer->nextToken();
    while (tok != Tokenizer::TokenEndUnits)
    {
        if (tok != Tokenizer::TokenName)
        {
            tokenizer->pushBack();
            return false;
        }
       
        string unit = tokenizer->getNameValue();
        Value* value = new Value(unit);
        
        if (astro::isLengthUnit(unit))
        {
            string keyName(propertyName + "%Length");
            hash->addValue(keyName, *value);
        }
        else if (astro::isTimeUnit(unit))
        {
            string keyName(propertyName + "%Time");
            hash->addValue(keyName, *value);
        }
        else if (astro::isAngleUnit(unit))
        {
            string keyName(propertyName + "%Angle");
            hash->addValue(keyName, *value);
        }
        else
        {
            delete value;
            return false;
        }
        
        tok = tokenizer->nextToken();
    }
    
    return true;
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


/** @copydoc AssociativeArray::getRotation() */
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
    
    double ang = w->getNumber();
    double angScale = 1.0;
    getAngleScale(key, angScale);
    float angle = degToRad((float) (ang * angScale));

    val.setAxisAngle(axis, angle);

    return true;
}


/**
 * Retrieves a quaternion, scaled to an associated angle unit.
 * 
 * The quaternion is specified in the catalog file in axis-angle format as follows:
 * \verbatim {PropertyName} [ angle axisX axisY axisZ ] \endverbatim
 * 
 * @param[in] key Hash key for the rotation.
 * @param[out] val A quaternion representing the value if present, unaffected if not.
 * @return True if the key exists in the hash, false otherwise.
 */
bool AssociativeArray::getRotation(const string& key, Eigen::Quaternionf& val) const
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

    double ang = w->getNumber();
    double angScale = 1.0;
    getAngleScale(key, angScale);
    float angle = degToRad((float) (ang * angScale));

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


/**
 * Retrieves a numeric quantity scaled to an associated angle unit.
 * @param[in] key Hash key for the quantity.
 * @param[out] val The returned quantity if present, unaffected if not.
 * @param[in] outputScale Returned value is scaled to this value.
 * @param[in] defaultScale If no units are specified, use this scale. Defaults to outputScale.
 * @return True if the key exists in the hash, false otherwise.
 */
bool
AssociativeArray::getAngle(const string& key, double& val, double outputScale, double defaultScale) const
{
    if (!getNumber(key, val))
        return false;
    
    double angleScale;
    if(getAngleScale(key, angleScale))
    {
        angleScale /= outputScale;
    }
    else
    {
        angleScale = (defaultScale == 0.0) ? 1.0 : defaultScale / outputScale;
    }
    
    val *= angleScale;
    
    return true;
}


/** @copydoc AssociativeArray::getAngle() */ 
bool
AssociativeArray::getAngle(const string& key, float& val, double outputScale, double defaultScale) const
{
    double dval;
    
    if (!getAngle(key, dval, outputScale, defaultScale))
        return false;
        
    val = ((float) dval);
    
    return true;
}


/**
 * Retrieves a numeric quantity scaled to an associated length unit.
 * @param[in] key Hash key for the quantity.
 * @param[out] val The returned quantity if present, unaffected if not.
 * @param[in] outputScale Returned value is scaled to this value.
 * @param[in] defaultScale If no units are specified, use this scale. Defaults to outputScale.
 * @return True if the key exists in the hash, false otherwise.
 */
bool
AssociativeArray::getLength(const string& key, double& val, double outputScale, double defaultScale) const
{
    if(!getNumber(key, val))
        return false;
    
    double lengthScale;
    if(getLengthScale(key, lengthScale))
    {
        lengthScale /= outputScale;
    }
    else
    {
        lengthScale = (defaultScale == 0.0) ? 1.0 : defaultScale / outputScale;
    }
    
    val *= lengthScale;
    
    return true;
}


/** @copydoc AssociativeArray::getLength() */
bool AssociativeArray::getLength(const string& key, float& val, double outputScale, double defaultScale) const
{
    double dval;
    
    if (!getLength(key, dval, outputScale, defaultScale))
        return false;
    
    val = ((float) dval);
    
    return true;
}


/**
 * Retrieves a numeric quantity scaled to an associated time unit.
 * @param[in] key Hash key for the quantity.
 * @param[out] val The returned quantity if present, unaffected if not.
 * @param[in] outputScale Returned value is scaled to this value.
 * @param[in] defaultScale If no units are specified, use this scale. Defaults to outputScale.
 * @return True if the key exists in the hash, false otherwise.
 */
bool AssociativeArray::getTime(const string& key, double& val, double outputScale, double defaultScale) const
{
    if(!getNumber(key, val))
        return false;
    
    double timeScale;
    if(getTimeScale(key, timeScale))
    {
        timeScale /= outputScale;
    }
    else
    {
        timeScale = (defaultScale == 0.0) ? 1.0 : defaultScale / outputScale;
    }
    
    val *= timeScale;
    
    return true;
}


/** @copydoc AssociativeArray::getTime() */
bool AssociativeArray::getTime(const string& key, float& val, double outputScale, double defaultScale) const
{
    double dval;
    
    if(!getLength(key, dval, outputScale, defaultScale))
        return false;
    
    val = ((float) dval);
    
    return true;
}


/**
 * Retrieves a vector quantity scaled to an associated length unit.
 * @param[in] key Hash key for the quantity.
 * @param[out] val The returned vector if present, unaffected if not.
 * @param[in] outputScale Returned value is scaled to this value.
 * @param[in] defaultScale If no units are specified, use this scale. Defaults to outputScale.
 * @return True if the key exists in the hash, false otherwise.
 */
bool AssociativeArray::getLengthVector(const string& key, Eigen::Vector3d& val, double outputScale, double defaultScale) const
{
    if(!getVector(key, val))
        return false;
    
    double lengthScale;
    if(getLengthScale(key, lengthScale))
    {
        lengthScale /= outputScale;
    }
    else
    {
        lengthScale = (defaultScale == 0.0) ? 1.0 : defaultScale / outputScale;
    }
    
    val *= lengthScale;
    
    return true;
}


/** @copydoc AssociativeArray::getLengthVector() */
bool AssociativeArray::getLengthVector(const string& key, Eigen::Vector3f& val, double outputScale, double defaultScale) const
{
    Vector3d vecVal;
    
    if(!getLengthVector(key, vecVal, outputScale, defaultScale))
        return false;
        
    val = vecVal.cast<float>();
    
    return true;
}


/**
 * Retrieves a spherical tuple \verbatim [longitude, latitude, altitude] \endverbatim scaled to associated angle and length units.
 * @param[in] key Hash key for the quantity.
 * @param[out] val The returned tuple in units of degrees and kilometers if present, unaffected if not.
 * @return True if the key exists in the hash, false otherwise.
 */
bool AssociativeArray::getSphericalTuple(const string& key, Vector3d& val) const
{
    if(!getVector(key, val))
        return false;
    
    double angleScale;
    if(getAngleScale(key, angleScale))
    {
        val[0] *= angleScale;
        val[1] *= angleScale;
    }
    
    double lengthScale = 1.0;
    getLengthScale(key, lengthScale);
    val[2] *= lengthScale;
    
    return true;
}


/** @copydoc AssociativeArray::getSphericalTuple */
bool AssociativeArray::getSphericalTuple(const string& key, Vector3f& val) const
{
    Vector3d vecVal;
    
    if(!getSphericalTuple(key, vecVal))
        return false;
    
    val = vecVal.cast<float>();
    
    return true;
}


/**
 * Retrieves the angle unit associated with a given property.
 * @param[in] key Hash key for the property.
 * @param[out] scale The returned angle unit scaled to degrees if present, unaffected if not.
 * @return True if an angle unit has been specified for the property, false otherwise.
 */
bool AssociativeArray::getAngleScale(const string& key, double& scale) const
{
    string unitKey(key + "%Angle");
    string unit;
    
    if (!getString(unitKey, unit))
        return false;
    
    return astro::getAngleScale(unit, scale);
}


/** @copydoc AssociativeArray::getAngleScale() */
bool AssociativeArray::getAngleScale(const string& key, float& scale) const
{
    double dscale;
    if (!getAngleScale(key, dscale))
        return false;
    
    scale = ((float) dscale);
    return true;
}


/**
 * Retrieves the length unit associated with a given property.
 * @param[in] key Hash key for the property.
 * @param[out] scale The returned length unit scaled to kilometers if present, unaffected if not.
 * @return True if a length unit has been specified for the property, false otherwise.
 */
bool AssociativeArray::getLengthScale(const string& key, double& scale) const
{
    string unitKey(key + "%Length");
    string unit;
    
    if (!getString(unitKey, unit))
        return false;
    
    return astro::getLengthScale(unit, scale);
}


/** @copydoc AssociativeArray::getLengthScale() */
bool AssociativeArray::getLengthScale(const string& key, float& scale) const
{
    double dscale;
    if (!getLengthScale(key, dscale))
        return false;
    
    scale = ((float) dscale);
    return true;
}


/**
 * Retrieves the time unit associated with a given property.
 * @param[in] key Hash key for the property.
 * @param[out] scale The returned time unit scaled to days if present, unaffected if not.
 * @return True if a time unit has been specified for the property, false otherwise.
 */
bool AssociativeArray::getTimeScale(const string& key, double& scale) const
{
    string unitKey(key + "%Time");
    string unit;
    
    if (!getString(unitKey, unit))
        return false;
    
    return astro::getTimeScale(unit, scale);
}


/** @copydoc AssociativeArray::getTimeScale() */
bool AssociativeArray::getTimeScale(const string& key, float& scale) const
{
    double dscale;
    if (!getTimeScale(key, dscale))
        return false;
    
    scale = ((float) dscale);
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
