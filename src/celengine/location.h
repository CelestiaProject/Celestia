// location.h
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_LOCATION_H_
#define _CELENGINE_LOCATION_H_

#include <string>
#include <celutil/basictypes.h>
#include <celutil/color.h>
#include <celmath/vecmath.h>


class Body;

class Location
{
 public:
    Location();
    virtual ~Location();

    std::string getName(bool i18n = false) const;
    void setName(const std::string&);

    Vec3f getPosition() const;
    void setPosition(const Vec3f&);

    float getSize() const;
    void setSize(float);

    float getImportance() const;
    void setImportance(float);

    std::string getInfoURL() const;
    void setInfoURL(const std::string&);

    bool isLabelColorOverridden() const { return overrideLabelColor; }
    void setLabelColorOverridden(bool override) { overrideLabelColor = override; }

    Color getLabelColor() const { return labelColor; }
    void setLabelColor(Color color) { labelColor = color; }

    void setParentBody(Body*);
    Body* getParentBody() const;

    Point3d getPlanetocentricPosition(double) const;
    Point3d getHeliocentricPosition(double) const;

    typedef uint64_t FeatureType;
    static const FeatureType City           = 0x0000000000000001ull;
    static const FeatureType Observatory    = 0x0000000000000002ull;
    static const FeatureType LandingSite    = 0x0000000000000004ull;
    static const FeatureType Crater         = 0x0000000000000008ull;
    static const FeatureType Vallis         = 0x0000000000000010ull;
    static const FeatureType Mons           = 0x0000000000000020ull;
    static const FeatureType Planum         = 0x0000000000000040ull;
    static const FeatureType Chasma         = 0x0000000000000080ull;
    static const FeatureType Patera         = 0x0000000000000100ull;
    static const FeatureType Mare           = 0x0000000000000200ull;
    static const FeatureType Rupes          = 0x0000000000000400ull;
    static const FeatureType Tessera        = 0x0000000000000800ull;
    static const FeatureType Regio          = 0x0000000000001000ull;
    static const FeatureType Chaos          = 0x0000000000002000ull;
    static const FeatureType Terra          = 0x0000000000004000ull;
    static const FeatureType Astrum         = 0x0000000000008000ull;
    static const FeatureType Corona         = 0x0000000000010000ull;
    static const FeatureType Dorsum         = 0x0000000000020000ull;
    static const FeatureType Fossa          = 0x0000000000040000ull;
    static const FeatureType Catena         = 0x0000000000080000ull;
    static const FeatureType Mensa          = 0x0000000000100000ull;
    static const FeatureType Rima           = 0x0000000000200000ull;
    static const FeatureType Undae          = 0x0000000000400000ull;
    static const FeatureType Tholus         = 0x0000000000800000ull; // Small domical mountain or hill
    static const FeatureType Reticulum      = 0x0000000001000000ull;
    static const FeatureType Planitia       = 0x0000000002000000ull;
    static const FeatureType Linea          = 0x0000000004000000ull;
    static const FeatureType Fluctus        = 0x0000000008000000ull;
    static const FeatureType Farrum         = 0x0000000010000000ull;
    static const FeatureType EruptiveCenter = 0x0000000020000000ull; // Active volcanic centers on Io
    static const FeatureType Insula         = 0x0000000040000000ull; // Islands
    static const FeatureType Albedo         = 0x0000000080000000ull;
    static const FeatureType Arcus          = 0x0000000100000000ull;
    static const FeatureType Cavus          = 0x0000000200000000ull;
    static const FeatureType Colles         = 0x0000000400000000ull;
    static const FeatureType Facula         = 0x0000000800000000ull;
    static const FeatureType Flexus         = 0x0000001000000000ull;
    static const FeatureType Flumen         = 0x0000002000000000ull;
    static const FeatureType Fretum         = 0x0000004000000000ull;
    static const FeatureType Labes          = 0x0000008000000000ull;
    static const FeatureType Labyrinthus    = 0x0000010000000000ull;
    static const FeatureType Lacuna         = 0x0000020000000000ull;
    static const FeatureType Lacus          = 0x0000040000000000ull;
    static const FeatureType Large          = 0x0000080000000000ull;
    static const FeatureType Lenticula      = 0x0000100000000000ull;
    static const FeatureType Lingula        = 0x0000200000000000ull;
    static const FeatureType Macula         = 0x0000400000000000ull;
    static const FeatureType Oceanus        = 0x0000800000000000ull;
    static const FeatureType Palus          = 0x0001000000000000ull;
    static const FeatureType Plume          = 0x0002000000000000ull;
    static const FeatureType Promontorium   = 0x0004000000000000ull;
    static const FeatureType Satellite      = 0x0008000000000000ull;
    static const FeatureType Scopulus       = 0x0010000000000000ull;
    static const FeatureType Serpens        = 0x0020000000000000ull;
    static const FeatureType Sinus          = 0x0040000000000000ull;
    static const FeatureType Sulcus         = 0x0080000000000000ull;
    static const FeatureType Vastitas       = 0x0100000000000000ull;
    static const FeatureType Virga          = 0x0200000000000000ull;
    static const FeatureType Saxum          = 0x0400000000000000ull;
    static const FeatureType Other          = 0x8000000000000000ull;

    FeatureType getFeatureType() const;
    void setFeatureType(FeatureType);
    static FeatureType parseFeatureType(const std::string&);

 private:
    Body* parent;
    std::string name;
    std::string i18nName;
    Vec3f position;
    float size;
    float importance;
    FeatureType featureType;
    bool overrideLabelColor;
    Color labelColor;
    std::string* infoURL;
};

#endif // _CELENGINE_LOCATION_H_
