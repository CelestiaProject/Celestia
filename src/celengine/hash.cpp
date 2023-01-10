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
AssociativeArray::AssociativeArray(AssociativeArray&&) noexcept(std::is_nothrow_move_constructible_v<AssocType>) = default;
AssociativeArray& AssociativeArray::operator=(AssociativeArray&&) noexcept(std::is_nothrow_move_assignable_v<AssocType>) = default;


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
    assoc.try_emplace(std::move(key), index);
}


std::optional<double> AssociativeArray::getNumberImpl(std::string_view key) const
{
    const Value* v = getValue(key);
    if (v == nullptr) { return std::nullopt; }

    return v->getNumber();
}


const std::string* AssociativeArray::getString(std::string_view key) const
{
    const Value* v = getValue(key);
    if (v == nullptr) { return nullptr; }

    return v->getString();
}


std::optional<fs::path> AssociativeArray::getPath(std::string_view key) const
{
    const std::string* v = getString(key);
    if (v == nullptr) { return std::nullopt; }

    return std::make_optional(celutil::PathExp(*v));
}


std::optional<bool> AssociativeArray::getBoolean(std::string_view key) const
{
    const Value* v = getValue(key);
    if (v == nullptr) { return std::nullopt; }

    return v->getBoolean();
}


std::optional<Eigen::Vector3d>
AssociativeArray::getVector3Impl(std::string_view key) const
{
    const Value* v = getValue(key);
    if (v == nullptr) { return std::nullopt; }

    const ValueArray* arr = v->getArray();
    if (arr == nullptr || arr->size() != 3) { return std::nullopt; }

    std::optional<double> x = (*arr)[0].getNumber();
    std::optional<double> y = (*arr)[1].getNumber();
    std::optional<double> z = (*arr)[2].getNumber();

    if (!x.has_value() || !y.has_value() || !z.has_value()) { return std::nullopt; }

    return std::make_optional<Eigen::Vector3d>(*x, *y, *z);
}


std::optional<Eigen::Vector4d>
AssociativeArray::getVector4Impl(std::string_view key) const
{
    const Value* v = getValue(key);
    if (v == nullptr) { return std::nullopt; }

    const ValueArray* arr = v->getArray();
    if (arr == nullptr || arr->size() != 4) { return std::nullopt; }

    std::optional<double> x = (*arr)[0].getNumber();
    std::optional<double> y = (*arr)[1].getNumber();
    std::optional<double> z = (*arr)[2].getNumber();
    std::optional<double> w = (*arr)[3].getNumber();

    if (!x.has_value() || !y.has_value() || !z.has_value() || !w.has_value())
    {
        return std::nullopt;
    }

    return std::make_optional<Eigen::Vector4d>(*x, *y, *z, *w);
}


/**
 * Retrieves a quaternion, scaled to an associated angle unit.
 *
 * The quaternion is specified in the catalog file in axis-angle format as follows:
 * \verbatim {PropertyName} [ angle axisX axisY axisZ ] \endverbatim
 *
 * @param[in] key Hash key for the rotation.
 * @return The quaternion if the key exists, nullopt otherwise.
 */
std::optional<Eigen::Quaternionf> AssociativeArray::getRotation(std::string_view key) const
{
    const Value* v = getValue(key);
    if (v == nullptr ) { return std::nullopt; }

    const ValueArray* arr = v->getArray();
    if (arr == nullptr || arr->size() != 4) { return std::nullopt; }

    std::optional<double> w = (*arr)[0].getNumber();
    std::optional<double> x = (*arr)[1].getNumber();
    std::optional<double> y = (*arr)[2].getNumber();
    std::optional<double> z = (*arr)[3].getNumber();

    if (!w.has_value() || !x.has_value() || !y.has_value() || !z.has_value())
    {
        return std::nullopt;
    }

    Eigen::Vector3f axis(static_cast<float>(*x),
                         static_cast<float>(*y),
                         static_cast<float>(*z));

    double angScale = astro::getAngleScale(v->getAngleUnit()).value_or(1.0);
    auto angle = static_cast<float>(celmath::degToRad(*w * angScale));

    return std::make_optional<Eigen::Quaternionf>(Eigen::AngleAxisf(angle, axis.normalized()));
}


std::optional<Color> AssociativeArray::getColor(std::string_view key) const
{
    if (auto vec4 = getVector4<float>(key); vec4.has_value())
    {
        return std::make_optional<Color>(*vec4);
    }

    if (auto vec3 = getVector3<float>(key); vec3.has_value())
    {
        return std::make_optional<Color>(*vec3);
    }

    if (const std::string* rgba = getString(key); rgba != nullptr)
    {
        Color color;
        if (Color::parse(*rgba, color))
        {
            return color;
        }
    }

    return std::nullopt;
}


/**
 * Retrieves a numeric quantity scaled to an associated angle unit.
 * @param[in] key Hash key for the quantity.
 * @param[in] outputScale Returned value is scaled to this value.
 * @param[in] defaultScale If no units are specified, use this scale. Defaults to outputScale.
 * @return The scaled value if the key exists in the hash, nullopt otherwise.
 */
std::optional<double>
AssociativeArray::getAngleImpl(std::string_view key, double outputScale, double defaultScale) const
{
    const Value* v = getValue(key);
    if (v == nullptr) { return std::nullopt; }

    double val;
    if (std::optional<double> dbl = v->getNumber(); dbl.has_value())
    {
        val = *dbl;
    }
    else
    {
        return std::nullopt;
    }

    if(auto angleScale = astro::getAngleScale(v->getAngleUnit()); angleScale.has_value())
    {
        val *= *angleScale / outputScale;
    }
    else if (defaultScale != 0.0)
    {
        val *= defaultScale / outputScale;
    }

    return val;
}


/**
 * Retrieves a numeric quantity scaled to an associated length unit.
 * @param[in] key Hash key for the quantity.
 * @param[in] outputScale Returned value is scaled to this value.
 * @param[in] defaultScale If no units are specified, use this scale. Defaults to outputScale.
 * @return The scaled value if the key exists in the hash, otherwise nullopt.
 */
std::optional<double>
AssociativeArray::getLengthImpl(std::string_view key, double outputScale, double defaultScale) const
{
    const Value* v = getValue(key);
    if (v == nullptr) { return std::nullopt; }

    double val;
    if (std::optional<double> dbl = v->getNumber(); dbl.has_value())
    {
        val = *dbl;
    }
    else
    {
        return std::nullopt;
    }

    if (auto lengthScale = astro::getLengthScale(v->getLengthUnit()); lengthScale.has_value())
    {
        val *= *lengthScale / outputScale;
    }
    else if (defaultScale != 0.0)
    {
        val *= defaultScale / outputScale;
    }

    return val;
}


/**
 * Retrieves a numeric quantity scaled to an associated time unit.
 * @param[in] key Hash key for the quantity.
 * @param[in] outputScale Returned value is scaled to this value.
 * @param[in] defaultScale If no units are specified, use this scale. Defaults to outputScale.
 * @return The scaled value if the key exists in the hash, nullopt otherwise.
 */
std::optional<double>
AssociativeArray::getTimeImpl(std::string_view key, double outputScale, double defaultScale) const
{
    const Value* v = getValue(key);
    if (v == nullptr) { return std::nullopt; }

    double val;
    if (std::optional<double> dbl = v->getNumber(); dbl.has_value())
    {
        val = *dbl;
    }
    else
    {
        return std::nullopt;
    }

    if (auto timeScale = astro::getTimeScale(v->getTimeUnit()))
    {
        val *= *timeScale / outputScale;
    }
    else if (defaultScale != 0.0)
    {
        val *= defaultScale / outputScale;
    }

    return val;
}


/**
 * Retrieves a numeric quantity scaled to an associated mass unit.
 * @param[in] key Hash key for the quantity.
 * @param[in] outputScale Returned value is scaled to this value.
 * @param[in] defaultScale If no units are specified, use this scale. Defaults to outputScale.
 * @return The scaled value if the key exists in the hash, nullopt otherwise.
 */
std::optional<double>
AssociativeArray::getMassImpl(std::string_view key, double outputScale, double defaultScale) const
{
    const Value* v = getValue(key);
    if (v == nullptr) { return std::nullopt; }

    double val;
    if (std::optional<double> dbl = v->getNumber(); dbl.has_value())
    {
        val = *dbl;
    }
    else
    {
        return std::nullopt;
    }

    if (auto massScale = astro::getMassScale(v->getMassUnit()); massScale.has_value())
    {
        val *= *massScale / outputScale;
    }
    else if (defaultScale != 0.0)
    {
        val *= defaultScale / outputScale;
    }

    return val;
}


/**
 * Retrieves a vector quantity scaled to an associated length unit.
 * @param[in] key Hash key for the quantity.
 * @param[in] outputScale Returned value is scaled to this value.
 * @param[in] defaultScale If no units are specified, use this scale. Defaults to outputScale.
 * @return The scaled vector if the key exists in the hash, nullopt otherwise.
 */
std::optional<Eigen::Vector3d>
AssociativeArray::getLengthVectorImpl(std::string_view key, double outputScale, double defaultScale) const
{
    const Value* v = getValue(key);
    if (v == nullptr) { return std::nullopt; }

    const ValueArray* arr = v->getArray();
    if (arr == nullptr || arr->size() != 3) { return std::nullopt; }

    std::optional<double> x = (*arr)[0].getNumber();
    std::optional<double> y = (*arr)[1].getNumber();
    std::optional<double> z = (*arr)[2].getNumber();

    if (!x.has_value() || !y.has_value() || !z.has_value()) { return std::nullopt; }

    if (auto lengthScale = astro::getLengthScale(v->getLengthUnit()); lengthScale.has_value())
    {
        auto scale = *lengthScale / outputScale;
        return std::make_optional<Eigen::Vector3d>(*x * scale, *y * scale, *z * scale);
    }

    if (defaultScale != 0.0)
    {
        auto scale = defaultScale / outputScale;
        return std::make_optional<Eigen::Vector3d>(*x * scale, *y * scale, *z * scale);
    }

    return std::make_optional<Eigen::Vector3d>(*x, *y, *z);
}


/**
 * Retrieves a spherical tuple \verbatim [longitude, latitude, altitude] \endverbatim scaled to associated angle and length units.
 * @return The tuple if the key exists in the hash, nullopt otherwise.
 */
std::optional<Eigen::Vector3d>
AssociativeArray::getSphericalTuple(std::string_view key) const
{
    const Value* v = getValue(key);
    if (v == nullptr) { return std::nullopt; }

    const ValueArray* arr = v->getArray();
    if (arr == nullptr || arr->size() != 3) { return std::nullopt; }

    std::optional<double> x = (*arr)[0].getNumber();
    std::optional<double> y = (*arr)[1].getNumber();
    std::optional<double> z = (*arr)[2].getNumber();

    if (!x.has_value() || !y.has_value() || !z.has_value()) { return std::nullopt; }

    if (auto angleScale = astro::getAngleScale(v->getAngleUnit()); angleScale.has_value())
    {
        *x *= *angleScale;
        *y *= *angleScale;
    }

    if (auto lengthScale = astro::getLengthScale(v->getLengthUnit()); lengthScale.has_value())
    {
        *z *= *lengthScale;
    }

    return std::make_optional<Eigen::Vector3d>(*x, *y, *z);
}
