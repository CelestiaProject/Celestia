// location.cpp
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <map>
#include <celengine/location.h>
#include <celengine/body.h>
#include <celutil/util.h>

using namespace Eigen;
using namespace std;


static map<string, uint32> FeatureNameToFlag;
static bool featureTableInitialized = false;

struct FeatureNameEntry
{
    const char* name;
    uint32 flag;
};

FeatureNameEntry FeatureNames[] =
{
    { "AA", Location::Crater },
    { "VA", Location::Vallis },
    { "MO", Location::Mons },
    { "PM", Location::Planum },
    { "CM", Location::Chasma },
    { "PE", Location::Patera },
    { "ME", Location::Mare },
    { "RU", Location::Rupes },
    { "TE", Location::Tessera },
    { "RE", Location::Regio },
    { "CH", Location::Chaos },
    { "TA", Location::Terra },
    { "AS", Location::Astrum },
    { "CR", Location::Corona },
    { "DO", Location::Dorsum },
    { "FO", Location::Fossa },
    { "CA", Location::Catena },
    { "MN", Location::Mensa },
    { "RI", Location::Rima },
    { "UN", Location::Undae },
    { "TH", Location::Tholus },
    { "RT", Location::Reticulum },
    { "PL", Location::Planitia },
    { "LI", Location::Linea },
    { "FL", Location::Fluctus },
    { "FR", Location::Farrum },
    { "LF", Location::LandingSite },
    { "ER", Location::EruptiveCenter },
    { "IN", Location::Insula },
    { "XX", Location::Other },
    { "City", Location::City },
    { "Observatory", Location::Observatory },
    { "Landing Site", Location::LandingSite },
    { "Crater", Location::Crater },
};
 

Location::Location() :
    parent(NULL),
    position(Vector3f::Zero()),
    size(0.0f),
    importance(-1.0f),
    featureType(Other),
    overrideLabelColor(false),
    labelColor(1.0f, 1.0f, 1.0f),
    infoURL(NULL)
{
}

Location::~Location()
{
    if (infoURL != NULL)
        delete infoURL;
}


string Location::getName(bool i18n) const
{
    if (!i18n || i18nName == "") return name;
    return i18nName;
}


void Location::setName(const string& _name)
{
    name = _name;
    i18nName = _(_name.c_str()); 
    if (name == i18nName) i18nName = "";
}


Vector3f Location::getPosition() const
{
    return position;
}


void Location::setPosition(const Vector3f& _position)
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


void Location::setInfoURL(const string&)
{
}


uint32 Location::getFeatureType() const
{
    return featureType;
}


void Location::setFeatureType(uint32 _featureType)
{
    featureType = _featureType;
}


static void initFeatureTypeTable()
{
    featureTableInitialized = true;

    for (int i = 0; i < (int)(sizeof(FeatureNames) / sizeof(FeatureNames[0])); i++)
    {
        FeatureNameToFlag[string(FeatureNames[i].name)] = FeatureNames[i].flag;
    }
}


uint32 Location::parseFeatureType(const string& s)
{
    if (!featureTableInitialized)
        initFeatureTypeTable();

    int flag = FeatureNameToFlag[s];
    return flag != 0 ? flag : (uint32) Other;
}


Body* Location::getParentBody() const
{
    return parent;
}


void Location::setParentBody(Body* _parent)
{
    parent = _parent;
}


/*! Get the position of the location relative to its body in 
 *  the J2000 ecliptic coordinate system.
 */
Vector3d Location::getPlanetocentricPosition(double t) const
{
    if (parent == NULL)
    {
        return position.cast<double>();
    }
    else
    {
        Quaterniond q = parent->getEclipticToBodyFixed(t);
        return q.conjugate() * position.cast<double>();
    }
}


Vector3d Location::getHeliocentricPosition(double t) const
{
    if (parent == NULL)
    {
        return position.cast<double>();
    }
    else
    {
        return parent->getAstrocentricPosition(t) + getPlanetocentricPosition(t);
    }

}
