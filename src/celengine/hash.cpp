// hash.cpp
//
// Copyright (C) 2001-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celutil/color.h>
#include <celmath/mathlib.h>
#include <celengine/astro.h>
#include <celengine/hash.h>
#include <celengine/value.h>

using namespace Eigen;
using namespace std;
using namespace celmath;

namespace celestia
{
namespace engine
{


AssociativeArray::~AssociativeArray()
{
    for (const auto &t : assoc)
        delete t.second;
}

Value* AssociativeArray::getValue(const string& key) const
{
    auto iter = assoc.find(key);
    if (iter == assoc.end())
        return nullptr;

    return iter->second;
}

void AssociativeArray::addValue(const string& key, Value& val)
{
    assoc.insert(map<string, Value*>::value_type(key, &val));
}

bool AssociativeArray::getNumber(const string& key, double& val) const
{
    Value* v = getValue(key);
    if (v == nullptr || v->getType() != Value::NumberType)
        return false;

    val = v->getNumber();
    return true;
}

bool AssociativeArray::getNumber(const string& key, float& val) const
{
    double dval;

    if (!getNumber(key, dval))
        return false;

    val = (float) dval;
    return true;
}

bool AssociativeArray::getNumber(const string& key, int& val) const
{
    double ival;

    if (!getNumber(key, ival))
        return false;

    val = (int) ival;
    return true;
}

bool AssociativeArray::getNumber(const string& key, uint32_t& val) const
{
    double ival;

    if (!getNumber(key, ival))
        return false;

    val = (uint32_t) ival;
    return true;
}

bool AssociativeArray::getString(const string& key, string& val) const
{
    Value* v = getValue(key);
    if (v == nullptr || v->getType() != Value::StringType)
        return false;

    val = v->getString();
    return true;
}

bool AssociativeArray::getBoolean(const string& key, bool& val) const
{
    Value* v = getValue(key);
    if (v == nullptr || v->getType() != Value::BooleanType)
        return false;

    val = v->getBoolean();
    return true;
}

bool AssociativeArray::getVector(const string& key, Vector3d& val) const
{
    Value* v = getValue(key);
    if (v == nullptr || v->getType() != Value::ArrayType)
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


bool AssociativeArray::getVector(const string& key, Vector3f& val) const
{
    Vector3d vecVal;

    if (!getVector(key, vecVal))
        return false;

    val = vecVal.cast<float>();
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
    if (v == nullptr || v->getType() != Value::ArrayType)
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
    Vector3d vecVal;

    if (!getVector(key, vecVal))
        return false;

    val = Color((float) vecVal.x(), (float) vecVal.y(), (float) vecVal.z());

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

}
}
