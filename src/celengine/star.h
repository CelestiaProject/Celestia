// star.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _STAR_H_
#define _STAR_H_

#include <celutil/basictypes.h>
#include <celmath/vecmath.h>
#include <celengine/celestia.h>
#include <celengine/stellarclass.h>


class Star
{
public:
    inline Star();

    inline uint32 getCatalogNumber() const;
    inline StellarClass getStellarClass() const;
    inline Point3f getPosition() const;
    inline float getAbsoluteMagnitude() const;
    float getApparentMagnitude(float) const;
    float getLuminosity() const;
    float getRadius() const;
    float getTemperature() const;
    float getRotationPeriod() const;
    float getBolometricMagnitude() const;

    void setCatalogNumber(uint32);
    void setPosition(float, float, float);
    void setPosition(Point3f);
    void setStellarClass(StellarClass);
    void setAbsoluteMagnitude(float);
    void setLuminosity(float);

    enum {
        InvalidCatalogNumber = 0xffffffff
    };

private:
    uint32 catalogNumber;
    void* extendedInfo;
    Point3f position;
    float absMag;
    StellarClass stellarClass;
};


Star::Star() :
    catalogNumber(InvalidCatalogNumber),
    extendedInfo(NULL),
    position(0, 0, 0),
    absMag(4.83f),
    stellarClass()
{
}

uint32 Star::getCatalogNumber() const
{
    return catalogNumber;
}

float Star::getAbsoluteMagnitude() const
{
    return absMag;
}

StellarClass Star::getStellarClass() const
{
    return stellarClass;
}

Point3f Star::getPosition() const
{
    return position;
}

#endif // _STAR_H_
