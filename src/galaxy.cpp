// galaxy.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "galaxy.h"

using namespace std;


Galaxy::Galaxy() :
    name(""), position(0, 0, 0), orientation(1), radius(1)
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
