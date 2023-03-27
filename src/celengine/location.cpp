// location.cpp
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <libintl.h>
#include <map>
#include <celengine/location.h>
#include <celengine/body.h>
#include <celutil/util.h>

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
    { "AL", Location::Albedo },
    { "AR", Location::Arcus },
    { "AS", Location::Astrum },
    { "CA", Location::Catena },
    { "CB", Location::Cavus },
    { "CH", Location::Chaos },
    { "CM", Location::Chasma },
    { "CO", Location::Colles },
    { "CR", Location::Corona },
    { "DO", Location::Dorsum },
    { "ER", Location::EruptiveCenter },
    { "FA", Location::Facula },
    { "FR", Location::Farrum },
    { "FE", Location::Flexus },
    { "FL", Location::Fluctus },
    { "FM", Location::Flumen },
    { "FO", Location::Fossa },
    { "FR", Location::Farrum },
    { "FT", Location::Fretum },
    { "IN", Location::Insula },
    { "LA", Location::Labes },
    { "LB", Location::Labyrinthus },
    { "LU", Location::Lacuna },
    { "LC", Location::Lacus },
    { "LF", Location::LandingSite },
    { "LG", Location::LargeRinged },
    { "LE", Location::Lenticula },
    { "LI", Location::Linea },
    { "LN", Location::Lingula },
    { "MA", Location::Macula },
    { "ME", Location::Mare },
    { "MN", Location::Mensa },
    { "MO", Location::Mons },
    { "OC", Location::Oceanus },
    { "PA", Location::Palus },
    { "PE", Location::Patera },
    { "PL", Location::Planitia },
    { "PM", Location::Planum },
    { "PU", Location::Plume },
    { "PR", Location::Promontorium },
    { "RE", Location::Regio },
    { "RI", Location::Rima },
    { "RT", Location::Reticulum },
    { "RU", Location::Rupes },
    { "SA", Location::Saxum },
    { "SF", Location::Satellite },
    { "SC", Location::Scopulus },
    { "SE", Location::Serpens },
    { "SI", Location::Sinus },
    { "SU", Location::Sulcus },
    { "TA", Location::Terra },
    { "TE", Location::Tessera },
    { "TH", Location::Tholus },
    { "UN", Location::Undae },
    { "VA", Location::Vallis },
    { "VS", Location::Vastitas },
    { "VI", Location::Virga },
    { "XX", Location::Other },
    { "City", Location::City },
    { "Observatory", Location::Observatory },
    { "Landing Site", Location::LandingSite },
    { "Crater", Location::Crater },
    { "Capital", Location::Capital},
    { "Cosmodrome", Location::Cosmodrome},
    { "Ring", Location::Ring},
    { "RG", Location::Ring},
    { "Historical", Location::Historical},
};
 

Location::Location() :
    parent(NULL),
    position(0.0f, 0.0f, 0.0f),
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


Vec3f Location::getPosition() const
{
    return position;
}


void Location::setPosition(const Vec3f& _position)
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

    FeatureType flag = FeatureNameToFlag[s];
    return flag != (FeatureType) 0 ? flag : Other;
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
Point3d Location::getPlanetocentricPosition(double t) const
{
    if (parent == NULL)
        return Point3d(position.x, position.y, position.z);

    Quatd q = parent->getEclipticToBodyFixed(t);
    return Point3d(position.x, position.y, position.z) * q.toMatrix3();
}


Point3d Location::getHeliocentricPosition(double t) const
{
    if (parent == NULL)
        return Point3d(position.x, position.y, position.z);

    return parent->getAstrocentricPosition(t) +
        (getPlanetocentricPosition(t) - Point3d(0.0, 0.0, 0.0));
}
