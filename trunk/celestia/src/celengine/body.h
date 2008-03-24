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

#include <string>
#include <vector>
#include <map>
#include <list>
#include <celutil/utf8.h>
#include <celmath/quaternion.h>
#include <celengine/surface.h>
#include <celengine/atmosphere.h>
#include <celengine/orbit.h>
#include <celengine/star.h>
#include <celengine/location.h>
#include <celengine/rotation.h>
#include <celengine/timeline.h>

class ReferenceFrame;
class Body;
class FrameTree;
class ReferenceMark;

class PlanetarySystem
{
 public:
    PlanetarySystem(Body* _primary);
    PlanetarySystem(Star* _star);
    ~PlanetarySystem();

    Star* getStar() const { return star; };
    Body* getPrimaryBody() const { return primary; };
    int getSystemSize() const { return satellites.size(); };
    Body* getBody(int i) const { return satellites[i]; };
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
    std::vector<std::string> getCompletion(const std::string& _name, bool rec = true) const;

 private:
    typedef std::map<std::string, Body*, UTF8StringOrderingPredicate> ObjectIndex;

 private:
    Star* star;
    Body* primary;
    std::vector<Body*> satellites;
    ObjectIndex objectIndex;  // index of bodies by name
    ObjectIndex i18nObjectIndex;
};


class RingSystem 
{
 public:
    float innerRadius;
    float outerRadius;
    Color color;
    MultiResTexture texture;

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


class Body
{
 public:
     Body(PlanetarySystem*, const std::string& name);
    ~Body();

    enum
    {
        Planet      =    0x01,
        Moon        =    0x02,
        Asteroid    =    0x04,
        Comet       =    0x08,
        Spacecraft  =    0x10,
        Invisible   =    0x20,
        Barycenter  =    0x40,
        SmallBody   =    0x80,
        DwarfPlanet =   0x100,
        Stellar     =   0x200, // only used for orbit mask
        SurfaceFeature = 0x400,
        Component   = 0x800,
        Unknown     = 0x10000,
    };

    enum VisibilityPolicy
    {
        NeverVisible       = 0,
        UseClassVisibility = 1,
        AlwaysVisible      = 2,
    };

    void setDefaultProperties();

    PlanetarySystem* getSystem() const;
    std::string getName(bool i18n = false) const;
    void setName(const std::string);

    void setTimeline(Timeline* timeline);
    const Timeline* getTimeline() const;

    FrameTree* getFrameTree() const;
    FrameTree* getOrCreateFrameTree();

    const ReferenceFrame* getOrbitFrame(double tdb) const;
    const Orbit* getOrbit(double tdb) const;
    const ReferenceFrame* getBodyFrame(double tdb) const;
    const RotationModel* getRotationModel(double tdb) const;
 
    // Size methods
    void setSemiAxes(const Vec3f&);
    Vec3f getSemiAxes() const;
    float getRadius() const;

    bool isSphere() const;
    bool isEllipsoid() const;

    float getMass() const;
    void setMass(float);
    float getAlbedo() const;
    void setAlbedo(float);
    Quatf getOrientation() const;
    void setOrientation(const Quatf&);
    int getClassification() const;
    void setClassification(int);
    std::string getInfoURL() const;
    void setInfoURL(const std::string&);

    PlanetarySystem* getSatellites() const;
    void setSatellites(PlanetarySystem*);

    float getBoundingRadius() const;

    RingSystem* getRings() const;
    void setRings(const RingSystem&);
    const Atmosphere* getAtmosphere() const;
    Atmosphere* getAtmosphere();
    void setAtmosphere(const Atmosphere&);

    void setModel(ResourceHandle);
    ResourceHandle getModel() const;
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
                               const Vec3d& sunPosition,
                               const Vec3d& viewerPosition) const;
    float getApparentMagnitude(float sunLuminosity,
                               const Vec3d& sunPosition,
                               const Vec3d& viewerPosition) const;

    UniversalCoord getPosition(double tdb) const;
    Quatd getOrientation(double tdb) const;
	Vec3d getVelocity(double tdb) const;
	Vec3d getAngularVelocity(double tdb) const;

    Mat4d getLocalToAstrocentric(double) const;
    Point3d getAstrocentricPosition(double) const;
    Quatd getEquatorialToBodyFixed(double) const;
    Quatd getEclipticToFrame(double) const;
    Quatd getEclipticToEquatorial(double) const;
    Quatd getEclipticToBodyFixed(double) const;
    Mat4d getBodyFixedToAstrocentric(double) const;

    Vec3d planetocentricToCartesian(double lon, double lat, double alt) const;
    Vec3d planetocentricToCartesian(const Vec3d& lonLatAlt) const;
    Vec3d cartesianToPlanetocentric(const Vec3d& v) const;

    Vec3d eclipticToPlanetocentric(const Vec3d& ecl, double tdb) const;  

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

    bool isVisible() const { return visible == 1; }
    void setVisible(bool _visible);
    bool isClickable() const { return clickable == 1; }
    void setClickable(bool _clickable);
    bool isVisibleAsPoint() const { return visibleAsPoint == 1; }
    void setVisibleAsPoint(bool _visibleAsPoint);
    bool isOrbitColorOverridden() const { return overrideOrbitColor == 1; }
    void setOrbitColorOverridden(bool override);
    VisibilityPolicy getOrbitVisibility() const { return orbitVisibility; }
    void setOrbitVisibility(VisibilityPolicy _orbitVisibility);

    Color getOrbitColor() const { return orbitColor; }
    void setOrbitColor(const Color&);

    enum
    {
        BodyAxes       =   0x01,
        FrameAxes      =   0x02,
        LongLatGrid    =   0x04,
        SunDirection   =   0x08,
        VelocityVector =   0x10,
    };
	
    bool referenceMarkVisible(uint32) const;
    uint32 getVisibleReferenceMarks() const;
    void setVisibleReferenceMarks(uint32);
    void addReferenceMark(ReferenceMark* refMark);
    void removeReferenceMark(const std::string& tag);
    ReferenceMark* findReferenceMark(const std::string& tag) const;
    const std::list<ReferenceMark*>* getReferenceMarks() const;

    Star* getReferenceStar() const;
    Star* getFrameReferenceStar() const;

    void markChanged();
    void markUpdated();

 private:
    std::string name;
    std::string i18nName;

    // Parent in the name hierarchy
    PlanetarySystem* system;
    // Children in the name hierarchy
    PlanetarySystem* satellites;

    Timeline* timeline;
    // Children in the frame hierarchy
    FrameTree* frameTree;

    float radius;
    Vec3f semiAxes;
    float mass;
    float albedo;
    Quatf orientation;

    ResourceHandle model;
    Surface surface;

    Atmosphere* atmosphere;
    RingSystem* rings;

    int classification;

    std::string infoURL;

    typedef std::map<std::string, Surface*> AltSurfaceTable;
    AltSurfaceTable *altSurfaces;

    std::vector<Location*>* locations;
    mutable bool locationsComputed;

    uint32 refMarks;
    std::list<ReferenceMark*>* referenceMarks;

    Color orbitColor;

    unsigned int visible : 1;
    unsigned int clickable : 1;
    unsigned int visibleAsPoint : 1;
    unsigned int overrideOrbitColor : 1;
    VisibilityPolicy orbitVisibility : 3;
};

#endif // _CELENGINE_BODY_H_
