// deepskyobj.cpp
//
// Copyright (C) 2003-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cmath>

#include <celmath/intersect.h>
#include <celmath/sphere.h>
#include "deepskyobj.h"

Eigen::Vector3d DeepSkyObject::getPosition() const
{
    return position;
}

void DeepSkyObject::setPosition(const Eigen::Vector3d& p)
{
    position = p;
}

Eigen::Quaternionf DeepSkyObject::getOrientation() const
{
    return orientation;
}

void DeepSkyObject::setOrientation(const Eigen::Quaternionf& q)
{
    orientation = q;
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

std::string DeepSkyObject::getDescription() const
{
    return "";
}

const std::string& DeepSkyObject::getInfoURL() const
{
    return infoURL;
}

void DeepSkyObject::setInfoURL(const std::string& s)
{
    infoURL = s;
}


bool DeepSkyObject::pick(const Eigen::ParametrizedLine<double, 3>& ray,
                         double& distanceToPicker,
                         double& cosAngleToBoundCenter) const
{
    if (isVisible())
        return celmath::testIntersection(ray,
                                         celmath::Sphered(position, static_cast<double>(radius)),
                                         distanceToPicker,
                                         cosAngleToBoundCenter);
    else
        return false;
}


void DeepSkyObject::hsv2rgb( float *r, float *g, float *b, float h, float s, float v )
{
    // r,g,b values are from 0 to 1
    // h = [0,360], s = [0,1], v = [0,1]

    int i;
    float f, p, q, t;

    if( s == 0 )
    {
        // achromatic (grey)
        *r = *g = *b = v;
        return;
    }

    h /= 60;                      // sector 0 to 5
    i = static_cast<int>(std::floor( h ));
    f = h - static_cast<float>(i);            // factorial part of h
    p = v * ( 1 - s );
    q = v * ( 1 - s * f );
    t = v * ( 1 - s * ( 1 - f ) );

    switch( i )
    {
    case 0:
        *r = v;
        *g = t;
        *b = p;
        break;
    case 1:
        *r = q;
        *g = v;
        *b = p;
        break;
    case 2:
        *r = p;
        *g = v;
        *b = t;
        break;
    case 3:
        *r = p;
        *g = q;
        *b = v;
        break;
    case 4:
        *r = t;
        *g = p;
        *b = v;
        break;
    default:
        *r = v;
        *g = p;
        *b = q;
        break;
    }
}

bool DeepSkyObject::load(AssociativeArray* params, const fs::path& resPath)
{
    // Get position
    Eigen::Vector3d position(Eigen::Vector3d::Zero());
    if (params->getVector("Position", position))
    {
        setPosition(position);
    }
    else
    {
        double distance = 1.0;
        double RA = 0.0;
        double dec = 0.0;
        params->getLength("Distance", distance, KM_PER_LY<double>);
        params->getAngle("RA", RA, DEG_PER_HRA);
        params->getAngle("Dec", dec);

        Eigen::Vector3d p = astro::equatorialToCelestialCart(RA, dec, distance);
        setPosition(p);
    }

    // Get orientation
    Eigen::Vector3d axis(Eigen::Vector3d::UnitX());
    double angle = 0.0;
    params->getVector("Axis", axis);
    params->getAngle("Angle", angle);

    setOrientation(Eigen::Quaternionf(Eigen::AngleAxisf(static_cast<float>(celmath::degToRad(angle)),
                                                        axis.cast<float>().normalized())));

    double radius = 1.0;
    params->getLength("Radius", radius, KM_PER_LY<double>);

    setRadius(static_cast<float>(radius));

    double absMag = 0.0;
    if (params->getNumber("AbsMag", absMag))
        setAbsoluteMagnitude(static_cast<float>(absMag));

    std::string infoURL; // FIXME: infourl class
    if (params->getString("InfoURL", infoURL))
    {
        if (infoURL.find(':') == std::string::npos)
        {
            // Relative URL, the base directory is the current one,
            // not the main installation directory
            if (resPath.c_str()[1] == ':')
                // Absolute Windows path, file:/// is required
                infoURL = "file:///" + resPath.string() + "/" + infoURL;
            else if (!resPath.empty())
                infoURL = resPath.string() + "/" + infoURL;
        }
        setInfoURL(infoURL);
    }

    bool visible = true;
    if (params->getBoolean("Visible", visible))
    {
        setVisible(visible);
    }

    bool clickable = true;
    if (params->getBoolean("Clickable", clickable))
    {
        setClickable(clickable);
    }

    return true;
}

Selection DeepSkyObject::toSelection()
{
//    std::cout << "DeepSkyObject::toSelection()\n";
    return Selection(this);
}
