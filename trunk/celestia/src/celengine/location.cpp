// location.cpp
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celengine/location.h>
#include <celutil/util.h>

using namespace std;


Location::Location() :
    position(0.0f, 0.0f, 0.0f),
    size(0.0f),
    importance(-1.0f),
    infoURL(NULL)
{
}

Location::~Location()
{
    if (infoURL != NULL)
        delete infoURL;
}


string Location::getName() const
{
    return name;
}


void Location::setName(const string& _name)
{
    name = _name;
}


Point3f Location::getPosition() const
{
    return position;
}


void Location::setPosition(const Point3f& _position)
{
    position = _position;
}


float Location::getSize() const
{
    return size;
}


void Location::setSize(float _size)
{
    size = _size;
}


float Location::getImportance() const
{
    return importance;
}


void Location::setImportance(float _importance)
{
    importance = _importance;
}


string Location::getInfoURL() const
{
    return "";
}


void Location::setInfoURL(const string& url)
{
}


static uint32 parseFeatureType(const string& s)
{
    if (compareIgnoringCase(s, "city"))
        return 1;
    else
        return 0;
}
