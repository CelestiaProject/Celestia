// particlesystem.h
//
// Copyright (C) 2008, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_PARTICLESYSTEM_H_
#define _CELENGINE_PARTICLESYSTEM_H_

#include <string>
#include <list>
#include "model.h"


class VectorGenerator;



struct ParticleVertex
{
    void set(const Vec3f& _position, const Vec2f& _texCoord, const unsigned char* _color)
    {
        position = _position;
        texCoord = _texCoord;
        color[Color::Red]   = _color[Color::Red];
        color[Color::Green] = _color[Color::Green];
        color[Color::Blue]  = _color[Color::Blue];
        color[Color::Alpha] = _color[Color::Alpha];
    }
    
    Vec3f position;
    Vec2f texCoord;
    unsigned char color[4];
};


class ParticleEmitter
{
public:
    ParticleEmitter();
    ~ParticleEmitter();
    
    void render(double tsec, RenderContext& rc, ParticleVertex* particleBuffer, unsigned int bufferCapacity) const;
    
    void setAcceleration(const Vec3f& acceleration);
    void createMaterial();
    
    void setLifespan(double startTime, double endTime);
    void setRotationRateRange(float minRate, float maxRate);
    void setBlendMode(Mesh::BlendMode blendMode);
    
private:
    double m_startTime;
    double m_endTime;

public:
    ResourceHandle m_texture;
    
    float m_rate;
    float m_lifetime;
    
    Color m_startColor;
    float m_startSize;
    
    Color m_endColor;
    float m_endSize;
    
    VectorGenerator* m_positionGenerator;
    VectorGenerator* m_velocityGenerator;
    
private:
    Vec3f m_acceleration;
    bool m_nonZeroAcceleration;
    
    float m_minRotationRate;
    float m_rotationRateVariance;
    bool m_rotationEnabled;
    
    Mesh::BlendMode m_blendMode;
    
    Mesh::Material m_material;    
};


class ParticleSystem : public Geometry
{
 public:
    ParticleSystem();
    virtual ~ParticleSystem();
    
    virtual void render(RenderContext& rc, double t = 0.0);
    virtual bool pick(const Ray3d& r, double& distance) const;
    virtual bool isOpaque() const;
    
    void addEmitter(ParticleEmitter* emitter);
        
 public:
    std::list<ParticleEmitter*> m_emitterList;

    Mesh::VertexDescription* m_vertexDesc;
    ParticleVertex* m_vertexData;
    unsigned int m_particleCapacity;
    unsigned int m_particleCount;    
};


class LCGRandomGenerator;

/*! Generator abstract base class.
 *  Subclasses must implement generate() method.
 */
class VectorGenerator
{
public:
    VectorGenerator() {};
    virtual ~VectorGenerator() {};
    virtual Vec3f generate(LCGRandomGenerator& gen) const = 0;
};


/*! Simplest generator; produces the exact same value on each call
 *  to generate().
 */
class ConstantGenerator : public VectorGenerator
{
public:
    ConstantGenerator(const Vec3f& value) : m_value(value) {}
    
    virtual Vec3f generate(LCGRandomGenerator& gen) const;
    
private:
    Vec3f m_value;
};


/*! Generates values uniformly distributed within an axis-aligned box.
 */
class BoxGenerator : public VectorGenerator
{
public:
    BoxGenerator(const Vec3f& center, const Vec3f& axes) :
        m_center(center),
        m_semiAxes(axes * 0.5f)
    {
    }
    
    virtual Vec3f generate(LCGRandomGenerator& gen) const;
    
private:
    Vec3f m_center;
    Vec3f m_semiAxes;
};


/*! Generates values uniformly distributed on a line between
 *  two points.
 */
class LineGenerator : public VectorGenerator
{
public:
    LineGenerator(const Vec3f& p0, const Vec3f& p1) :
        m_origin(p0),
        m_direction(p1 - p0)
    {
    }
    
    virtual Vec3f generate(LCGRandomGenerator& gen) const;
    
private:
    Vec3f m_origin;
    Vec3f m_direction;
};


/*! Generates values uniformly distributed on the surface
 *  of an ellipsoid.
 */
class EllipsoidSurfaceGenerator : public VectorGenerator
{
public:
    EllipsoidSurfaceGenerator(const Vec3f& center, const Vec3f& semiAxes) :
        m_center(center),
        m_semiAxes(semiAxes)
    {
    }
    
    virtual Vec3f generate(LCGRandomGenerator& gen) const;
    
private:
    Vec3f m_center;
    Vec3f m_semiAxes;
};


/*! Generates values uniformly distributed within a spherical
 *  section. The section is centered on the z-axis.
 */
class ConeGenerator : public VectorGenerator
{
public:
    ConeGenerator(float minAngle, float maxAngle, float minLength, const float maxLength) :
        m_cosMinAngle(1.0f - std::cos(minAngle)),
        m_cosAngleVariance(std::cos(minAngle) - std::cos(maxAngle)),
        m_minLength(minLength),
        m_lengthVariance(maxLength - minLength)
    {
    }
    
    virtual Vec3f generate(LCGRandomGenerator& gen) const;
    
private:
    float m_cosMinAngle;
    float m_cosAngleVariance;
    float m_minLength;
    float m_lengthVariance;
};


/*! Generates points in a 2D gaussian distribution in
 *  the xy-plane and centered on the origin.
 */
class GaussianDiscGenerator : public VectorGenerator
{
public:
    GaussianDiscGenerator(float sigma) :
        m_sigma(sigma)
    {
    }

    virtual Vec3f generate(LCGRandomGenerator& gen) const;

private:
    float m_sigma;
};

//ParticleSystem* LoadParticleSystem(const std::string& filename, const std::string& resourcePath);

#endif // _CELENGINE_PARTICLESYSTEM_H_
