// galaxy.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "mathlib.h"
#include "galaxy.h"
#include "perlin.h"

using namespace std;

static bool formsInitialized = false;
static GalacticForm* spiralForm = NULL;
static GalacticForm* ellipticalForm = NULL;
static GalacticForm* irregularForm = NULL;
static void InitializeForms();


Galaxy::Galaxy() :
    name(""),
    position(0, 0, 0), orientation(1), radius(1),
    form(NULL)
{
}


string Galaxy::getName() const
{
    return name;
}

void Galaxy::setName(const string& s)
{
    name = s;
}

Point3d Galaxy::getPosition() const
{
    return position;
}

void Galaxy::setPosition(Point3d p)
{
    position = p;
}

Quatf Galaxy::getOrientation() const
{
    return orientation;
}

void Galaxy::setOrientation(Quatf q)
{
    orientation = q;
}

float Galaxy::getRadius() const
{
    return radius;
}

void Galaxy::setRadius(float r)
{
    radius = r;
}


Galaxy::GalaxyType Galaxy::getType() const
{
    return type;
}

void Galaxy::setType(Galaxy::GalaxyType _type)
{
    type = _type;

    if (!formsInitialized)
        InitializeForms();
    switch (type)
    {
    case Sa:
    case Sb:
    case Sc:
    case SBa:
    case SBb:
    case SBc:
        form = spiralForm;
        break;
    case Irr:
        form = irregularForm;
        break;
    default:
        form = ellipticalForm;
        break;
    }
}


GalacticForm* Galaxy::getForm() const
{
    return form;
}


void InitializeForms()
{
    int galaxySize = 5000;
    int i;

    // Initialize the spiral form--this is a big hack
    spiralForm = new GalacticForm();
    spiralForm->reserve(galaxySize);
    for (i = 0; i < galaxySize; i++)
    {
        float r = Mathf::frand();
        float theta = Mathf::sfrand() * PI;

        if (r > 0.2f)
        {
            theta = (Mathf::sfrand() + Mathf::sfrand() + Mathf::sfrand()) * (float) PI / 2.0f / 3.0f;
            if (Mathf::sfrand() < 0)
                theta += (float) PI;
        }
        theta += (float) log(r + 1) * 3 * PI;
        float x = r * (float) cos(theta);
        float z = r * (float) sin(theta);
        float y = Mathf::sfrand() * 0.1f * 1.0f / (1 + 2 * r);
        spiralForm->insert(spiralForm->end(), Point3f(x, y, z));
    }

    irregularForm = new GalacticForm();
    irregularForm->reserve(galaxySize);
    i = 0;
    while (i < galaxySize)
    {
        Point3f p(Mathf::sfrand(), Mathf::sfrand(), Mathf::sfrand());
        float r = p.distanceTo(Point3f(0, 0, 0));
        if (r < 1)
        {
            float prob = (1 - r) * (fractalsum(Point3f(p.x + 5, p.y + 5, p.z + 5), 8) + 1) * 0.5f;
            if (Mathf::frand() < prob)
            {
                irregularForm->insert(irregularForm->end(), p);
                i++;
            }
        }
    }

    formsInitialized = true;
}
