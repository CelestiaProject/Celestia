// deepskyobj.cpp
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <stdio.h>
#include "celestia.h"
#include <cassert>
#include "astro.h"
#include "deepskyobj.h"
#include "galaxy.h"
#include "nebula.h"
#include <celutil/util.h>
#include <celutil/debug.h>

using namespace std;


DeepSkyObject::DeepSkyObject() :
    name(""),
    position(0, 0, 0),
    orientation(1),
    radius(1)
{
}

DeepSkyObject::~DeepSkyObject()
{
}


string DeepSkyObject::getName() const
{
    return name;
}

void DeepSkyObject::setName(const string& s)
{
    name = s;
}

Point3d DeepSkyObject::getPosition() const
{
    return position;
}

void DeepSkyObject::setPosition(Point3d p)
{
    position = p;
}

Quatf DeepSkyObject::getOrientation() const
{
    return orientation;
}

void DeepSkyObject::setOrientation(Quatf q)
{
    orientation = q;
}

float DeepSkyObject::getRadius() const
{
    return radius;
}

void DeepSkyObject::setRadius(float r)
{
    radius = r;
}

bool DeepSkyObject::load(AssociativeArray* params)
{
    // Get position
    Vec3d position(0.0, 0.0, 0.0);
    if (params->getVector("Position", position))
    {
        setPosition(Point3d(position.x, position.y, position.z));
    }
    else
    {
        double distance = 1.0;
        double RA = 0.0;
        double dec = 0.0;
        params->getNumber("Distance", distance);
        params->getNumber("RA", RA);
        params->getNumber("Dec", dec);
        Point3d p = astro::equatorialToCelestialCart(RA, dec, distance);
        setPosition(p);
    }

    // Get orientation
    Vec3d axis(1.0, 0.0, 0.0);
    double angle = 0.0;
    params->getVector("Axis", axis);
    params->getNumber("Angle", angle);
    Quatf q(1);
    q.setAxisAngle(Vec3f((float) axis.x, (float) axis.y, (float) axis.z),
                   (float) degToRad(angle));
    setOrientation(q);
    
    double radius = 0.0;
    params->getNumber("Radius", radius);
    setRadius((float) radius);

    return true;
}


DeepSkyCatalog* ReadDeepSkyCatalog(istream& in)
{
    DeepSkyCatalog* catalog = new DeepSkyCatalog();

    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        string objType;
        string objName;
        
        if (tokenizer.getTokenType() != Tokenizer::TokenName)
        {
            DPRINTF(0, "Error parsing deep sky catalog file.\n");
            for_each(catalog->begin(), catalog->end(),
                     deleteFunc<DeepSkyObject*>());
            delete catalog;
            return NULL;
        }
        objType = tokenizer.getNameValue();

        if (tokenizer.nextToken() != Tokenizer::TokenString)
        {
            DPRINTF(0, "Error parsing deep sky catalog file: bad name.\n");
            for_each(catalog->begin(), catalog->end(),
                     deleteFunc<DeepSkyObject*>());
            delete catalog;
            return NULL;
        }
        objName = tokenizer.getStringValue();

        Value* objParamsValue = parser.readValue();
        if (objParamsValue == NULL ||
            objParamsValue->getType() != Value::HashType)
        {
            DPRINTF(0, "Error parsing deep sky catalog entry %s\n",
                    objName.c_str());
            for_each(catalog->begin(), catalog->end(),
                     deleteFunc<DeepSkyObject*>());
            delete catalog;
            return NULL;
        }

        Hash* objParams = objParamsValue->getHash();
        assert(objParams != NULL);

        DeepSkyObject* obj = NULL;
        if (compareIgnoringCase(objType, "Galaxy") == 0)
            obj = new Galaxy();
        else if (compareIgnoringCase(objType, "Nebula") == 0)
            obj = new Nebula();

        if (obj != NULL)
        {
            obj->setName(objName);
            if (obj->load(objParams))
                catalog->insert(catalog->end(), obj);
            else
                delete obj;
            delete objParamsValue;
        }
    }

    return catalog;
}

