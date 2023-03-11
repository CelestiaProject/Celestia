// star.h
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <Eigen/Core>

#include <celengine/astroobj.h>
#include <celengine/multitexture.h>
#include <celengine/stellarclass.h>
#include <celutil/reshandle.h>

class Selection;
class Star;
class UniversalCoord;

namespace celestia::ephem
{
class Orbit;
class RotationModel;
}

class StarDetails
{
 public:
    struct StarTextureSet
    {
        MultiResTexture defaultTex{ };
        MultiResTexture neutronStarTex{ };
        std::array<MultiResTexture, StellarClass::Spectral_Count> starTex{ };
    };

    StarDetails();
    ~StarDetails() = default;
    StarDetails(const StarDetails&);
    // Prohibit assignment of StarDetails objects
    StarDetails& operator=(const StarDetails&) = delete;

    float getRadius() const;
    float getTemperature() const;
    ResourceHandle getGeometry() const;
    MultiResTexture getTexture() const;
    celestia::ephem::Orbit* getOrbit() const;
    float getOrbitalRadius() const;
    const char* getSpectralType() const;
    float getBolometricCorrection() const;
    Star* getOrbitBarycenter() const;
    bool getVisibility() const;
    const celestia::ephem::RotationModel* getRotationModel() const;
    Eigen::Vector3f getEllipsoidSemiAxes() const;
    const std::string& getInfoURL() const;

    void setRadius(float);
    void setTemperature(float);
    void setSpectralType(std::string_view);
    void setBolometricCorrection(float);
    void setTexture(const MultiResTexture&);
    void setGeometry(ResourceHandle);
    void setOrbit(celestia::ephem::Orbit*);
    void setOrbitBarycenter(Star*);
    void setOrbitalRadius(float);
    void computeOrbitalRadius();
    void setVisibility(bool);
    void setRotationModel(const celestia::ephem::RotationModel*);
    void setEllipsoidSemiAxes(const Eigen::Vector3f&);
    void setInfoURL(std::string_view _infoURL);

    bool shared() const;
    inline bool hasCorona() const;

    enum
    {
        KnowRadius   = 0x1,
        KnowRotation = 0x2,
        KnowTexture  = 0x4,
    };
    std::uint32_t getKnowledge() const;
    bool getKnowledge(std::uint32_t) const;
    void setKnowledge(std::uint32_t);
    void addKnowledge(std::uint32_t);

    static StarDetails* GetStarDetails(const StellarClass&);
    static StarDetails* GetBarycenterDetails();

    static void SetStarTextures(const StarTextureSet&);

 private:
    friend class Star;

    void addOrbitingStar(Star*);

    float radius{ 0.0f };
    float temperature{ 0.0f };
    float bolometricCorrection{ 0.0f };

    std::uint32_t knowledge{ 0 };
    bool visible{ true };
    std::array<char, 8> spectralType{ };

    MultiResTexture texture{ InvalidResource };
    ResourceHandle geometry{ InvalidResource };

    celestia::ephem::Orbit* orbit{ nullptr };
    float orbitalRadius{ 0.0f };
    Star* barycenter{ nullptr };

    const celestia::ephem::RotationModel* rotationModel{ nullptr };

    Eigen::Vector3f semiAxes{ Eigen::Vector3f::Ones() };

    std::string infoURL;

    std::unique_ptr<std::vector<Star*>> orbitingStars{ nullptr };
    bool isShared{ true };
};


inline float
StarDetails::getRadius() const
{
    return radius;
}

inline float
StarDetails::getTemperature() const
{
    return temperature;
}

inline ResourceHandle
StarDetails::getGeometry() const
{
    return geometry;
}

inline MultiResTexture
StarDetails::getTexture() const
{
    return texture;
}

inline celestia::ephem::Orbit*
StarDetails::getOrbit() const
{
    return orbit;
}

inline float
StarDetails::getOrbitalRadius() const
{
    return orbitalRadius;
}

inline std::uint32_t
StarDetails::getKnowledge() const
{
    return knowledge;
}

inline bool
StarDetails::getKnowledge(std::uint32_t knowledgeFlags) const
{
    return ((knowledge & knowledgeFlags) == knowledgeFlags);
}

inline const char*
StarDetails::getSpectralType() const
{
    return spectralType.data();
}

inline float
StarDetails::getBolometricCorrection() const
{
    return bolometricCorrection;
}

inline Star*
StarDetails::getOrbitBarycenter() const
{
    return barycenter;
}

inline bool
StarDetails::getVisibility() const
{
    return visible;
}

inline const celestia::ephem::RotationModel*
StarDetails::getRotationModel() const
{
    return rotationModel;
}

inline Eigen::Vector3f
StarDetails::getEllipsoidSemiAxes() const
{
    return semiAxes;
}

bool
StarDetails::hasCorona() const
{
    // Y dwarfs and T dwarf subclasses 5-9 don't have a corona
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
    float getBolometricLuminosity() const;

    void setExtinction(float);
    float getExtinction() const
    {
        return extinction;
    }

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

    void setRotationModel(const celestia::ephem::RotationModel*);

    void addOrbitingStar(Star*);
    const std::vector<Star*>* getOrbitingStars() const;

    // Accessor methods that delegate to StarDetails
    float getRadius() const;
    float getTemperature() const;
    const char* getSpectralType() const;
    float getBolometricMagnitude() const;
    MultiResTexture getTexture() const;
    ResourceHandle getGeometry() const;
    celestia::ephem::Orbit* getOrbit() const;
    float getOrbitalRadius() const;
    Star* getOrbitBarycenter() const;
    bool getVisibility() const;
    const celestia::ephem::RotationModel* getRotationModel() const;
    Eigen::Vector3f getEllipsoidSemiAxes() const;
    const std::string& getInfoURL() const;
    bool hasCorona() const;

    static constexpr AstroCatalog::IndexNumber MaxTychoCatalogNumber = 0xf0000000;

private:
    Eigen::Vector3f position{ Eigen::Vector3f::Zero() };
    float absMag{ 4.83f };
    float extinction{ 0.0f };
    StarDetails* details{ nullptr };
};


inline float
Star::getTemperature() const
{
    return details->getTemperature();
}

inline const char*
Star::getSpectralType() const
{
    return details->getSpectralType();
}

inline float
Star::getBolometricMagnitude() const
{
    return absMag + details->getBolometricCorrection();
}

inline celestia::ephem::Orbit*
Star::getOrbit() const
{
    return details->getOrbit();
}

inline float
Star::getOrbitalRadius() const
{
    return details->getOrbitalRadius();
}

inline Star*
Star::getOrbitBarycenter() const
{
    return details->getOrbitBarycenter();
}

inline bool
Star::getVisibility() const
{
    return details->getVisibility();
}

inline const celestia::ephem::RotationModel*
Star::getRotationModel() const
{
    return details->getRotationModel();
}

inline Eigen::Vector3f
Star::getEllipsoidSemiAxes() const
{
    return details->getEllipsoidSemiAxes();
}

inline const std::vector<Star*>*
Star::getOrbitingStars() const
{
    return details->orbitingStars.get();
}

inline bool
Star::hasCorona() const
{
    return details->hasCorona();
}
