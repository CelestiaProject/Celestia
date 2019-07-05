// body.h
//
// Copyright (C) 2001-2006 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_BODY_H_
#define _CELENGINE_BODY_H_

#include <celengine/astroobj.h>
#include <celengine/surface.h>
#include <celengine/star.h>
#include <celengine/location.h>
#include <celengine/timeline.h>
#include <celephem/rotation.h>
#include <celephem/orbit.h>
#include <celutil/utf8.h>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <GL/glew.h>
#include <string>
#include <vector>
#include <map>
#include <list>

class Selection;
class ReferenceFrame;
class Body;
class FrameTree;
class ReferenceMark;
class Atmosphere;

class PlanetarySystem
{
 public:
    PlanetarySystem(Body* _primary);
    PlanetarySystem(Star* _star);
    ~PlanetarySystem() = default;

    Star* getStar() const { return star; };
    Body* getPrimaryBody() const { return primary; };
    int getSystemSize() const { return satellites.size(); };
    Body* getBody(int i) const { return satellites[i]; };

    void addAlias(Body* body, const std::string& alias);
    void removeAlias(const Body* body, const std::string& alias);
    void addBody(Body* body);
    void removeBody(Body* body);
    void replaceBody(Body* oldBody, Body* newBody);

    int getOrder(const Body* body) const;

    enum TraversalResult
    {
        ContinueTraversal   = 0,
        StopTraversal       = 1
    };

    typedef bool (*TraversalFunc)(Body*, void*);

    bool traverse(TraversalFunc, void*) const;
    Body* find(const std::string&, bool deepSearch = false, bool i18n = false) const;
    std::vector<Name> getCompletion(const std::string& _name, bool rec = true) const;

 private:
    void addBodyToNameIndex(Body* body);
    void removeBodyFromNameIndex(const Body* body);

 private:
    typedef std::map<std::string, Body*, UTF8StringOrderingPredicate> ObjectIndex;

 private:
    Star* star;
    Body* primary{nullptr};
    std::vector<Body*> satellites;
    ObjectIndex objectIndex;  // index of bodies by name
};


class RingSystem
{
 public:
    float innerRadius;
    float outerRadius;
    Color color;
    MultiResTexture texture;
    GLuint vboId{ 0 };

    RingSystem(float inner, float outer) :
        innerRadius(inner), outerRadius(outer),
#ifdef HDR_COMPRESS
        color(0.5f, 0.5f, 0.5f),
#else
        color(1.0f, 1.0f, 1.0f),
#endif
        texture()
        { };
    RingSystem(float inner, float outer, Color _color, int _loTexture = -1, int _texture = -1) :
        innerRadius(inner), outerRadius(outer), color(_color), texture(_loTexture, _texture)
        { };
    RingSystem(float inner, float outer, Color _color, const MultiResTexture& _texture) :
        innerRadius(inner), outerRadius(outer), color(_color), texture(_texture)
        { };
};


class Body : public AstroObject
{
 public:
     EIGEN_MAKE_ALIGNED_OPERATOR_NEW

     Body(PlanetarySystem*, const std::string& name);
     virtual ~Body();

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
    enum
    {
        Planet         =    0x01,
        Moon           =    0x02,
        Asteroid       =    0x04,
        Comet          =    0x08,
        Spacecraft     =    0x10,
        Invisible      =    0x20,
        Barycenter     =    0x40, // Not used (invisible is used instead)
        SmallBody      =    0x80, // Not used
        DwarfPlanet    =   0x100,
        Stellar        =   0x200, // only used for orbit mask
        SurfaceFeature =   0x400,
        Component      =   0x800,
        MinorMoon      =  0x1000,
        Diffuse        =  0x2000,
        Unknown        = 0x10000,
    };

    enum VisibilityPolicy
    {
        NeverVisible       = 0,
        UseClassVisibility = 1,
        AlwaysVisible      = 2,
    };

    virtual Selection toSelection();
    void setDefaultProperties();

    PlanetarySystem* getSystem() const;

    void setTimeline(Timeline* timeline);
    const Timeline* getTimeline() const;

    FrameTree* getFrameTree() const;
    FrameTree* getOrCreateFrameTree();

    const ReferenceFrame::SharedConstPtr& getOrbitFrame(double tdb) const;
    const Orbit* getOrbit(double tdb) const;
    const ReferenceFrame::SharedConstPtr& getBodyFrame(double tdb) const;
    const RotationModel* getRotationModel(double tdb) const;

    // Size methods
    void setSemiAxes(const Eigen::Vector3f&);
    Eigen::Vector3f getSemiAxes() const;
    float getRadius() const;

    bool isSphere() const;
    bool isEllipsoid() const;

    float getMass() const;
    void setMass(float);
    float getDensity() const;
    void setDensity(float);

    // Albedo functions and temperature
    /* [[deprecated]] */ float getAlbedo() const;
    /* [[deprecated]] */ void setAlbedo(float);
    float getGeomAlbedo() const;
    void setGeomAlbedo(float);
    float getBondAlbedo() const;
    void setBondAlbedo(float);
    float getTemperature(double t = 0) const;
    void setTemperature(float);
    float getTempDiscrepancy() const;
    void setTempDiscrepancy(float);

    int getClassification() const;
    void setClassification(int);
    const std::string& getInfoURL() const;
    void setInfoURL(const std::string&);

    PlanetarySystem* getSatellites() const;
    void setSatellites(PlanetarySystem*);

    float getBoundingRadius() const;
    float getCullingRadius() const;

    RingSystem* getRings() const;
    void setRings(const RingSystem&);
    const Atmosphere* getAtmosphere() const;
    Atmosphere* getAtmosphere();
    void setAtmosphere(const Atmosphere&);

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
    Eigen::Quaterniond getEclipticToBodyFixed(double) const;
    Eigen::Matrix4d getBodyFixedToAstrocentric(double) const;

    Eigen::Vector3d planetocentricToCartesian(double lon, double lat, double alt) const;
    Eigen::Vector3d planetocentricToCartesian(const Eigen::Vector3d& lonLatAlt) const;
    Eigen::Vector3d cartesianToPlanetocentric(const Eigen::Vector3d& v) const;

    Eigen::Vector3d eclipticToPlanetocentric(const Eigen::Vector3d& ecl, double tdb) const;

    bool extant(double) const;
    void setLifespan(double, double);
    void getLifespan(double&, double&) const;

    Surface* getAlternateSurface(const std::string&) const;
    void addAlternateSurface(const std::string&, Surface*);
    std::vector<std::string>* getAlternateSurfaceNames() const;

    std::vector<Location*>* getLocations() const;
    void addLocation(Location*);
    Location* findLocation(const std::string&, bool i18n = false) const;
    void computeLocations();

    bool isVisible() const { return visible; }
    void setVisible(bool _visible);
    bool isClickable() const { return clickable; }
    void setClickable(bool _clickable);
    bool isVisibleAsPoint() const { return visibleAsPoint; }
    void setVisibleAsPoint(bool _visibleAsPoint);
    bool isOrbitColorOverridden() const { return overrideOrbitColor; }
    void setOrbitColorOverridden(bool override);
    bool isSecondaryIlluminator() const { return secondaryIlluminator; }
    void setSecondaryIlluminator(bool enable);

    bool hasVisibleGeometry() const { return classification != Invisible && visible; }

    VisibilityPolicy getOrbitVisibility() const { return orbitVisibility; }
    void setOrbitVisibility(VisibilityPolicy _orbitVisibility);

    Color getOrbitColor() const { return orbitColor; }
    void setOrbitColor(const Color&);

    Color getCometTailColor() const { return cometTailColor; }
    void setCometTailColor(const Color& c);

    int getOrbitClassification() const;

    enum
    {
        BodyAxes       =   0x01,
        FrameAxes      =   0x02,
        LongLatGrid    =   0x04,
        SunDirection   =   0x08,
        VelocityVector =   0x10,
    };

    bool referenceMarkVisible(uint32_t) const;
    uint32_t getVisibleReferenceMarks() const;
    void setVisibleReferenceMarks(uint32_t);
    void addReferenceMark(ReferenceMark* refMark);
    void removeReferenceMark(const std::string& tag);
    ReferenceMark* findReferenceMark(const std::string& tag) const;
    const std::list<ReferenceMark*>* getReferenceMarks() const;

    Star* getReferenceStar() const;
    Star* getFrameReferenceStar() const;

    void markChanged();
    void markUpdated();

 private:
//     void setName(const std::string& name);
    void recomputeCullingRadius();

 private:
    // Parent in the name hierarchy
    PlanetarySystem* system;
    // Children in the name hierarchy
    PlanetarySystem* satellites{ nullptr };

    Timeline* timeline{ nullptr };
    // Children in the frame hierarchy
    FrameTree* frameTree{ nullptr };

    float radius{ 1.0f };
    Eigen::Vector3f semiAxes{ Eigen::Vector3f::Ones() };
    float mass{ 0.0f };
    float density{ 0.0f };
    float geomAlbedo{ 0.5f };
    float bondAlbedo{ 0.5f };
    float temperature{ 0.0f };
    float tempDiscrepancy{ 0.0f };

    Eigen::Quaternionf geometryOrientation{ Eigen::Quaternionf::Identity() };

    float cullingRadius{ 0.0f };

    ResourceHandle geometry{ InvalidResource };
    float geometryScale{ 1.0f };
    Surface surface{ Color(1.0f, 1.0f, 1.0f) };

    Atmosphere* atmosphere{ nullptr };
    RingSystem* rings{ nullptr };

    int classification{ Unknown };

    std::string infoURL;

    typedef std::map<std::string, Surface*> AltSurfaceTable;
    AltSurfaceTable *altSurfaces{ nullptr };

    std::vector<Location*>* locations{ nullptr };
    mutable bool locationsComputed{ false };

    std::list<ReferenceMark*>* referenceMarks{ nullptr };

    Color orbitColor;
    Color cometTailColor{ 0.5f, 0.5f, 0.75f };

    bool visible{ true };
    bool clickable{ true };
    bool visibleAsPoint{ true };
    bool overrideOrbitColor{ false };
    VisibilityPolicy orbitVisibility : 3;
    bool secondaryIlluminator{ true };
};

#endif // _CELENGINE_BODY_H_
