// marker.h
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "marker.h"


using namespace std;


Marker::Marker(const Selection& s) :
    obj(s),
    size(10.0f),
    color(Color::White)
{
}

Marker::~Marker()
{
}


UniversalCoord Marker::getPosition(double jd) const
{
    return obj.getPosition(jd);
}


Selection Marker::getObject() const
{
    return obj;
}


Color Marker::getColor() const
{
    return color;
}


void Marker::setColor(Color _color)
{
    color = _color;
}


float Marker::getSize() const
{
    return size;
}


void Marker::setSize(float _size)
{
    size = _size;
}
