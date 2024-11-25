// body.h
//
// Copyright (C) 2001-2006 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <celutil/color.h>
#include <celutil/flag.h>
#include <celutil/ranges.h>
#include <celutil/reshandle.h>
#include <celutil/utf8.h>
#include "multitexture.h"
#include "surface.h"

class Atmosphere;
class Body;
class FrameTree;
class Location;
class ReferenceFrame;
class ReferenceMark;
class Star;
class StarDatabase;
class Timeline;
class UniversalCoord;

namespace celestia
{
namespace engine
{
class Completion;
} // end namespace celestia::engine

namespace ephem
{
class Orbit;
class RotationModel;
} // end namespace celestia::ephem
} // end namespace celestia

class PlanetarySystem //NOSONAR
{
public:
    explicit PlanetarySystem(Body* _primary);
    explicit PlanetarySystem(Star* _star);
    ~PlanetarySystem();

    Star* getStar() const { return star; };
    Body* getPrimaryBody() const { return primary; };
    int getSystemSize() const { return satellites.size(); };
    Body* getBody(int i) const { return satellites[i].get(); };

    void addAlias(Body* body, const std::string& alias);
    Body* addBody(const std::string& name);
    void removeBody(const Body* body);

    enum TraversalResult
    {
        ContinueTraversal   = 0,
        StopTraversal       = 1
    };

    Body* find(std::string_view, bool deepSearch = false, bool i18n = false) const;
    void getCompletion(std::vector<celestia::engine::Completion>& completion, std::string_view _name, bool rec = true) const;

private:
    void addBodyToNameIndex(Body* body);
    void removeBodyFromNameIndex(const Body* body);

    using ObjectIndex = std::map<std::string, Body*, UTF8StringOrderingPredicate>;

    Star* star;
    Body* primary{nullptr};
    std::vector<std::unique_ptr<Body>> satellites;
    ObjectIndex objectIndex;  // index of bodies by name
};

struct RingSystem
{
    float innerRadius;
    float outerRadius;
    Color color;
    MultiResTexture texture;

    RingSystem(float inner, float outer) :
        innerRadius(inner), outerRadius(outer),
        color(1.0f, 1.0f, 1.0f),
        texture()
        { };
    RingSystem(float inner, float outer, Color _color, int _loTexture = -1, int _texture = -1) :
        innerRadius(inner), outerRadius(outer), color(_color), texture(_loTexture, _texture)
        { };
    RingSystem(float inner, float outer, Color _color, const MultiResTexture& _texture) :
        innerRadius(inner), outerRadius(outer), color(_color), texture(_texture)
        { };
};

// Object class enumeration:
// All of these values must be powers of two so that they can
// be used in an object type bit mask.
//
// The values of object class enumerants cannot be modified
// without consequence. The object orbit mask is a stored user
// setting, so there will be unexpected results when the user
// upgrades if the orbit mask values mean something different
// in the new version.
//
// * Planet, Moon, Asteroid, DwarfPlanet, and MinorMoon all behave
// essentially the same. They're distinguished from each other for
// user convenience, so that it's possible to assign them different
// orbit and label colors, and to categorize them in the solar
// system browser.
//
// * Comet is identical to the asteroid class except that comets may
// be rendered with dust and ion tails.
//
// Other classes have different default settings for the properties
// Clickable, VisibleAsPoint, Visible, and SecondaryIlluminator. These
// defaults are assigned in the ssc file parser and may be overridden
// for a particular body.
//
// * Invisible is used for barycenters and other reference points.
// An invisible object is not clickable, visibleAsPoint, visible, or
// a secondary illuminator.
//
// * SurfaceFeature is meant to be used for buildings and landscape.
// SurfaceFeatures is clickable and visible, but not visibleAsPoint or
// a secondary illuminator.
//
// * Component should be used for parts of spacecraft or buildings that
// are separate ssc objects. A component is clickable and visible, but
// not visibleAsPoint or a secondary illuminator.
//
// * Diffuse is used for gas clouds, dust plumes, and the like. They are
// visible, but other properties are false by default. It is expected
// that an observer will move through a diffuse object, so there's no
// need for any sort of collision detection to be applied.
//
// * Stellar is a pseudo-class used only for orbit rendering.
//
// * Barycenter and SmallBody are not used currently. Invisible is used
// instead of barycenter.
enum class BodyClassification : std::uint32_t
{
    EmptyMask      =        0,
    Planet         =     0x01,
    Moon           =     0x02,
    Asteroid       =     0x04,
    Comet          =     0x08,
    Spacecraft     =     0x10,
    Invisible      =     0x20,
    Barycenter     =     0x40, // Not used (invisible is used instead)
    SmallBody      =     0x80, // Not used
    DwarfPlanet    =    0x100,
    Stellar        =    0x200, // only used for orbit mask
    SurfaceFeature =    0x400,
    Component      =    0x800,
    MinorMoon      =   0x1000,
    Diffuse        =   0x2000,
    Unknown        =  0x10000,
};

ENUM_CLASS_BITWISE_OPS(BodyClassification);

enum class BodyFeatures : std::uint8_t
{
    None              = 0x00,
    Atmosphere        = 0x01,
    Rings             = 0x02,
    AlternateSurfaces = 0x04,
    Locations         = 0x08,
    ReferenceMarks    = 0x10,
    OrbitColor        = 0x20,
    CometTailColor    = 0x40,
};

ENUM_CLASS_BITWISE_OPS(BodyFeatures);

class BodyFeaturesManager;

class Body //NOSONAR
{
public:
     Body(PlanetarySystem*, const std::string& name);
     ~Body();

    enum VisibilityPolicy
    {
        NeverVisible       = 0,
        UseClassVisibility = 1,
        AlwaysVisible      = 2,
    };

    void setDefaultProperties();

    PlanetarySystem* getSystem() const;
    const std::vector<std::string>& getNames() const;
    const std::string& getName(bool i18n = false) const;
    std::string getPath(const StarDatabase*, char delimiter = '/') const;
    const std::string& getLocalizedName() const;
    bool hasLocalizedName() const;
    void addAlias(const std::string& alias);

    void setTimeline(std::unique_ptr<Timeline>&& timeline);
    const Timeline* getTimeline() const;

    FrameTree* getFrameTree() const;
    FrameTree* getOrCreateFrameTree();

    const std::shared_ptr<const ReferenceFrame>& getOrbitFrame(double tdb) const;
    const celestia::ephem::Orbit* getOrbit(double tdb) const;
    const std::shared_ptr<const ReferenceFrame>& getBodyFrame(double tdb) const;
    const celestia::ephem::RotationModel* getRotationModel(double tdb) const;

    // Size methods
    void setSemiAxes(const Eigen::Vector3f&);
    const Eigen::Vector3f& getSemiAxes() const;
    float getRadius() const;

    bool isSphere() const;
    bool isEllipsoid() const;

    float getMass() const;
    void setMass(float);
    float getDensity() const;
    void setDensity(float);

    // Albedo functions and temperature
    float getGeomAlbedo() const;
    void setGeomAlbedo(float);
    float getBondAlbedo() const;
    void setBondAlbedo(float);
    float getReflectivity() const;
    void setReflectivity(float);
    float getTemperature(double t = 0) const;
    void setTemperature(float);
    float getTempDiscrepancy() const;
    void setTempDiscrepancy(float);

    BodyClassification getClassification() const;
    void setClassification(BodyClassification);
    const std::string& getInfoURL() const;
    void setInfoURL(std::string&&);

    PlanetarySystem* getSatellites() const;
    PlanetarySystem* getOrCreateSatellites();

    float getBoundingRadius() const;
    float getCullingRadius() const;

    ResourceHandle getGeometry() const { return geometry; }
    void setGeometry(ResourceHandle);
    Eigen::Quaternionf getGeometryOrientation() const;
    void setGeometryOrientation(const Eigen::Quaternionf& orientation);
    float getGeometryScale() const { return geometryScale; }
    void setGeometryScale(float scale);

    void setSurface(const Surface&);
    const Surface& getSurface() const;
    Surface& getSurface();

    float getLuminosity(const Star& sun,
                        float distanceFromSun) const;
    float getLuminosity(float sunLuminosity,
                        float distanceFromSun) const;

    float getApparentMagnitude(const Star& sun,
                               float distanceFromSun,
                               float distanceFromViewer) const;
    float getApparentMagnitude(float sunLuminosity,
                               float distanceFromSun,
                               float distanceFromViewer) const;
    float getApparentMagnitude(const Star& sun,
                               const Eigen::Vector3d& sunPosition,
                               const Eigen::Vector3d& viewerPosition) const;
    float getApparentMagnitude(float sunLuminosity,
                               const Eigen::Vector3d& sunPosition,
                               const Eigen::Vector3d& viewerPosition) const;

    UniversalCoord getPosition(double tdb) const;
    Eigen::Quaterniond getOrientation(double tdb) const;
    Eigen::Vector3d getVelocity(double tdb) const;
    Eigen::Vector3d getAngularVelocity(double tdb) const;

    Eigen::Matrix4d getLocalToAstrocentric(double) const;
    Eigen::Vector3d getAstrocentricPosition(double) const;
    Eigen::Quaterniond getEquatorialToBodyFixed(double) const;
    Eigen::Quaterniond getEclipticToFrame(double) const;
    Eigen::Quaterniond getEclipticToEquatorial(double) const;
    /*! Get a rotation that converts from the ecliptic frame to this
    *  objects's body fixed frame.
    */
    inline Eigen::Quaterniond getEclipticToBodyFixed(double tdb) const { return getOrientation(tdb); }
    Eigen::Matrix4d getBodyFixedToAstrocentric(double) const;

    Eigen::Vector3d planetocentricToCartesian(double lon, double lat, double alt) const;
    Eigen::Vector3d planetocentricToCartesian(const Eigen::Vector3d& lonLatAlt) const;
    Eigen::Vector3d cartesianToPlanetocentric(const Eigen::Vector3d& v) const;

    Eigen::Vector3d geodeticToCartesian(double lon, double lat, double alt) const;
    Eigen::Vector3d geodeticToCartesian(const Eigen::Vector3d& lonLatAlt) const;

    Eigen::Vector3d eclipticToPlanetocentric(const Eigen::Vector3d& ecl, double tdb) const;

    bool extant(double) const;
    void getLifespan(double&, double&) const;

    bool isVisible() const { return visible; }
    void setVisible(bool _visible);
    bool isClickable() const { return clickable; }
    void setClickable(bool _clickable);
    bool isVisibleAsPoint() const;
    bool isSecondaryIlluminator() const;

    bool hasVisibleGeometry() const { return classification != BodyClassification::Invisible && visible; }

    VisibilityPolicy getOrbitVisibility() const { return orbitVisibility; }
    void setOrbitVisibility(VisibilityPolicy _orbitVisibility);

    BodyClassification getOrbitClassification() const;

    enum
    {
        BodyAxes       =   0x01,
        FrameAxes      =   0x02,
        LongLatGrid    =   0x04,
        SunDirection   =   0x08,
        VelocityVector =   0x10,
    };

    void markChanged();
    void markUpdated();
    void recomputeCullingRadius();

private:
    void setName(const std::string& name);

    std::vector<std::string> names{ 1 };
    std::string localizedName;

    // Parent in the name hierarchy
    PlanetarySystem* system;
    // Children in the name hierarchy
    std::unique_ptr<PlanetarySystem> satellites;

    std::unique_ptr<Timeline> timeline;
    // Children in the frame hierarchy
    std::unique_ptr<FrameTree> frameTree;

    float radius{ 1.0f };
    Eigen::Vector3f semiAxes{ Eigen::Vector3f::Ones() };
    float mass{ 0.0f };
    float density{ 0.0f };
    float geomAlbedo{ 0.5f };
    float bondAlbedo{ 0.5f };
    float reflectivity{ 0.5f };
    float temperature{ 0.0f };
    float tempDiscrepancy{ 0.0f };

    Eigen::Quaternionf geometryOrientation{ Eigen::Quaternionf::Identity() };

    float cullingRadius{ 0.0f };

    ResourceHandle geometry{ InvalidResource };
    float geometryScale{ 1.0f };
    Surface surface{ Color(1.0f, 1.0f, 1.0f) };

    BodyClassification classification{ BodyClassification::Unknown };

    std::string infoURL;

    // Track enabled features to allow fast rejection during lookup
    BodyFeatures features{ BodyFeatures::None };

    bool visible{ true };
    bool clickable{ true };
    VisibilityPolicy orbitVisibility : 3;

    friend class BodyFeaturesManager;
};

struct BodyLocations //NOSONAR
{
    BodyLocations() = default;
    ~BodyLocations();

    std::vector<std::unique_ptr<Location>> locations;
    bool locationsComputed;
};

class BodyFeaturesManager
{
public:
    BodyFeaturesManager() = default;
    ~BodyFeaturesManager() = default;

    BodyFeaturesManager(const BodyFeaturesManager&) = delete;
    BodyFeaturesManager(BodyFeaturesManager&&) = delete;
    BodyFeaturesManager& operator=(const BodyFeaturesManager&) = delete;
    BodyFeaturesManager& operator=(BodyFeaturesManager&&) = delete;

    Atmosphere* getAtmosphere(const Body*) const;
    void setAtmosphere(Body*, std::unique_ptr<Atmosphere>&&);

    RingSystem* getRings(const Body*) const;
    void setRings(Body*, std::unique_ptr<RingSystem>&&);
    void scaleRings(Body*, float);

    Surface* getAlternateSurface(const Body*, std::string_view) const;
    void addAlternateSurface(Body*, std::string_view, std::unique_ptr<Surface>&&);

    auto getAlternateSurfaceNames(const Body* body) const
    {
        using range_type = decltype(celestia::util::keysView(std::declval<AltSurfaceTable>()));
        if (!celestia::util::is_set(body->features, BodyFeatures::AlternateSurfaces))
            return std::optional<range_type>();

        auto it = alternateSurfaces.find(body);
        assert(it != alternateSurfaces.end());
        return std::make_optional(celestia::util::keysView(*it->second));
    }

    void addReferenceMark(Body*, std::unique_ptr<ReferenceMark>&&);
    bool removeReferenceMark(Body*, std::string_view tag);
    const ReferenceMark* findReferenceMark(const Body*, std::string_view tag) const;

    template<typename F>
    void processReferenceMarks(const Body* body, F&& processor) const
    {
        if (!celestia::util::is_set(body->features, BodyFeatures::ReferenceMarks))
            return;

        auto [start, end] = referenceMarks.equal_range(body);
        for (auto it = start; it != end; ++it)
        {
            processor(it->second.get());
        }
    }

    void addLocation(Body*, std::unique_ptr<Location>&&);
    Location* findLocation(const Body*, std::string_view, bool i18n = false) const;
    bool hasLocations(const Body*) const;
    void computeLocations(const Body*);

    auto getLocations(const Body* body) const
    {
        using range_type = decltype(celestia::util::pointerView(std::declval<BodyLocations>().locations));
        if (!celestia::util::is_set(body->features, BodyFeatures::Locations))
            return std::optional<range_type>();

        auto it = locations.find(body);
        assert(it != locations.end());
        return std::make_optional(celestia::util::pointerView(it->second.locations));
    }

    bool getOrbitColor(const Body*, Color&) const;
    void setOrbitColor(const Body*, const Color&);
    bool getOrbitColorOverridden(const Body*) const;
    void setOrbitColorOverridden(Body*, bool);
    void unsetOrbitColor(Body*);

    Color getCometTailColor(const Body*) const;
    void setCometTailColor(Body*, const Color&);
    void unsetCometTailColor(Body*);

    void removeFeatures(Body*);

private:
    using AltSurfaceTable = std::map<std::string, std::unique_ptr<Surface>, std::less<>>;

    std::unordered_map<const Body*, std::unique_ptr<Atmosphere>> atmospheres;
    std::unordered_map<const Body*, std::unique_ptr<RingSystem>> rings;
    std::unordered_map<const Body*, std::unique_ptr<AltSurfaceTable>> alternateSurfaces;
    std::unordered_map<const Body*, BodyLocations> locations;
    std::unordered_map<const Body*, Color> orbitColors;
    std::unordered_map<const Body*, Color> cometTailColors;
    std::unordered_multimap<const Body*, std::unique_ptr<ReferenceMark>> referenceMarks;
};

BodyFeaturesManager* GetBodyFeaturesManager();
