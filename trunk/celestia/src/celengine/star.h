// star.h
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_STAR_H_
#define _CELENGINE_STAR_H_

#include <celutil/basictypes.h>
#include <celutil/reshandle.h>
#include <celutil/color.h>
#include <celengine/univcoord.h>
#include <celengine/celestia.h>
#include <celengine/stellarclass.h>
#include <celengine/rotation.h>
#include <celengine/multitexture.h>
#include <Eigen/Core>
#include <vector>

class Orbit;
class Star;

class StarDetails
{
    friend class Star;

 public:
    StarDetails();
    StarDetails(const StarDetails&);

    ~StarDetails();
    
 private:
    // Prohibit assignment of StarDetails objects
    StarDetails& operator=(const StarDetails&);

 public:
    inline float getRadius() const;
    inline float getTemperature() const;
    inline ResourceHandle getGeometry() const;
    inline MultiResTexture getTexture() const;
    inline Orbit* getOrbit() const;
    inline float getOrbitalRadius() const;
    inline const char* getSpectralType() const;
    inline float getBolometricCorrection() const;
    inline Star* getOrbitBarycenter() const;
    inline bool getVisibility() const;
    inline const RotationModel* getRotationModel() const;
    inline Eigen::Vector3f getEllipsoidSemiAxes() const;
    const std::string& getInfoURL() const;

    void setRadius(float);
    void setTemperature(float);
    void setSpectralType(const std::string&);
    void setBolometricCorrection(float);
    void setTexture(const MultiResTexture&);
    void setGeometry(ResourceHandle);
    void setOrbit(Orbit*);
    void setOrbitBarycenter(Star*);
    void setOrbitalRadius(float);
    void computeOrbitalRadius();
    void setVisibility(bool);
    void setRotationModel(const RotationModel*);
    void setEllipsoidSemiAxes(const Eigen::Vector3f&);
    void setInfoURL(const std::string& _infoURL);

    bool shared() const;
    
    enum
    {
        KnowRadius   = 0x1,
        KnowRotation = 0x2,
        KnowTexture  = 0x4,
    };
    inline uint32 getKnowledge() const;
    inline bool getKnowledge(uint32) const;
    void setKnowledge(uint32);
    void addKnowledge(uint32);

 private:
    void addOrbitingStar(Star*);

 private:
    float radius;
    float temperature;
    float bolometricCorrection;

    uint32 knowledge;
    bool visible;
    char spectralType[8];

    MultiResTexture texture;
    ResourceHandle geometry;

    Orbit* orbit;
    float orbitalRadius;
    Star* barycenter;

    const RotationModel* rotationModel;

    Eigen::Vector3f semiAxes;

    std::string* infoURL;
    
    std::vector<Star*>* orbitingStars;
    bool isShared;

 public:
    struct StarTextureSet
    {
        MultiResTexture defaultTex;
        MultiResTexture neutronStarTex;
        MultiResTexture starTex[StellarClass::Spectral_Count];
    };
    
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
    static StarDetails* GetBarycenterDetails();
    
    static void SetStarTextures(const StarTextureSet&);
    
 private:
    static StarTextureSet starTextures;
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

ResourceHandle
StarDetails::getGeometry() const
{
    return geometry;
}

MultiResTexture
StarDetails::getTexture() const
{
    return texture;
}

Orbit*
StarDetails::getOrbit() const
{
    return orbit;
}

float
StarDetails::getOrbitalRadius() const
{
    return orbitalRadius;
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

Star*
StarDetails::getOrbitBarycenter() const
{
    return barycenter;
}

bool
StarDetails::getVisibility() const
{
    return visible;
}

const RotationModel*
StarDetails::getRotationModel() const
{
    return rotationModel;
}

Eigen::Vector3f
StarDetails::getEllipsoidSemiAxes() const
{
    return semiAxes;
}



class Star
{
public:
    inline Star();
    ~Star();

    inline uint32 getCatalogNumber() const
    {
        return catalogNumber;
    }

    /** This getPosition() method returns the approximate star position; that is,
     *  star position without any orbital motion taken into account.  For a
     *  star in an orbit, the position should be set to the 'root' barycenter
     *  of the system.
     */
    Eigen::Vector3f getPosition() const
    {
        return position;
    }

    float getAbsoluteMagnitude() const
    {
        return absMag;
    }

    float getApparentMagnitude(float) const;
    float getLuminosity() const;

    // Return the exact position of the star, accounting for its orbit
    UniversalCoord getPosition(double t) const;
    UniversalCoord getOrbitBarycenterPosition(double t) const;

    Eigen::Vector3d getVelocity(double t) const;

    void setCatalogNumber(uint32);
    void setPosition(float, float, float);
    void setPosition(const Eigen::Vector3f& positionLy);
    void setAbsoluteMagnitude(float);
    void setLuminosity(float);

    StarDetails* getDetails() const;
    void setDetails(StarDetails*);
    void setOrbitBarycenter(Star*);
    void computeOrbitalRadius();

    void setRotationModel(const RotationModel*);

    void addOrbitingStar(Star*);
    inline const std::vector<Star*>* getOrbitingStars() const;

    // Accessor methods that delegate to StarDetails
    float getRadius() const;
    inline float getTemperature() const;
    inline const char* getSpectralType() const;
    inline float getBolometricMagnitude() const;
    MultiResTexture getTexture() const;
    ResourceHandle getGeometry() const;
    inline Orbit* getOrbit() const;
    inline float getOrbitalRadius() const;
    inline Star* getOrbitBarycenter() const;
    inline bool getVisibility() const;
    inline uint32 getKnowledge() const;
    inline const RotationModel* getRotationModel() const;
    inline Eigen::Vector3f getEllipsoidSemiAxes() const;
    const std::string& getInfoURL() const;

    enum {
        MaxTychoCatalogNumber = 0xf0000000,
        InvalidCatalogNumber = 0xffffffff,
    };

private:
    uint32 catalogNumber;
    Eigen::Vector3f position;
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

float
Star::getTemperature() const
{
    return details->getTemperature();
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

Orbit*
Star::getOrbit() const
{
    return details->getOrbit();
}

float
Star::getOrbitalRadius() const
{
    return details->getOrbitalRadius();
}

Star*
Star::getOrbitBarycenter() const
{
    return details->getOrbitBarycenter();
}

bool
Star::getVisibility() const
{
    return details->getVisibility();
}

const RotationModel*
Star::getRotationModel() const
{
    return details->getRotationModel();
}

Eigen::Vector3f
Star::getEllipsoidSemiAxes() const
{
    return details->getEllipsoidSemiAxes();
}

const std::vector<Star*>*
Star::getOrbitingStars() const
{
    return details->orbitingStars;
}

#endif // _CELENGINE_STAR_H_
