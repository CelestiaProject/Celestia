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
#include <celutil/utf8.h>
#include <celmath/quaternion.h>
#include <celengine/surface.h>
#include <celengine/atmosphere.h>
#include <celengine/orbit.h>
#include <celengine/star.h>
#include <celengine/location.h>
#include <celengine/rotation.h>

class ReferenceFrame;
class Body;

class PlanetarySystem
{
 public:
    PlanetarySystem(Body* _primary);
    PlanetarySystem(Star* _star);
    
    Star* getStar() const { return star; };
    Body* getPrimaryBody() const { return primary; };
    int getSystemSize() const { return satellites.size(); };
    Body* getBody(int i) const { return satellites[i]; };
    void addBody(Body* body);
    void removeBody(Body* body);
    void replaceBody(Body* oldBody, Body* newBody);

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
        innerRadius(inner), outerRadius(outer), color(1.0f, 1.0f, 1.0f), texture()
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
    Body(PlanetarySystem*);
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
        Unknown     = 0x10000,
    };

    PlanetarySystem* getSystem() const;
    std::string getName(bool i18n = false) const;
    void setName(const std::string);
    Orbit* getOrbit() const;
    void setOrbit(Orbit*);
    const Body* getOrbitBarycenter() const;
    void setOrbitBarycenter(const Body*);

    const ReferenceFrame* getOrbitFrame() const;
    void setOrbitFrame(const ReferenceFrame* f);
    const ReferenceFrame* getBodyFrame() const;
    void setBodyFrame(const ReferenceFrame* f);

    const RotationModel* getRotationModel() const;
    void setRotationModel(const RotationModel*);

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

    /*! Get the apparent magnitude of the body, neglecting the phase (as if
     *  the body was at opposition.
     */
    float getApparentMagnitude(const Star& sun,
                               float distanceFromSun,
                               float distanceFromViewer) const;

    /*! Get the apparent magnitude of the body, neglecting the phase (as if
     *  the body was at opposition.
     */
    float getApparentMagnitude(float sunLuminosity,
                               float distanceFromSun,
                               float distanceFromViewer) const;

    /*! Get the apparent magnitude of the body, corrected for its phase.
     */
    float getApparentMagnitude(const Star& sun,
                               const Vec3d& sunPosition,
                               const Vec3d& viewerPosition) const;

    /*! Get the apparent magnitude of the body, corrected for the phase.
     */
    float getApparentMagnitude(float sunLuminosity,
                               const Vec3d& sunPosition,
                               const Vec3d& viewerPosition) const;

    /*! Get the transformation which converts body coordinates into
     *  heliocentric coordinates. Some clarification on the meaning
     *  of 'heliocentric': the position of every solar system body
     *  is ultimately defined with respect to some star or star
     *  system barycenter. Currently, this star is the root of the
     *  name hierarchy containing the body. In future (post-1.5.0)
     *  versions of Celestia, this will be changed so that the
     *  reference star is the root of the frame hierarchy.
     */
    Mat4d getLocalToHeliocentric(double) const;
    Point3d getHeliocentricPosition(double) const;
    Quatd getEquatorialToBodyFixed(double) const;
    Quatd getEclipticalToFrame(double) const;
    Quatd getEclipticalToEquatorial(double) const;
    Quatd getEclipticalToBodyFixed(double) const;
    Mat4d getBodyFixedToHeliocentric(double) const;

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
    
    bool isClickable() const { return clickable == 1; }
    void setClickable(bool _clickable);
    bool isVisibleAsPoint() const { return visibleAsPoint == 1; }
    void setVisibleAsPoint(bool _visibleAsPoint);

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

    Star* getReferenceStar() const;
    Star* getFrameReferenceStar() const;

 private:
    std::string name;
    std::string i18nName;

    PlanetarySystem* system;
    
    Orbit* orbit;
    const Body* orbitBarycenter;
    const ReferenceFrame* orbitFrame;
    const ReferenceFrame* bodyFrame;
    
    const RotationModel* rotationModel;

    float radius;
    Vec3f semiAxes;
    float mass;
    float albedo;
    Quatf orientation;

    double protos;
    double eschatos;

    ResourceHandle model;
    Surface surface;

    Atmosphere* atmosphere;
    RingSystem* rings;

    PlanetarySystem* satellites;

    int classification;

    std::string infoURL;

    typedef std::map<std::string, Surface*> AltSurfaceTable;
    AltSurfaceTable *altSurfaces;

    std::vector<Location*>* locations;
    mutable bool locationsComputed;

    uint32 referenceMarks;
    unsigned int clickable : 1;
    unsigned int visibleAsPoint : 1;

    // Only necessary until we switch to using frame hierarchy
    Star* frameRefStar;
};

#endif // _CELENGINE_BODY_H_
