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

#include <config.h>
#include <celutil/reshandle.h>
#include <celutil/color.h>
#include <celengine/astroobj.h>
#include <celengine/univcoord.h>
#include <celengine/stellarclass.h>
#include <celengine/multitexture.h>
#include <celephem/rotation.h>
#include <Eigen/Core>
#include <vector>

class Selection;
class Orbit;
class Star;
class AstroDatabase;

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
    inline bool hasCorona() const;

    enum
    {
        KnowRadius   = 0x1,
        KnowRotation = 0x2,
        KnowTexture  = 0x4,
    };
    inline uint32_t getKnowledge() const;
    inline bool getKnowledge(uint32_t) const;
    void setKnowledge(uint32_t);
    void addKnowledge(uint32_t);

 private:
    void addOrbitingStar(Star*);

 private:
    float radius{ 0.0f };
    float temperature{ 0.0f };
    float bolometricCorrection{ 0.0f };

    uint32_t knowledge{ 0 };
    bool visible{ true };
    char spectralType[8];

    MultiResTexture texture{ InvalidResource };
    ResourceHandle geometry{ InvalidResource };

    Orbit* orbit{ nullptr };
    float orbitalRadius{ 0.0f };
    Star* barycenter{ nullptr };

    const RotationModel* rotationModel{ nullptr };

    Eigen::Vector3f semiAxes{ Eigen::Vector3f::Ones() };

    std::string infoURL;

    std::vector<Star*>* orbitingStars{ nullptr };
    bool isShared{ true };

 public:
    struct StarTextureSet
    {
        MultiResTexture defaultTex;
        MultiResTexture neutronStarTex;
        MultiResTexture starTex[StellarClass::Spectral_Count];
    };

 public:
    static StarDetails* GetStarDetails(const StellarClass&);
    static StarDetails* CreateStandardStarType(const std::string& specTypeName,
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

uint32_t
StarDetails::getKnowledge() const
{
    return knowledge;
}

bool
StarDetails::getKnowledge(uint32_t knowledgeFlags) const
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

bool
StarDetails::hasCorona() const
{
    // Y dwarves and T dwarves subclasses 5-9 don't have a corona
    return spectralType[0] != 'Y' && (spectralType[0] != 'T' || spectralType[1] < '5');
}



class Star : public AstroObject
{
public:
    Star() = default;
    virtual ~Star();

    virtual Selection toSelection();

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
    inline uint32_t getKnowledge() const;
    inline const RotationModel* getRotationModel() const;
    inline Eigen::Vector3f getEllipsoidSemiAxes() const;
    const std::string& getInfoURL() const;
    inline bool hasCorona() const;

    enum : uint32_t {
        MaxTychoCatalogNumber = 0xf0000000
    };

    static bool createStar(Star* star,
                        DataDisposition disposition,
                        Hash* starData,
                        const string& path,
                        bool isBarycenter,
                        AstroDatabase *);
private:
    Eigen::Vector3f position{ Eigen::Vector3f::Zero() };
    float absMag{ 4.83f };
    StarDetails* details{ nullptr };
};


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

bool
Star::hasCorona() const
{
    return details->hasCorona();
}

#endif // _CELENGINE_STAR_H_
