// selection.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include <fmt/format.h>
#include "astro.h"
#include "selection.h"
#include "frametree.h"
#include <celengine/star.h>
#include <celengine/body.h>
#include <celengine/location.h>
#include <celengine/deepskyobj.h>

using namespace Eigen;
using namespace std;


// Some velocities are computed by differentiation; units
// are Julian days.
static const double VELOCITY_DIFF_DELTA = 1.0 / 1440.0;


double Selection::radius() const
{
    switch (type)
    {
    case SelectionType::Star:
        return star()->getRadius();
    case SelectionType::Body:
        return body()->getRadius();
    case SelectionType::DeepSky:
        return astro::lightYearsToKilometers(deepsky()->getRadius());
    case SelectionType::Location:
        // The size of a location is its diameter, so divide by 2.
        return location()->getSize() / 2.0f;
    default:
        return 0.0;
    }
}


UniversalCoord Selection::getPosition(double t) const
{
    switch (type)
    {
    case SelectionType::Body:
        return body()->getPosition(t);

    case SelectionType::Star:
        return star()->getPosition(t);

    case SelectionType::DeepSky:
        {
            // NOTE: cast to single precision is only present to maintain compatibility with
            // Celestia 1.6.0.
            Vector3f p = deepsky()->getPosition().cast<float>();
            return UniversalCoord::CreateLy(p.cast<double>());
        }

    case SelectionType::Location:
        {
            Body* body = location()->getParentBody();
            if (body != nullptr)
            {
                return body->getPosition(t).offsetKm(location()->getPlanetocentricPosition(t));
            }
            else
            {
                // Bad location; all locations should have a parent.
                assert(0);
                return UniversalCoord::Zero();
            }
        }

    default:
        return UniversalCoord::Zero();
    }
}


Vector3d Selection::getVelocity(double t) const
{
    switch (type)
    {
    case SelectionType::Body:
        return body()->getVelocity(t);

    case SelectionType::Star:
        return star()->getVelocity(t);

    case SelectionType::DeepSky:
        return Vector3d::Zero();

    case SelectionType::Location:
        {
            // For now, just use differentiation for location velocities.
            return getPosition(t).offsetFromKm(getPosition(t - VELOCITY_DIFF_DELTA)) / VELOCITY_DIFF_DELTA;
        }

    default:
        return Vector3d::Zero();
    }
}


string Selection::getName(bool i18n) const
{
    switch (type)
    {
    case SelectionType::Star:
        return fmt::format("#{}", star()->getIndex());

    case SelectionType::DeepSky:
        return fmt::format("#{}", deepsky()->getIndex());

    case SelectionType::Body:
        {
            string name = body()->getName(i18n);
            PlanetarySystem* system = body()->getSystem();
            while (system != nullptr)
            {
                Body* parent = system->getPrimaryBody();
                if (parent != nullptr)
                {
                    name = parent->getName(i18n) + '/' + name;
                    system = parent->getSystem();
                }
                else
                {
                    const Star* parentStar = system->getStar();
                    if (parentStar != nullptr)
                        name = fmt::format("#{}/{}", parentStar->getIndex(), name);
                    system = nullptr;
                }
            }
            return name;
        }

    case SelectionType::Location:
        if (location()->getParentBody() == nullptr)
        {
            return location()->getName(i18n);
        }
        else
        {
            return Selection(location()->getParentBody()).getName(i18n) + '/' +
                location()->getName(i18n);
        }

    default:
        return "";
    }
}


Selection Selection::parent() const
{
    switch (type)
    {
    case SelectionType::Location:
        return Selection(location()->getParentBody());

    case SelectionType::Body:
        if (body()->getSystem())
        {
            if (body()->getSystem()->getPrimaryBody() != nullptr)
                return Selection(body()->getSystem()->getPrimaryBody());
            else
                return Selection(body()->getSystem()->getStar());
        }
        else
        {
            return Selection();
        }
        break;

    case SelectionType::Star:
        return Selection(star()->getOrbitBarycenter());

    case SelectionType::DeepSky:
        // Currently no hierarchy for stars and deep sky objects.
        return Selection();

    default:
        return Selection();
    }
}


/*! Return true if the selection's visibility flag is set. */
bool Selection::isVisible() const
{
    switch (type)
    {
    case SelectionType::Body:
        return body()->isVisible();
    case SelectionType::Star:
        return true;
    case SelectionType::DeepSky:
        return deepsky()->isVisible();
    default:
        return false;
    }
}
