// selection.cpp
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstdio>
#include "astro.h"
#include "selection.h"

using namespace std;


double Selection::radius() const
{
    switch (type)
    {
    case Type_Star:
        return star()->getRadius();
    case Type_Body:
        return body()->getRadius();
    case Type_DeepSky:
        return astro::lightYearsToKilometers(deepsky()->getRadius());
    case Type_Location:
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
    case Type_Body:
        {
            PlanetarySystem* system = body()->getSystem();
            const Star* sun = NULL;
            if (system != NULL)
                sun = system->getStar();

            Point3d hpos = body()->getHeliocentricPosition(t);
            if (sun != NULL)
                return astro::universalPosition(hpos, sun->getPosition(t));
            else
                return astro::universalPosition(hpos, Point3f(0.0f, 0.0f, 0.0f));
                                                
        }
        
    case Type_Star:
        return star()->getPosition(t);

    case Type_DeepSky:
        {
            Point3d p = deepsky()->getPosition();
            return astro::universalPosition(Point3d(0.0, 0.0, 0.0),
                                            Point3f((float) p.x, (float) p.y, (float) p.z));
        }
        
    case Type_Location:
        {
            Point3f sunPos(0.0f, 0.0f, 0.0f);
            Body* body = location()->getParentBody();
            if (body != NULL && body->getSystem() != NULL)
            {
                const Star* sun = body->getSystem()->getStar();
                if (sun != NULL)
                    sunPos = sun->getPosition();
            }

            return astro::universalPosition(location()->getHeliocentricPosition(t),
                                            sunPos);
        }

    default:
        return UniversalCoord(Point3d(0.0, 0.0, 0.0));
    }
}


string Selection::getName() const
{
    switch (type)
    {
    case Type_Star:
        {
            char buf[20];
            sprintf(buf, "#%d", star()->getCatalogNumber());
            return string(buf);
        }

    case Type_DeepSky:
        {
            char buf[20];
            sprintf(buf, "#%d", deepsky()->getCatalogNumber());
            return string(buf);
        }
        
    case Type_Body:
        {
            string name = body()->getName();
            PlanetarySystem* system = body()->getSystem();
            while (system != NULL)
            {
                Body* parent = system->getPrimaryBody();
                if (parent != NULL)
                {
                    name = parent->getName() + '/' + name;
                    system = parent->getSystem();
                }
                else
                {
                    const Star* parentStar = system->getStar();
                    if (parentStar != NULL)
                    {
                        char buf[20];
                        sprintf(buf, "#%d", parentStar->getCatalogNumber());
                        name = string(buf) + '/' + name;
                    }
                    system = NULL;
                }
            }
            return name;
        }

    case Type_Location:
        if (location()->getParentBody() == NULL)
        {
            return location()->getName();
        }
        else
        {
            return Selection(location()->getParentBody()).getName() + '/' +
                location()->getName();
        }

    default:
        return "";
    }
}


Selection Selection::parent() const
{
    switch (type)
    {
    case Type_Location:
        return Selection(location()->getParentBody());

    case Type_Body:
        if (body()->getSystem())
        {
            if (body()->getSystem()->getPrimaryBody() != NULL)
                return Selection(body()->getSystem()->getPrimaryBody());
            else
                return Selection(body()->getSystem()->getStar());
        }
        else
        {
            return Selection();
        }
        break;

    case Type_Star:
    case Type_DeepSky:
        // Currently no hierarchy for stars and deep sky objects.
        return Selection();

    default:
        return Selection();
    }
}
