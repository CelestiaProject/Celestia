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
#include <celengine/astroobj.h>
#include <celutil/color.h>
#include <Eigen/Core>

class Selection;
class Body;

class Location : public AstroObject
{
public:
    virtual Selection toSelection();

    Eigen::Vector3f getPosition() const;
    void setPosition(const Eigen::Vector3f&);

    float getSize() const;
    void setSize(float);

    float getImportance() const;
    void setImportance(float);

    const std::string& getInfoURL() const;
    void setInfoURL(const std::string&);

    bool isLabelColorOverridden() const { return overrideLabelColor; }
    void setLabelColorOverridden(bool override) { overrideLabelColor = override; }

    Color getLabelColor() const { return labelColor; }
    void setLabelColor(Color color) { labelColor = color; }

    void setParentBody(Body*);
    Body* getParentBody() const;

    Eigen::Vector3d getPlanetocentricPosition(double) const;
    Eigen::Vector3d getHeliocentricPosition(double) const;

    enum FeatureType : uint64_t
    {
        // Custom locations, part I
        City           = 0x0000000000000001,
        Observatory    = 0x0000000000000002,
        LandingSite    = 0x0000000000000004,
        // Standard locations
        Crater         = 0x0000000000000008,
        Vallis         = 0x0000000000000010,
        Mons           = 0x0000000000000020,
        Planum         = 0x0000000000000040,
        Chasma         = 0x0000000000000080,
        Patera         = 0x0000000000000100,
        Mare           = 0x0000000000000200,
        Rupes          = 0x0000000000000400,
        Tessera        = 0x0000000000000800,
        Regio          = 0x0000000000001000,
        Chaos          = 0x0000000000002000,
        Terra          = 0x0000000000004000,
        Astrum         = 0x0000000000008000,
        Corona         = 0x0000000000010000,
        Dorsum         = 0x0000000000020000,
        Fossa          = 0x0000000000040000,
        Catena         = 0x0000000000080000,
        Mensa          = 0x0000000000100000,
        Rima           = 0x0000000000200000,
        Undae          = 0x0000000000400000,
        Tholus         = 0x0000000000800000, // Small domical mountain or hill
        Reticulum      = 0x0000000001000000,
        Planitia       = 0x0000000002000000,
        Linea          = 0x0000000004000000,
        Fluctus        = 0x0000000008000000,
        Farrum         = 0x0000000010000000,
        EruptiveCenter = 0x0000000020000000, // Active volcanic centers on Io
        Insula         = 0x0000000040000000, // Islands
        Albedo         = 0x0000000080000000,
        Arcus          = 0x0000000100000000,
        Cavus          = 0x0000000200000000,
        Colles         = 0x0000000400000000,
        Facula         = 0x0000000800000000,
        Flexus         = 0x0000001000000000,
        Flumen         = 0x0000002000000000,
        Fretum         = 0x0000004000000000,
        Labes          = 0x0000008000000000,
        Labyrinthus    = 0x0000010000000000,
        Lacuna         = 0x0000020000000000,
        Lacus          = 0x0000040000000000,
        LargeRinged    = 0x0000080000000000,
        Lenticula      = 0x0000100000000000,
        Lingula        = 0x0000200000000000,
        Macula         = 0x0000400000000000,
        Oceanus        = 0x0000800000000000,
        Palus          = 0x0001000000000000,
        Plume          = 0x0002000000000000,
        Promontorium   = 0x0004000000000000,
        Satellite      = 0x0008000000000000,
        Scopulus       = 0x0010000000000000,
        Serpens        = 0x0020000000000000,
        Sinus          = 0x0040000000000000,
        Sulcus         = 0x0080000000000000,
        Vastitas       = 0x0100000000000000,
        Virga          = 0x0200000000000000,
        Saxum          = 0x0400000000000000,
        // Custom locations, part II
        Capital        = 0x0800000000000000,
        Cosmodrome     = 0x1000000000000000,
        Ring           = 0x2000000000000000,
        Historical     = 0x4000000000000000,
        Other          = 0x8000000000000000,
    };

    static FeatureType parseFeatureType(const std::string&);

    FeatureType getFeatureType() const;
    void setFeatureType(FeatureType);

 private:
    Body* parent{ nullptr };
    Eigen::Vector3f position{ Eigen::Vector3f::Zero() };
    float size{ 0.0f };
    float importance{ -1.0f };
    FeatureType featureType{ Other };
    bool overrideLabelColor{ false };
    Color labelColor{ 1.0f, 1.0f, 1.0f };
    std::string infoURL;
};

#endif // _CELENGINE_LOCATION_H_
