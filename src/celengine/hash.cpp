// hash.cpp
//
// Copyright (C) 2001-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celmath/mathlib.h>
#include <celutil/color.h>
#include <celutil/fsutils.h>
#include "astro.h"
#include "hash.h"
#include "value.h"

namespace celutil = celestia::util;


// Define these here: at declaration the vector member contains an incomplete type
AssociativeArray::~AssociativeArray() = default;
AssociativeArray::AssociativeArray(AssociativeArray&&) = default;
AssociativeArray& AssociativeArray::operator=(AssociativeArray&&) = default;


const Value* AssociativeArray::getValue(std::string_view key) const
{
    auto iter = assoc.find(key);
    if (iter == assoc.end())
        return nullptr;

    return &values[iter->second];
}


void AssociativeArray::addValue(std::string&& key, Value&& val)
{
    std::size_t index = values.size();
    values.emplace_back(std::move(val));
    assoc.emplace(std::move(key), index);
}


bool AssociativeArray::getNumber(std::string_view key, double& val) const
{
    const Value* v = getValue(key);
    if (v == nullptr) { return false; }
    std::optional<double> dbl = v->getNumber();
    if (!dbl.has_value()) { return false; }

    val = *dbl;
    return true;
}


bool AssociativeArray::getNumber(std::string_view key, float& val) const
{
    double dval;

    if (!getNumber(key, dval))
        return false;

    val = (float) dval;
    return true;
}


bool AssociativeArray::getNumber(std::string_view key, int& val) const
{
    double ival;

    if (!getNumber(key, ival))
        return false;

    val = (int) ival;
    return true;
}


bool AssociativeArray::getNumber(std::string_view key, std::uint32_t& val) const
{
    double ival;

    if (!getNumber(key, ival))
        return false;

    val = static_cast<std::uint32_t>(ival);
    return true;
}


bool AssociativeArray::getString(std::string_view key, std::string& val) const
{
    const Value* v = getValue(key);
    if (v == nullptr) { return false; }

    const std::string* str = v->getString();
    if (str == nullptr) { return false; }

    val = *str;
    return true;
}


bool AssociativeArray::getPath(std::string_view key, fs::path& val) const
{
    std::string v;
    if (getString(key, v))
    {
        val = celutil::PathExp(v);
        return true;
    }
    return false;
}


bool AssociativeArray::getBoolean(std::string_view key, bool& val) const
{
    const Value* v = getValue(key);
    if (v == nullptr) { return false; }
    auto bln = v->getBoolean();
    if (!bln.has_value()) { return false;}

    val = *bln;
    return true;
}


bool AssociativeArray::getVector(std::string_view key, Eigen::Vector3d& val) const
{
    const Value* v = getValue(key);
    if (v == nullptr) { return false; }

    const ValueArray* arr = v->getArray();
    if (arr == nullptr || arr->size() != 3) { return false; }

    std::optional<double> x = (*arr)[0].getNumber();
    std::optional<double> y = (*arr)[1].getNumber();
    std::optional<double> z = (*arr)[2].getNumber();

    if (!x.has_value() || !y.has_value() || !z.has_value()) { return false; }

    val = Eigen::Vector3d(*x, *y, *z);
    return true;
}


bool AssociativeArray::getVector(std::string_view key, Eigen::Vector3f& val) const
{
    Eigen::Vector3d vecVal;

    if (!getVector(key, vecVal))
        return false;

    val = vecVal.cast<float>();
    return true;
}


bool AssociativeArray::getVector(std::string_view key, Eigen::Vector4d& val) const
{
    const Value* v = getValue(key);
    if (v == nullptr) { return false; }

    const ValueArray* arr = v->getArray();
    if (arr == nullptr || arr->size() != 4) { return false; }

    std::optional<double> x = (*arr)[0].getNumber();
    std::optional<double> y = (*arr)[1].getNumber();
    std::optional<double> z = (*arr)[2].getNumber();
    std::optional<double> w = (*arr)[3].getNumber();

    if (!x.has_value() || !y.has_value() || !z.has_value() || !w.has_value())
    {
        return false;
    }

    val = Eigen::Vector4d(*x, *y, *z, *w);
    return true;
}


bool AssociativeArray::getVector(std::string_view key, Eigen::Vector4f& val) const
{
    Eigen::Vector4d vecVal;

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
bool AssociativeArray::getRotation(std::string_view key, Eigen::Quaternionf& val) const
{
    const Value* v = getValue(key);
    if (v == nullptr ) { return false; }

    const ValueArray* arr = v->getArray();
    if (arr == nullptr || arr->size() != 4) { return false; }

    std::optional<double> w = (*arr)[0].getNumber();
    std::optional<double> x = (*arr)[1].getNumber();
    std::optional<double> y = (*arr)[2].getNumber();
    std::optional<double> z = (*arr)[3].getNumber();

    if (!w.has_value() || !x.has_value() || !y.has_value() || !z.has_value())
    {
        return false;
    }

    Eigen::Vector3f axis(static_cast<float>(*x),
                         static_cast<float>(*y),
                         static_cast<float>(*z));

    double angScale = astro::getAngleScale(v->getAngleUnit()).value_or(1.0);
    auto angle = static_cast<float>(celmath::degToRad(*w * angScale));

    val = Eigen::Quaternionf(Eigen::AngleAxisf(angle, axis.normalized()));

    return true;
}


bool AssociativeArray::getColor(std::string_view key, Color& val) const
{
    Eigen::Vector4d vec4;
    if (getVector(key, vec4))
    {
        Eigen::Vector4f vec4f = vec4.cast<float>();
        val = Color(vec4f);
        return true;
    }

    Eigen::Vector3d vec3;
    if (getVector(key, vec3))
    {
        Eigen::Vector3f vec3f = vec3.cast<float>();
        val = Color(vec3f);
        return true;
    }

    std::string rgba;
    if (getString(key, rgba))
    {
        return Color::parse(rgba.c_str(), val);
    }

    return false;
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
AssociativeArray::getAngle(std::string_view key, double& val, double outputScale, double defaultScale) const
{
    const Value* v = getValue(key);
    if (v == nullptr) { return false; }

    if (std::optional<double> dbl = v->getNumber(); dbl.has_value())
    {
        val = *dbl;
    }
    else
    {
        return false;
    }

    if(auto angleScale = astro::getAngleScale(v->getAngleUnit()); angleScale.has_value())
    {
        val *= *angleScale / outputScale;
    }
    else if (defaultScale != 0.0)
    {
        val *= defaultScale / outputScale;
    }

    return true;
}


/** @copydoc AssociativeArray::getAngle() */
bool
AssociativeArray::getAngle(std::string_view key, float& val, double outputScale, double defaultScale) const
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
AssociativeArray::getLength(std::string_view key, double& val, double outputScale, double defaultScale) const
{
    const Value* v = getValue(key);
    if (v == nullptr) { return false; }

    if (std::optional<double> dbl = v->getNumber(); dbl.has_value())
    {
        val = *dbl;
    }
    else
    {
        return false;
    }

    if (auto lengthScale = astro::getLengthScale(v->getLengthUnit()); lengthScale.has_value())
    {
        val *= *lengthScale / outputScale;
    }
    else if (defaultScale != 0.0)
    {
        val *= defaultScale / outputScale;
    }

    return true;
}


/** @copydoc AssociativeArray::getLength() */
bool AssociativeArray::getLength(std::string_view key, float& val, double outputScale, double defaultScale) const
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
bool AssociativeArray::getTime(std::string_view key, double& val, double outputScale, double defaultScale) const
{
    const Value* v = getValue(key);
    if (v == nullptr) { return false; }

    if (std::optional<double> dbl = v->getNumber(); dbl.has_value())
    {
        val = *dbl;
    }
    else
    {
        return false;
    }

    if (auto timeScale = astro::getTimeScale(v->getTimeUnit()))
    {
        val *= *timeScale / outputScale;
    }
    else if (defaultScale != 0.0)
    {
        val *= defaultScale / outputScale;
    }

    return true;
}


/** @copydoc AssociativeArray::getTime() */
bool AssociativeArray::getTime(std::string_view key, float& val, double outputScale, double defaultScale) const
{
    double dval;

    if(!getTime(key, dval, outputScale, defaultScale))
        return false;

    val = ((float) dval);

    return true;
}


/**
 * Retrieves a numeric quantity scaled to an associated mass unit.
 * @param[in] key Hash key for the quantity.
 * @param[out] val The returned quantity if present, unaffected if not.
 * @param[in] outputScale Returned value is scaled to this value.
 * @param[in] defaultScale If no units are specified, use this scale. Defaults to outputScale.
 * @return True if the key exists in the hash, false otherwise.
 */
bool AssociativeArray::getMass(std::string_view key, double& val, double outputScale, double defaultScale) const
{
    const Value* v = getValue(key);
    if (v == nullptr) { return false; }

    if (std::optional<double> dbl = v->getNumber(); dbl.has_value())
    {
        val = *dbl;
    }
    else
    {
        return false;
    }

    if (auto massScale = astro::getMassScale(v->getMassUnit()); massScale.has_value())
    {
        val *= *massScale / outputScale;
    }
    else if (defaultScale != 0.0)
    {
        val *= defaultScale / outputScale;
    }

    return true;
}


/** @copydoc AssociativeArray::getMass() */
bool AssociativeArray::getMass(std::string_view key, float& val, double outputScale, double defaultScale) const
{
    double dval;

    if(!getMass(key, dval, outputScale, defaultScale))
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
bool AssociativeArray::getLengthVector(std::string_view key, Eigen::Vector3d& val, double outputScale, double defaultScale) const
{
    const Value* v = getValue(key);
    if (v == nullptr) { return false; }

    const ValueArray* arr = v->getArray();
    if (arr == nullptr || arr->size() != 3) { return false; }

    std::optional<double> x = (*arr)[0].getNumber();
    std::optional<double> y = (*arr)[1].getNumber();
    std::optional<double> z = (*arr)[2].getNumber();

    if (!x.has_value() || !y.has_value() || !z.has_value()) { return false; }

    val = Eigen::Vector3d(*x, *y, *z);

    if (auto lengthScale = astro::getLengthScale(v->getLengthUnit()); lengthScale.has_value())
    {
        val *= *lengthScale / outputScale;
    }
    else if (defaultScale != 0.0)
    {
        val *= defaultScale / outputScale;
    }

    return true;
}


/** @copydoc AssociativeArray::getLengthVector() */
bool AssociativeArray::getLengthVector(std::string_view key, Eigen::Vector3f& val, double outputScale, double defaultScale) const
{
    Eigen::Vector3d vecVal;

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
bool AssociativeArray::getSphericalTuple(std::string_view key, Eigen::Vector3d& val) const
{
    const Value* v = getValue(key);
    if (v == nullptr) { return false; }

    const ValueArray* arr = v->getArray();
    if (arr == nullptr || arr->size() != 3)
        return false;

    std::optional<double> x = (*arr)[0].getNumber();
    std::optional<double> y = (*arr)[1].getNumber();
    std::optional<double> z = (*arr)[2].getNumber();

    if (!x.has_value() || !y.has_value() || !z.has_value())
        return false;

    val = Eigen::Vector3d(*x, *y, *z);

    if (auto angleScale = astro::getAngleScale(v->getAngleUnit()); angleScale.has_value())
    {
        val.head<2>() *= *angleScale;
    }

    if (auto lengthScale = astro::getLengthScale(v->getLengthUnit()); lengthScale.has_value())
    {
        val[2] *= *lengthScale;
    }

    return true;
}


/** @copydoc AssociativeArray::getSphericalTuple */
bool AssociativeArray::getSphericalTuple(std::string_view key, Eigen::Vector3f& val) const
{
    Eigen::Vector3d vecVal;

    if(!getSphericalTuple(key, vecVal))
        return false;

    val = vecVal.cast<float>();
    return true;
}
