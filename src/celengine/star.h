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
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <Eigen/Core>

#include <celengine/astroobj.h>
#include <celengine/multitexture.h>
#include <celengine/stellarclass.h>
#include <celutil/array_view.h>
#include <celutil/flag.h>
#include <celutil/reshandle.h>

class Selection;
class Star;
class StarDatabaseBuilder;
class UniversalCoord;

namespace celestia::ephem
{
class Orbit;
class RotationModel;
}

class StarDetailsManager;

class StarDetails
{
public:
    struct StarTextureSet
    {
        MultiResTexture defaultTex{ };
        MultiResTexture neutronStarTex{ };
        std::array<MultiResTexture, StellarClass::Spectral_Count> starTex{ };
    };

    ~StarDetails() = default;
    StarDetails(const StarDetails&) = delete;
    StarDetails& operator=(const StarDetails&) = delete;
    StarDetails(StarDetails&&) = delete;
    StarDetails& operator=(StarDetails&&) = delete;

    boost::intrusive_ptr<StarDetails> clone() const;
    void mergeFromStandard(const StarDetails* standardDetails);

    float getRadius() const;
    float getTemperature() const;
    ResourceHandle getGeometry() const;
    MultiResTexture getTexture() const;
    const celestia::ephem::Orbit* getOrbit() const;
    float getOrbitalRadius() const;
    const char* getSpectralType() const;
    float getBolometricCorrection() const;
    Star* getOrbitBarycenter() const;
    bool getVisibility() const;
    const std::shared_ptr<const celestia::ephem::RotationModel>& getRotationModel() const;
    Eigen::Vector3f getEllipsoidSemiAxes() const;
    const std::string& getInfoURL() const;
    bool isBarycenter() const;

    static void setRadius(boost::intrusive_ptr<StarDetails>&, float);
    static void setTemperature(boost::intrusive_ptr<StarDetails>&, float);
    static void setBolometricCorrection(boost::intrusive_ptr<StarDetails>&, float);
    static void setTexture(boost::intrusive_ptr<StarDetails>&, const MultiResTexture&);
    static void setGeometry(boost::intrusive_ptr<StarDetails>&, ResourceHandle);
    static void setOrbit(boost::intrusive_ptr<StarDetails>&, const std::shared_ptr<const celestia::ephem::Orbit>&);
    static void setOrbitBarycenter(boost::intrusive_ptr<StarDetails>&, Star*);
    static void setVisibility(boost::intrusive_ptr<StarDetails>&, bool);
    static void setRotationModel(boost::intrusive_ptr<StarDetails>&, const std::shared_ptr<const celestia::ephem::RotationModel>&);
    static void setEllipsoidSemiAxes(boost::intrusive_ptr<StarDetails>&, const Eigen::Vector3f&);
    static void setInfoURL(boost::intrusive_ptr<StarDetails>&, std::string_view _infoURL);
    static void addOrbitingStar(boost::intrusive_ptr<StarDetails>&, Star*);

    bool shared() const;
    inline bool hasCorona() const;

    enum class Knowledge : unsigned int
    {
        None         = 0,
        KnowRadius   = 0x1,
        KnowRotation = 0x2,
        KnowTexture  = 0x4,
    };

    static boost::intrusive_ptr<StarDetails> GetStarDetails(const StellarClass&);
    static boost::intrusive_ptr<StarDetails> GetBarycenterDetails();

    static void SetStarTextures(const StarTextureSet&);

private:
    StarDetails();

    friend class Star;

    void computeOrbitalRadius();

    inline friend void
    intrusive_ptr_add_ref(StarDetails* p)
    {
        p->refCount.fetch_add(1, std::memory_order_relaxed);
    }

    inline friend void
    intrusive_ptr_release(StarDetails* p)
    {
        if (p->refCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
            delete p;
    }

    std::atomic_uint32_t refCount{ 1 };

    float radius{ 0.0f };
    float temperature{ 0.0f };
    float bolometricCorrection{ 0.0f };

    Knowledge knowledge{ Knowledge::None };
    bool visible{ true };
    std::array<char, 8> spectralType{ };

    MultiResTexture texture{ InvalidResource };
    ResourceHandle geometry{ InvalidResource };

    std::shared_ptr<const celestia::ephem::Orbit> orbit;
    float orbitalRadius{ 0.0f };
    Star* barycenter{ nullptr };

    std::shared_ptr<const celestia::ephem::RotationModel> rotationModel;

    Eigen::Vector3f semiAxes{ Eigen::Vector3f::Ones() };

    std::string infoURL;

    std::unique_ptr<std::vector<Star*>> orbitingStars{ nullptr };
    bool isShared{ true };

    friend class StarDetailsManager;
};

ENUM_CLASS_BITWISE_OPS(StarDetails::Knowledge);

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

inline const celestia::ephem::Orbit*
StarDetails::getOrbit() const
{
    return orbit.get();
}

inline float
StarDetails::getOrbitalRadius() const
{
    return orbitalRadius;
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

inline const std::shared_ptr<const celestia::ephem::RotationModel>&
StarDetails::getRotationModel() const
{
    return rotationModel;
}

inline Eigen::Vector3f
StarDetails::getEllipsoidSemiAxes() const
{
    return semiAxes;
}

inline bool
StarDetails::hasCorona() const
{
    // Y dwarfs and T dwarf subclasses 5-9 don't have a corona
    return spectralType[0] != 'Y' && (spectralType[0] != 'T' || spectralType[1] < '5');
}

inline bool
StarDetails::isBarycenter() const
{
    using namespace std::string_view_literals;
    return spectralType.data() == "Bary"sv;
}

class Star
{
public:
    static constexpr AstroCatalog::IndexNumber MaxTychoCatalogNumber = 0xf0000000;

    // Required for array initialization in star octree builder
    Star() = default;
    Star(AstroCatalog::IndexNumber, const boost::intrusive_ptr<StarDetails>&);

    Star(const Star&) = default;
    Star& operator=(const Star&) = default;
    Star(Star&&) noexcept = default;
    Star& operator=(Star&&) noexcept = default;

    AstroCatalog::IndexNumber getIndex() const;
    void setIndex(AstroCatalog::IndexNumber idx);

    /** This getPosition() method returns the approximate star position; that is,
     *  star position without any orbital motion taken into account.  For a
     *  star in an orbit, the position should be set to the 'root' barycenter
     *  of the system.
     */
    const Eigen::Vector3f& getPosition() const;
    void setPosition(const Eigen::Vector3f& positionLy);

    float getAbsoluteMagnitude() const;
    void setAbsoluteMagnitude(float);

    float getExtinction() const;
    void setExtinction(float);

    float getApparentMagnitude(float) const;
    float getLuminosity() const;
    float getBolometricLuminosity() const;

    // Return the exact position of the star, accounting for its orbit
    UniversalCoord getPosition(double t) const;
    UniversalCoord getOrbitBarycenterPosition(double t) const;

    Eigen::Vector3d getVelocity(double t) const;

    // Accessor methods that delegate to StarDetails
    float getRadius() const;
    float getTemperature() const;
    const char* getSpectralType() const;
    float getBolometricMagnitude() const;
    MultiResTexture getTexture() const;
    ResourceHandle getGeometry() const;
    const celestia::ephem::Orbit* getOrbit() const;
    float getOrbitalRadius() const;
    Star* getOrbitBarycenter() const;
    bool getVisibility() const;
    const celestia::ephem::RotationModel* getRotationModel() const;
    Eigen::Vector3f getEllipsoidSemiAxes() const;
    const std::string& getInfoURL() const;
    bool hasCorona() const;
    bool isBarycenter() const;

    celestia::util::array_view<Star*> getOrbitingStars() const;

private:
    AstroCatalog::IndexNumber indexNumber{ AstroCatalog::InvalidIndex };
    Eigen::Vector3f position{ Eigen::Vector3f::Zero() };
    float absMag{ 4.83f };
    float extinction{ 0.0f };
    boost::intrusive_ptr<StarDetails> details{ nullptr };

    friend class StarDatabaseBuilder;
};

inline AstroCatalog::IndexNumber
Star::getIndex() const
{
    return indexNumber;
}

inline const Eigen::Vector3f&
Star::getPosition() const
{
    return position;
}

inline float
Star::getAbsoluteMagnitude() const
{
    return absMag;
}

inline float
Star::getExtinction() const
{
    return extinction;
}

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

inline const celestia::ephem::Orbit*
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
    return details->getRotationModel().get();
}

inline Eigen::Vector3f
Star::getEllipsoidSemiAxes() const
{
    return details->getEllipsoidSemiAxes();
}

inline celestia::util::array_view<Star*>
Star::getOrbitingStars() const
{
    if (details->orbitingStars != nullptr)
        return *details->orbitingStars;

    return {};
}

inline bool
Star::hasCorona() const
{
    return details->hasCorona();
}

inline bool
Star::isBarycenter() const
{
    return details->isBarycenter();
}
