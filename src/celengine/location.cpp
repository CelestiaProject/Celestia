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


static map<string, Location::FeatureType> FeatureNameToFlag;
static bool featureTableInitialized = false;

struct FeatureNameEntry
{
    const char* name;
    Location::FeatureType flag;
};

FeatureNameEntry FeatureNames[] =
{
    { "AA", Location::Crater },
    { "AS", Location::Astrum },
    { "CA", Location::Catena },
    { "CH", Location::Chaos },
    { "CM", Location::Chasma },
    { "CR", Location::Corona },
    { "DO", Location::Dorsum },
    { "ER", Location::EruptiveCenter },
    { "FL", Location::Fluctus },
    { "FO", Location::Fossa },
    { "FR", Location::Farrum },
    { "IN", Location::Insula },
    { "LF", Location::LandingSite },
    { "LI", Location::Linea },
    { "ME", Location::Mare },
    { "MN", Location::Mensa },
    { "MO", Location::Mons },
    { "PE", Location::Patera },
    { "PL", Location::Planitia },
    { "PM", Location::Planum },
    { "RE", Location::Regio },
    { "RI", Location::Rima },
    { "RT", Location::Reticulum },
    { "RU", Location::Rupes },
    { "TA", Location::Terra },
    { "TE", Location::Tessera },
    { "TH", Location::Tholus },
    { "UN", Location::Undae },
    { "VA", Location::Vallis },
    { "XX", Location::Other },
    { "City", Location::City },
    { "Observatory", Location::Observatory },
    { "Landing Site", Location::LandingSite },
    { "Crater", Location::Crater },
};


Location::~Location()
{
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


Location::FeatureType Location::getFeatureType() const
{
    return featureType;
}


void Location::setFeatureType(Location::FeatureType _featureType)
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


Location::FeatureType Location::parseFeatureType(const string& s)
{
    if (!featureTableInitialized)
        initFeatureTypeTable();

    auto flag = (uint32_t)FeatureNameToFlag[s];
    return flag != 0 ? (Location::FeatureType)flag : Other;
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
    if (parent == nullptr)
        return position.cast<double>();

    Quaterniond q = parent->getEclipticToBodyFixed(t);
    return q.conjugate() * position.cast<double>();
}


Vector3d Location::getHeliocentricPosition(double t) const
{
    if (parent == nullptr)
        return position.cast<double>();

    return parent->getAstrocentricPosition(t) + getPlanetocentricPosition(t);
}
