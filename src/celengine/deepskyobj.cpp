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
#include "opencluster.h"
#include <celutil/util.h>
#include <celutil/debug.h>

using namespace std;


const float DSO_DEFAULT_ABS_MAGNITUDE = -1000.0f;


DeepSkyObject::DeepSkyObject() :
    catalogNumber(InvalidCatalogNumber),
    position(0, 0, 0),
    orientation(1),
    radius(1),
    absMag(DSO_DEFAULT_ABS_MAGNITUDE),
    infoURL(NULL)
{
}

DeepSkyObject::~DeepSkyObject()
{
}

void DeepSkyObject::setCatalogNumber(uint32 n)
{
    catalogNumber = n;
}

Point3d DeepSkyObject::getPosition() const
{
    return position;
}

void DeepSkyObject::setPosition(const Point3d& p)
{
    position = p;
}

Quatf DeepSkyObject::getOrientation() const
{
    return orientation;
}

void DeepSkyObject::setOrientation(const Quatf& q)
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

float DeepSkyObject::getAbsoluteMagnitude() const
{
    return absMag;
}

void DeepSkyObject::setAbsoluteMagnitude(float _absMag)
{
    absMag = _absMag;
}

size_t DeepSkyObject::getDescription(char* buf, size_t bufLength) const
{
    if (bufLength > 0)
        buf[0] = '\0';
    return 0;
}

string DeepSkyObject::getInfoURL() const
{
    if (infoURL == NULL)
        return "";
    else
        return *infoURL;
}

void DeepSkyObject::setInfoURL(const string& s)
{
    if (infoURL == NULL)
        infoURL = new string(s);
    else
        *infoURL = s;
}

bool DeepSkyObject::load(AssociativeArray* params, const string& resPath)
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
    
    double radius = 1.0;
    params->getNumber("Radius", radius);
    setRadius((float) radius);

    double absMag = 0.0;
    if (params->getNumber("AbsMag", absMag))
        setAbsoluteMagnitude((float) absMag);

    string infoURL;
    if (params->getString("InfoURL", infoURL)) 
    {
        if (infoURL.find(':') == string::npos) 
        {
            // Relative URL, the base directory is the current one,
            // not the main installation directory
            if (resPath[1] == ':')
                // Absolute Windows path, file:/// is required
                infoURL = "file:///" + resPath + "/" + infoURL;
            else
                infoURL = resPath + "/" + infoURL;
        }
        setInfoURL(infoURL);
    }
    return true;
}
