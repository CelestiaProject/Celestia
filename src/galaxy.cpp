// galaxy.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include "celestia.h"
#include "mathlib.h"
#include "util.h"
#include "astro.h"
#include "galaxy.h"
#include "perlin.h"
#include "parser.h"

using namespace std;

static bool formsInitialized = false;
static vector<Point3f>* spiralPoints = NULL;
static vector<Point3f>* ellipticalPoints = NULL;
static vector<Point3f>* irregularPoints = NULL;
static GalacticForm* spiralForm = NULL;
static GalacticForm* irregularForm = NULL;
static GalacticForm** ellipticalForms = NULL;
static void InitializeForms();

struct GalaxyTypeName 
{
    char* name;
    Galaxy::GalaxyType type;
};

static GalaxyTypeName GalaxyTypeNames[] = 
{
    { "S0", Galaxy::S0 },
    { "Sa", Galaxy::Sa },
    { "Sb", Galaxy::Sb },
    { "Sc", Galaxy::Sc },
    { "SBa", Galaxy::SBa },
    { "SBb", Galaxy::SBb },
    { "SBc", Galaxy::SBc },
    { "E0", Galaxy::E0 },
    { "E1", Galaxy::E1 },
    { "E2", Galaxy::E2 },
    { "E3", Galaxy::E3 },
    { "E4", Galaxy::E4 },
    { "E5", Galaxy::E5 },
    { "E6", Galaxy::E6 },
    { "E7", Galaxy::E7 },
    { "Irr", Galaxy::Irr },
};


Galaxy::Galaxy() :
    name(""),
    position(0, 0, 0), orientation(1), radius(1),
    detail(1.0f),
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

float Galaxy::getDetail() const
{
    return detail;
}

void Galaxy::setDetail(float d)
{
    detail = d;
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
    case E0:
    case E1:
    case E2:
    case E3:
    case E4:
    case E5:
    case E6:
    case E7:
        form = ellipticalForms[type - E0];
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
    spiralPoints = new vector<Point3f>();
    spiralPoints->reserve(galaxySize);
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
        spiralPoints->insert(spiralPoints->end(), Point3f(x, y, z));
    }
    spiralForm = new GalacticForm();
    spiralForm->points = spiralPoints;
    spiralForm->scale = Vec3f(1, 1, 1);

    irregularPoints = new vector<Point3f>;
    irregularPoints->reserve(galaxySize);
    i = 0;
    while (i < galaxySize)
    {
        Point3f p(Mathf::sfrand(), Mathf::sfrand(), Mathf::sfrand());
        float r = p.distanceFromOrigin();
        if (r < 1)
        {
            float prob = (1 - r) * (fractalsum(Point3f(p.x + 5, p.y + 5, p.z + 5), 8) + 1) * 0.5f;
            if (Mathf::frand() < prob)
            {
                irregularPoints->insert(irregularPoints->end(), p);
                i++;
            }
        }
    }
    irregularForm = new GalacticForm();
    irregularForm->points = irregularPoints;
    irregularForm->scale = Vec3f(1, 1, 1);

    ellipticalPoints = new vector<Point3f>();
    ellipticalPoints->reserve(galaxySize);
    i = 0;
    while (i < galaxySize)
    {
        Point3f p(Mathf::sfrand(), Mathf::sfrand(), Mathf::sfrand());
        float r = p.distanceFromOrigin();
        if (r < 1)
        {
            if (Mathf::frand() < cube(1 - r))
            {
                ellipticalPoints->insert(ellipticalPoints->end(), p);
                i++;
            }
        }
    }

    ellipticalForms = new GalacticForm*[8];
    for (int eform = 0; eform <= 7; eform++)
    {
        ellipticalForms[eform] = new GalacticForm();
        ellipticalForms[eform]->points = ellipticalPoints;
        ellipticalForms[eform]->scale = Vec3f(1.0f, 1.0f - (float) eform / 8.0f, 1.0f);
    }

    formsInitialized = true;
}


GalaxyList* ReadGalaxyList(istream& in)
{
    GalaxyList* galaxies = new GalaxyList();
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        if (tokenizer.getTokenType() != Tokenizer::TokenString)
        {
            DPRINTF("Error parsing galaxies file.\n");
            for_each(galaxies->begin(), galaxies->end(), deleteFunc<Galaxy*>());
            delete galaxies;
            return NULL;
        }

        Galaxy* galaxy = new Galaxy();
        galaxy->setName(tokenizer.getStringValue());

        Value* galaxyParamsValue = parser.readValue();
        if (galaxyParamsValue == NULL || galaxyParamsValue->getType() != Value::HashType)
        {
            DPRINTF("Error parsing galaxy entry %s\n", galaxy->getName().c_str());
            for_each(galaxies->begin(), galaxies->end(), deleteFunc<Galaxy*>());
            delete galaxy;
            return NULL;
        }

        Hash* galaxyParams = galaxyParamsValue->getHash();

        // Get position
        Vec3d position(0.0, 0.0, 0.0);
        if (galaxyParams->getVector("Position", position))
        {
            galaxy->setPosition(Point3d(position.x, position.y, position.z));
        }
        else
        {
            double distance = 1.0;
            double RA = 0.0;
            double dec = 0.0;
            galaxyParams->getNumber("Distance", distance);
            galaxyParams->getNumber("RA", RA);
            galaxyParams->getNumber("Dec", dec);
            Point3f p = astro::equatorialToCelestialCart((float) RA, (float) dec, (float) distance);
            galaxy->setPosition(Point3d(p.x, p.y, p.z));
            DPRINTF("%s: %f, %f, %f\n", galaxy->getName().c_str(),
                    p.x, p.y, p.z);
        }

        // Get orientation
        Vec3d axis(1.0, 0.0, 0.0);
        double angle = 0.0;
        galaxyParams->getVector("Axis", axis);
        galaxyParams->getNumber("Angle", angle);
        Quatf q(1);
        q.setAxisAngle(Vec3f((float) axis.x, (float) axis.y, (float) axis.z),
                       (float) degToRad(angle));
        galaxy->setOrientation(q);

        double radius = 0.0;
        galaxyParams->getNumber("Radius", radius);
        galaxy->setRadius((float) radius);

        double detail = 1.0;
        galaxyParams->getNumber("Detail", detail);
        galaxy->setDetail((float) detail);

        string typeName;
        galaxyParams->getString("Type", typeName);
        Galaxy::GalaxyType type = Galaxy::Irr;
        for (int i = 0; i < (int) (sizeof(GalaxyTypeNames) / sizeof(GalaxyTypeNames[0])); i++)
        {
            if (GalaxyTypeNames[i].name == typeName)
            {
                type = GalaxyTypeNames[i].type;
                break;
            }
        }
        galaxy->setType(type);

        galaxies->insert(galaxies->end(), galaxy);
    }

    return galaxies;
}


ostream& operator<<(ostream& s, const Galaxy::GalaxyType& sc)
{
    return s << GalaxyTypeNames[static_cast<int>(sc)].name;
}
