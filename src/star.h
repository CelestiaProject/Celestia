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
    inline StellarClass getStellarClass() const;
    inline Point3f getPosition() const;
    inline float getLuminosity() const;
    float getRadius() const;
    float getTemperature() const;

    void setCatalogNumber(uint32);
    void setPosition(float, float, float);
    void setPosition(Point3f);
    void setStellarClass(StellarClass);
    void setLuminosity(float);

    float getAbsoluteMagnitude() const;
    void setAbsoluteMagnitude(float);

    enum {
        InvalidStar = 0xffffffff
    };

private:
    uint32 catalogNumber;
    Point3f position;
    float luminosity;           // in units of solar luminosity
    StellarClass stellarClass;
};


Star::Star() : catalogNumber(0),
               position(0, 0, 0),
               luminosity(1),
               stellarClass()
{
}

uint32 Star::getCatalogNumber() const
{
    return catalogNumber;
}

float Star::getLuminosity() const
{
    return luminosity;
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
