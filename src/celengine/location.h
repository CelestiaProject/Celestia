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
#include <celmath/vecmath.h>


class Location
{
 public:
    Location();
    virtual ~Location();

    std::string getName() const;
    void setName(const std::string&);

    Point3f getPosition() const;
    void setPosition(const Point3f&);

    float getSize() const;
    void setSize(float);

    float getImportance() const;
    void setImportance(float);

    std::string getInfoURL() const;
    void setInfoURL(const std::string&);

    static uint32 parseFeatureType(const std::string&);

    enum FeatureType
    {
        City           = 0x00000001,
        Observatory    = 0x00000002,
        LandingSite    = 0x00000004,
        Crater         = 0x00000008,
        Vallis         = 0x00000010,
        Mons           = 0x00000020,
        Planitia       = 0x00000040,
        Chasma         = 0x00000080,
        Patera         = 0x00000100,
        Mare           = 0x00000200,
        Rupes          = 0x00000400,
        Tessera        = 0x00000800,
        Regio          = 0x00001000,
        Chaos          = 0x00002000,
        Terra          = 0x00004000,
        Other          = 0x80000000,
    };

 private:
    std::string name;
    Point3f position;
    float size;
    float importance;
    std::string* infoURL;
};

#endif // _CELENGINE_LOCATION_H_
