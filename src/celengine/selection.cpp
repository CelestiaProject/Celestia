// selection.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "selection.h"

#include <cassert>

#include <fmt/format.h>

#include <celengine/star.h>
#include <celengine/body.h>
#include <celengine/location.h>
#include <celengine/deepskyobj.h>

namespace
{
// Some velocities are computed by differentiation; units
// are Julian days.
constexpr double VELOCITY_DIFF_DELTA = 1.0 / 1440.0;


UniversalCoord locationPosition(const Location* location, double t)
{
    if (const Body* body = location->getParentBody(); body != nullptr)
        return body->getPosition(t).offsetKm(location->getPlanetocentricPosition(t));

    // Bad location; all locations should have a parent.
    assert(0);
    return UniversalCoord::Zero();
}


std::string bodyName(const Body* body, bool i18n)
{
    std::string name = body->getName(i18n);
    const PlanetarySystem* system = body->getSystem();
    while (system != nullptr)
    {
        if (const Body* parent = system->getPrimaryBody(); parent != nullptr)
        {
            name = parent->getName(i18n) + '/' + name;
            system = parent->getSystem();
        }
        else
        {
            if (const Star* parentStar = system->getStar(); parentStar != nullptr)
                name = fmt::format("#{}/{}", parentStar->getIndex(), name);
            system = nullptr;
        }
    }

    return name;
}

} // end unnamed namespace


double
Selection::radius() const
{
    switch (type)
    {
    case SelectionType::Star:
        return static_cast<const Star*>(obj)->getRadius();
    case SelectionType::Body:
        return static_cast<const Body*>(obj)->getRadius();
    case SelectionType::DeepSky:
        return astro::lightYearsToKilometers(static_cast<const DeepSkyObject*>(obj)->getRadius());
    case SelectionType::Location:
        // The size of a location is its diameter, so divide by 2.
        return static_cast<const Location*>(obj)->getSize() / 2.0f;
    default:
        return 0.0;
    }
}


UniversalCoord
Selection::getPosition(double t) const
{
    switch (type)
    {
    case SelectionType::Star:
        return static_cast<const Star*>(obj)->getPosition(t);

    case SelectionType::Body:
        return static_cast<const Body*>(obj)->getPosition(t);

    case SelectionType::DeepSky:
        {
            // NOTE: cast to single precision is only present to maintain compatibility with
            // Celestia 1.6.0.
            Eigen::Vector3f p = static_cast<const DeepSkyObject*>(obj)->getPosition().cast<float>();
            return UniversalCoord::CreateLy(p.cast<double>());
        }

    case SelectionType::Location:
        return locationPosition(static_cast<const Location*>(obj), t);

    default:
        return UniversalCoord::Zero();
    }
}


Eigen::Vector3d
Selection::getVelocity(double t) const
{
    switch (type)
    {
    case SelectionType::Star:
        return static_cast<const Star*>(obj)->getVelocity(t);

    case SelectionType::Body:
        return static_cast<const Body*>(obj)->getVelocity(t);

    case SelectionType::DeepSky:
        return Eigen::Vector3d::Zero();

    case SelectionType::Location:
        {
            // For now, just use differentiation for location velocities.
            auto location = static_cast<const Location*>(obj);
            return locationPosition(location, t)
                .offsetFromKm(locationPosition(location, t - VELOCITY_DIFF_DELTA)) / VELOCITY_DIFF_DELTA;
        }

    default:
        return Eigen::Vector3d::Zero();
    }
}


std::string
Selection::getName(bool i18n) const
{
    switch (type)
    {
    case SelectionType::Star:
        return fmt::format("#{}", static_cast<const Star*>(obj)->getIndex());

    case SelectionType::Body:
        return bodyName(static_cast<const Body*>(obj), i18n);

    case SelectionType::DeepSky:
        return fmt::format("#{}", static_cast<const DeepSkyObject*>(obj)->getIndex());

    case SelectionType::Location:
        {
            auto location = static_cast<const Location*>(obj);
            if (auto parentBody = location->getParentBody(); parentBody == nullptr)
                return location->getName(i18n);
            else
                return fmt::format("{}/{}", bodyName(parentBody, i18n), location->getName(i18n));
        }

    default:
        return {};
    }
}


Selection Selection::parent() const
{
    switch (type)
    {
    case SelectionType::Star:
        return Selection(static_cast<const Star*>(obj)->getOrbitBarycenter());

    case SelectionType::Body:
        if (auto system = static_cast<const Body*>(obj)->getSystem(); system != nullptr)
        {
            if (auto primaryBody = system->getPrimaryBody(); primaryBody != nullptr)
                return Selection(primaryBody);

            return Selection(system->getStar());
        }

        return Selection();

    case SelectionType::DeepSky:
        // Currently no hierarchy for stars and deep sky objects.
        return Selection();

    case SelectionType::Location:
        return Selection(static_cast<const Location*>(obj)->getParentBody());

    default:
        return Selection();
    }
}


/*! Return true if the selection's visibility flag is set. */
bool Selection::isVisible() const
{
    switch (type)
    {
    case SelectionType::Star:
        return static_cast<const Star*>(obj)->getVisibility();
    case SelectionType::Body:
        return static_cast<const Body*>(obj)->isVisible();
    case SelectionType::DeepSky:
        return static_cast<const DeepSkyObject*>(obj)->isVisible();
    default:
        return false;
    }
}
