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
#include <celutil/reshandle.h>
#include <celutil/color.h>
#include <celmath/vecmath.h>
#include <celengine/celestia.h>
#include <celengine/stellarclass.h>

class Orbit;

class StarDetails
{
 public:
    StarDetails::StarDetails();

    inline float getRadius() const;
    inline float getTemperature() const;
    inline float getRotationPeriod() const;
    inline Vec3f getSemiAxes() const;
    inline ResourceHandle getMesh() const;
    inline ResourceHandle getTexture() const;
    inline const Orbit* getOrbit() const;
    inline uint32 getKnowledge() const;
    inline bool getKnowledge(uint32) const;
    inline const char* getSpectralType() const;
    inline float getBolometricCorrection() const;

    void setRadius(float);
    void setTemperature(float);
    void setRotationPeriod(float);
    void setKnowledge(uint32);
    void addKnowledge(uint32);
    void setSpectralType(const std::string&);
    void setBolometricCorrection(float);
    
    enum
    {
        KnowRadius   = 0x1,
        KnowRotation = 0x2,
    };

 private:
    float radius;
    float temperature;
    float bolometricCorrection;
    float rotationPeriod;

    uint32 knowledge;
    char spectralType[8];

 public:
    static StarDetails* GetStarDetails(const StellarClass&);
    static StarDetails* CreateStandardStarType(const std::string& _specType,
                                               float _temperature,
                                               float _rotationPeriod);

    static StarDetails* GetNormalStarDetails(StellarClass::SpectralClass specClass,
                                             unsigned int subclass,
                                             StellarClass::LuminosityClass lumClass);
    static StarDetails* GetWhiteDwarfDetails(StellarClass::SpectralClass specClass,
                                             unsigned int subclass);
    static StarDetails* GetNeutronStarDetails();
    static StarDetails* GetBlackHoleDetails();
};


float
StarDetails::getRadius() const
{
    return radius;
}

float
StarDetails::getTemperature() const
{
    return temperature;
}

float
StarDetails::getRotationPeriod() const
{
    return rotationPeriod;
}

Vec3f
StarDetails::getSemiAxes() const
{
    return Vec3f(radius, radius, radius);
}

ResourceHandle
StarDetails::getMesh() const
{
    return InvalidResource;
}

ResourceHandle
StarDetails::getTexture() const
{
    return InvalidResource;
}

const Orbit*
StarDetails::getOrbit() const
{
    return NULL;
}

uint32
StarDetails::getKnowledge() const
{
    return knowledge;
}

bool
StarDetails::getKnowledge(uint32 knowledgeFlags) const
{
    return ((knowledge & knowledgeFlags) == knowledgeFlags);
}

const char*
StarDetails::getSpectralType() const
{
    return spectralType;
}

float
StarDetails::getBolometricCorrection() const
{
    return bolometricCorrection;
}


class Star
{
public:
    inline Star();

    // Accessor methods for members of the star class
    inline uint32 getCatalogNumber() const;
    inline Point3f getPosition() const;
    inline float getAbsoluteMagnitude() const;
    float getApparentMagnitude(float) const;
    float getLuminosity() const;

    void setCatalogNumber(uint32);
    void setPosition(float, float, float);
    void setPosition(Point3f);
    void setAbsoluteMagnitude(float);
    void setLuminosity(float);

    void setDetails(StarDetails*);

    // Accessor methods that delegate to StarDetails
    float getRadius() const;
    inline float getTemperature() const;
    inline float getRotationPeriod() const;
    inline const char* getSpectralType() const;
    inline float getBolometricMagnitude() const;

    enum {
        InvalidCatalogNumber = 0xffffffff
    };

private:
    uint32 catalogNumber;
    Point3f position;
    float absMag;
    StarDetails* details;
};


Star::Star() :
    catalogNumber(InvalidCatalogNumber),
    position(0, 0, 0),
    absMag(4.83f),
    details(NULL)
{
}

uint32
Star::getCatalogNumber() const
{
    return catalogNumber;
}

float
Star::getAbsoluteMagnitude() const
{
    return absMag;
}

Point3f
Star::getPosition() const
{
    return position;
}

float
Star::getTemperature() const
{
    return details->getTemperature();
}

float
Star::getRotationPeriod() const
{
    return details->getRotationPeriod();
}

const char*
Star::getSpectralType() const
{
    return details->getSpectralType();
}

float
Star::getBolometricMagnitude() const
{
    return absMag + details->getBolometricCorrection();
}

#endif // _STAR_H_
