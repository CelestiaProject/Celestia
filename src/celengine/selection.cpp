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
    if (star != NULL)
        return star->getRadius();
    else if (body != NULL)
        return body->getRadius();
    else if (galaxy != NULL)
        return astro::lightYearsToKilometers(galaxy->getRadius());
    else
        return 0.0;
}


UniversalCoord Selection::getPosition(double t) const
{
    if (body != NULL)
    {
        Point3f sunPos(0.0f, 0.0f, 0.0f);
        PlanetarySystem* system = body->getSystem();
        if (system != NULL)
        {
            const Star* sun = system->getStar();
            if (sun != NULL)
                sunPos = sun->getPosition();
        }

        return astro::universalPosition(body->getHeliocentricPosition(t),
                                        sunPos);
    }
    else if (star != NULL)
    {
        return astro::universalPosition(Point3d(0.0, 0.0, 0.0),
                                        star->getPosition());
    }
    else if (galaxy != NULL)
    {
        Point3d p = galaxy->getPosition();
        return astro::universalPosition(Point3d(0.0, 0.0, 0.0),
                                        Point3f((float) p.x, (float) p.y, (float) p.z));
    }
    else
    {
        return UniversalCoord(Point3d(0.0, 0.0, 0.0));
    }
}


string Selection::getName() const
{
    if (star != NULL)
    {
        char buf[20];
        sprintf(buf, "#%d", star->getCatalogNumber());
        return string(buf);
    }
    else if (galaxy != NULL)
    {
        return galaxy->getName();
    }
    else if (body != NULL)
    {
        string name = body->getName();
        PlanetarySystem* system = body->getSystem();
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
    else
    {
        return "";
    }
}
