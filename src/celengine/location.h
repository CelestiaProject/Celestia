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
#include <celutil/color.h>
#include <Eigen/Core>

class Body;

class Location
{
 public:
    Location() = default;
    virtual ~Location();

    std::string getName(bool i18n = false) const;
    void setName(const std::string&);

    Eigen::Vector3f getPosition() const;
    void setPosition(const Eigen::Vector3f&);

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

    Eigen::Vector3d getPlanetocentricPosition(double) const;
    Eigen::Vector3d getHeliocentricPosition(double) const;

    enum FeatureType
    {
        City           = 0x00000001,
        Observatory    = 0x00000002,
        LandingSite    = 0x00000004,
        Crater         = 0x00000008,
        Vallis         = 0x00000010,
        Mons           = 0x00000020,
        Planum         = 0x00000040,
        Chasma         = 0x00000080,
        Patera         = 0x00000100,
        Mare           = 0x00000200,
        Rupes          = 0x00000400,
        Tessera        = 0x00000800,
        Regio          = 0x00001000,
        Chaos          = 0x00002000,
        Terra          = 0x00004000,
        Astrum         = 0x00008000,
        Corona         = 0x00010000,
        Dorsum         = 0x00020000,
        Fossa          = 0x00040000,
        Catena         = 0x00080000,
        Mensa          = 0x00100000,
        Rima           = 0x00200000,
        Undae          = 0x00400000,
        Tholus         = 0x00800000, // Small domical mountain or hill
        Reticulum      = 0x01000000,
        Planitia       = 0x02000000,
        Linea          = 0x04000000,
        Fluctus        = 0x08000000,
        Farrum         = 0x10000000,
        EruptiveCenter = 0x20000000, // Active volcanic centers on Io
        Insula         = 0x40000000, // Islands
        Other          = 0x80000000,
    };

    static FeatureType parseFeatureType(const std::string&);

    FeatureType getFeatureType() const;
    void setFeatureType(FeatureType);

 private:
    Body* parent{ nullptr };
    std::string name;
    std::string i18nName;
    Eigen::Vector3f position{ Eigen::Vector3f::Zero() };
    float size{ 0.0f };
    float importance{ -1.0f };
    FeatureType featureType{ Other };
    bool overrideLabelColor{ false };
    Color labelColor{ 1.0f, 1.0f, 1.0f };
    std::string* infoURL{ nullptr };
};

#endif // _CELENGINE_LOCATION_H_
