// body.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _BODY_H_
#define _BODY_H_

#include <string>
#include <vector>
#include <celmath/quaternion.h>
#include <celengine/surface.h>
#include <celengine/atmosphere.h>
#include <celengine/orbit.h>
#include <celengine/star.h>


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

    enum TraversalResult
    {
        ContinueTraversal   = 0,
        StopTraversal       = 1
    };

    typedef bool (*TraversalFunc)(Body*, void*);

    bool traverse(TraversalFunc, void*) const;
    Body* find(std::string, bool deepSearch = false) const;

 private:
    Star* star;
    Body* primary;
    std::vector<Body*> satellites;
};


class RotationElements
{
 public:
    RotationElements();

    float period;        // sidereal rotation period
    float offset;        // rotation at epoch
    double epoch;
    float obliquity;     // tilt of rotation axis
    float axisLongitude; // longitude of rot. axis projected onto orbital plane
    float precessionRate; // rate of precession of rotation axis in rads/day
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
    RingSystem(float inner, float outer, Color _color, const std::string& textureName) :
        innerRadius(inner), outerRadius(outer), color(_color), texture(textureName)
        { };
};


class Body
{
 public:
    Body(PlanetarySystem*);
    ~Body();

    enum
    {
        Planet     = 0x01,
        Moon       = 0x02,
        Asteroid   = 0x04,
        Comet      = 0x08,
        Spacecraft = 0x10,
        Unknown    = 0x10000,
    };

    PlanetarySystem* getSystem() const;
    std::string getName() const;
    void setName(const std::string);
    Orbit* getOrbit() const;
    void setOrbit(Orbit*);
    RotationElements getRotationElements() const;
    void setRotationElements(const RotationElements&);
    float getRadius() const;
    void setRadius(float);
    float getMass() const;
    void setMass(float);
    float getOblateness() const;
    void setOblateness(float);
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

    RingSystem* getRings() const;
    void setRings(const RingSystem&);
    const Atmosphere* getAtmosphere() const;
    Atmosphere* getAtmosphere();
    void setAtmosphere(const Atmosphere&);

    void setMesh(ResourceHandle);
    ResourceHandle getMesh() const;
    void setSurface(const Surface&);
    const Surface& getSurface() const;
    Surface& getSurface();

    float getLuminosity(const Star& sun,
                        float distanceFromSun) const;
    float getApparentMagnitude(const Star& sun,
                               float distanceFromSun,
                               float distanceFromViewer) const;
    float getApparentMagnitude(const Star& sun,
                               const Vec3d& sunPosition,
                               const Vec3d& viewerPosition) const;

    Mat4d getLocalToHeliocentric(double) const;
    Point3d getHeliocentricPosition(double) const;
    Quatd getEquatorialToGeographic(double);
    Quatd getEclipticalToEquatorial(double) const;
    Quatd getEclipticalToGeographic(double);
    Mat4d getGeographicToHeliocentric(double);

 private:
    std::string name;

    PlanetarySystem* system;
    Orbit* orbit;
    RotationElements rotationElements;

    float radius;
    float mass;
    float oblateness;
    float albedo;
    Quatf orientation;

    ResourceHandle mesh;
    Surface surface;

    Atmosphere* atmosphere;
    RingSystem* rings;

    PlanetarySystem* satellites;

    int classification;

    std::string infoURL;
};

#endif // _BODY_H_
