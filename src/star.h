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

#include "celestia.h"
#include "basictypes.h"
#include "vecmath.h"
#include "stellarclass.h"


class Star
{
public:
    inline Star();

    inline uint32 getCatalogNumber() const;
    inline uint32 getCatalogNumber(unsigned int) const;
    inline StellarClass getStellarClass() const;
    inline Point3f getPosition() const;
    inline float getAbsoluteMagnitude() const;
    float getApparentMagnitude(float) const;
    float getLuminosity() const;
    float getRadius() const;
    float getTemperature() const;
    float getRotationPeriod() const;

    void setCatalogNumber(uint32);
    void setCatalogNumber(unsigned int, uint32);
    void setPosition(float, float, float);
    void setPosition(Point3f);
    void setStellarClass(StellarClass);
    void setAbsoluteMagnitude(float);
    void setLuminosity(float);

    enum {
        InvalidCatalogNumber = 0xffffffff
    };

    enum {
        HIPCatalog           = 0,
        HDCatalog            = 1,
        CatalogCount         = 2,
    };

private:
    uint32 catalogNumbers[CatalogCount];
    Point3f position;
    float absMag;
    StellarClass stellarClass;
};


Star::Star() : position(0, 0, 0),
               absMag(4.83f),
               stellarClass()
{
    for (int i = 0; i < CatalogCount; i++)
        catalogNumbers[i] = InvalidCatalogNumber;
}

uint32 Star::getCatalogNumber() const
{
    return catalogNumbers[0];
}

uint32 Star::getCatalogNumber(unsigned int which) const
{
    return catalogNumbers[which];
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
