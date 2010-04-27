// particlesystem.cpp
//
// Stateless particle system renderer.
//
// Copyright (C) 2008, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "celutil/util.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <limits>
#include "modelgeometry.h"
#include "particlesystem.h"
#include <GL/glew.h>
#include "vecgl.h"
#include "rendcontext.h"
#include "texmanager.h"

#include <celutil/basictypes.h>

using namespace cmod;
using namespace Eigen;
using namespace std;

/* !!! IMPORTANT !!!
 * The particle system code is still under development; the complete
 * set of particle system features has not been decided and the cpart
 * format is not even close time final. There are most certainly bugs.
 * DO NOT enable this code and invest a lot of time in creating your
 * own particle system files until development is further along.
 */

/*  STATELESS PARTICLE SYSTEMS
 *
 *  THEORY
 *  In a typical particle system, initial particle states are generated
 *  and stored in array. At each time step, particles have their states
 *  updated and are then drawn. A typical sequence is:
 *    - Compute the forces acting on the particles
 *    - Update particle positions and velocities
 *    - Age the particles (updating state such as color and size)
 *    - Render the particles
 *
 *  This process is well-suited to a simulation where the time steps are
 *  relatively uniform. But, we cannot rely on a uniform time step in Celestia.
 *  The user may skip ahead instantly to times in the distance past or future,
 *  change the time rate to over a billion times normal, or reverse time. 
 *  Numerical integration of particle positions is completely impractical.
 *
 *  Instead, Celestia uses 'stateless' particle systems. From the particle
 *  system description, the particle positions and appearances can be generated
 *  for any time. Initial states are generated from a pseudorandom sequence
 *  and state at the current time is computed analytically from those initial
 *  values. The fact that motions must be calculated analytically means that
 *  only very simply force models may be used; still, a large variety of
 *  effects are still practical.
 *
 *  A particle system is just a list of particle emitters. Each emitter has
 *  a fixed emission rate (particles per second), start time, and end time.
 *  There are few properties that apply to all particles produced by an emitter:
 *     texture, lifetime, start/end color, and start/end size
 *  Color and size are linearly interpolated between start and end values over
 *  the lifetime of a particle.
 *  An emitter has two different 'generators': one produces initial particle
 *  velocities, the other initial positions.
 *
 *  Emitter generators are fed with values from a linear congruential
 *  generator. Other pseudorandom number generators can produce sequences with
 *  better distributions and can have better performance at generating values.
 *  However, seeding these other generators is very slow, and seeding must
 *  be every time a particle is to be drawn. The well-known defects in
 *  pseudorandom sequences produced by an LCG are not visible in a particle
 *  system (and lack of apparent visual artifacts is the *only* requirement here.)
 */

// Same values as rand48()
static const uint64 A = ((uint64) 0x5deece66ul << 4) | 0xd;
static const uint64 C = 0xb;
static const uint64 M = ((uint64) 1 << 48) - 1;

/*! Linear congruential random number generator that emulates
 *  rand48()
 */
class LCGRandomGenerator
{
public:
    LCGRandomGenerator() :
        previous(0)
    {
    }
    
    LCGRandomGenerator(uint64 seed) :
        previous(seed)
    {
    }
    
    uint64 randUint64()
    {
        previous = (A * previous + C) & M;
        return previous;
    }
    
    /*! Return a random integer between -2^31 and 2^31 - 1
     */
    int32 randInt32()
    {
        return (int32) (randUint64() >> 16);
    }
    
    /*! Return a random integer between 0 and 2^32 - 1                           
     */
    uint32 randUint32()
    {
        return (uint32) (randUint64() >> 16);
    }

    /*! Generate a random floating point value in [ 0, 1 )
     *  This function directly manipulates the bits of a floating
     *  point number, and will not work properly on a system that
     *  doesn't use IEEE754 floats.
     */
    float randFloat()
    {
        uint32 randBits = randInt32();
        randBits = (randBits & 0x007fffff) | 0x3f800000;
        return *reinterpret_cast<float*>(&randBits) - 1.0f;
    }
    
    /*! Generate a random floating point value in [ -1, 1 )
     *  This function directly manipulates the bits of a floating
     *  point number, and will not work properly on a system that
     *  doesn't use IEEE754 floats.
     */
    float randSfloat()
    {
        uint32 randBits = (uint32) (randUint64() >> 16);
        randBits = (randBits & 0x007fffff) | 0x40000000;
        return *reinterpret_cast<float*>(&randBits) - 3.0f;
    }
    
private:
    uint64 previous;
};


/**** Generator implementations ****/

Vector3f
ConstantGenerator::generate(LCGRandomGenerator& /* gen */) const
{
    return m_value;
}


Vector3f
BoxGenerator::generate(LCGRandomGenerator& gen) const
{
    return Vector3f(gen.randSfloat() * m_semiAxes.x(),
                    gen.randSfloat() * m_semiAxes.y(),
                    gen.randSfloat() * m_semiAxes.z()) + m_center;
}


Vector3f
LineGenerator::generate(LCGRandomGenerator& gen) const
{
    return m_origin + m_direction * gen.randFloat();
}


Vector3f
EllipsoidSurfaceGenerator::generate(LCGRandomGenerator& gen) const
{
    float theta = (float) PI * gen.randSfloat();
    float cosPhi = gen.randSfloat();
    float sinPhi = std::sqrt(1.0f - cosPhi * cosPhi);
    if (cosPhi < 0.0f)
        sinPhi = -sinPhi;
    
    float s = std::sin(theta);
    float c = std::cos(theta);
    return Vector3f(sinPhi * c * m_semiAxes.x(), sinPhi * s * m_semiAxes.y(), cosPhi * m_semiAxes.z()) + m_center;
}


Vector3f
ConeGenerator::generate(LCGRandomGenerator& gen) const
{
    float theta = (float) PI * gen.randSfloat();
    float cosPhi = 1.0f - m_cosMinAngle - gen.randFloat() * m_cosAngleVariance;
    float sinPhi = std::sqrt(1.0f - cosPhi * cosPhi);
    if (cosPhi < 0.0f)
        sinPhi = -sinPhi;
    
    float s = std::sin(theta);
    float c = std::cos(theta);
    return Vector3f(sinPhi * c, sinPhi * s, cosPhi) * (m_minLength + gen.randFloat() * m_lengthVariance);
}    



Vector3f
GaussianDiscGenerator::generate(LCGRandomGenerator& gen) const
{
    float r1 = 0.0f;
    float r2 = 0.0f;
    float s = 0.0f;

    do
    {
        r1 = gen.randSfloat();
        r2 = gen.randSfloat();
        s = r1 * r1 + r2 * r2;
    } while (s > 1.0f);

    // Choose angle uniformly distributed in [ 0, 2*PI ), radius
    // with a Gaussian distribution. Use the polar form of the
    // Box-Muller transform to produce a normally distributed
    // random number.
    float r = r1 * std::sqrt(-2.0f * std::log(s) / s) * m_sigma;
    float theta = r2 * 2.0f * (float) PI;
    return Vector3f(r * std::cos(theta), r * std::sin(theta), 0.0f);
}



ParticleEmitter::ParticleEmitter() :
    m_startTime(-numeric_limits<double>::infinity()),
    m_endTime(-numeric_limits<double>::infinity()),
    m_texture(InvalidResource),
    m_rate(1.0f),
    m_lifetime(1.0f),
    m_startColor(1.0f, 1.0f, 1.0f, 0.0f),
    m_startSize(1.0f),
    m_endColor(1.0f, 1.0f, 1.0f, 0.0f),
    m_endSize(1.0f),
    m_positionGenerator(NULL),
    m_velocityGenerator(NULL),
    m_acceleration(0.0f, 0.0f, 0.0f),
    m_nonZeroAcceleration(false),
    m_minRotationRate(0.0f),
    m_rotationRateVariance(0.0f),
    m_rotationEnabled(false),
    m_blendMode(cmod::Material::PremultipliedAlphaBlend)
{
}


ParticleEmitter::~ParticleEmitter()
{
    delete m_positionGenerator;
    delete m_velocityGenerator;
}


void
ParticleEmitter::setLifespan(double startTime, double endTime)
{
    m_startTime = startTime;
    m_endTime = endTime;
}


void
ParticleEmitter::setRotationRateRange(float minRate, float maxRate)
{
    m_rotationEnabled = minRate != 0.0f || maxRate != 0.0f;
    m_minRotationRate = minRate;
    m_rotationRateVariance = maxRate - minRate;
}


void
ParticleEmitter::setAcceleration(const Vector3f& acceleration)
{
    m_acceleration = acceleration;
    m_nonZeroAcceleration = m_acceleration != Vector3f::Zero();
}


void
ParticleEmitter::setBlendMode(cmod::Material::BlendMode blendMode)
{
    m_blendMode = blendMode;
}


static const uint64 scrambleMask = (uint64(0xcccccccc) << 32) | 0xcccccccc; 

void
ParticleEmitter::render(double tsec,
                        RenderContext& rc,
                        ParticleVertex* particleBuffer,
                        unsigned int particleBufferCapacity) const
{
    double t = tsec;
    bool startBounded = m_startTime > -numeric_limits<double>::infinity();
    bool endBounded = m_endTime < numeric_limits<double>::infinity();

    // Return immediately if we're far enough past the end time that no
    // particles remain.
    if (endBounded)
    {
        if (t > m_endTime + m_lifetime)
            return;
    }
    
    // If a start time is specified, set t to be relative to the start time.
    // Return immediately if we haven't reached the start time yet.
    if (startBounded)
    {
        t -= m_startTime;
        if (t < 0.0)
            return;
    }
    
    Matrix3f modelViewMatrix = rc.getCameraOrientation().conjugate().toRotationMatrix();

    rc.setMaterial(&m_material);

    Vector3f v0 = modelViewMatrix * Vector3f(-1.0f, -1.0f, 0.0f);
    Vector3f v1 = modelViewMatrix * Vector3f( 1.0f, -1.0f, 0.0f);
    Vector3f v2 = modelViewMatrix * Vector3f( 1.0f,  1.0f, 0.0f);
    Vector3f v3 = modelViewMatrix * Vector3f(-1.0f,  1.0f, 0.0f);

    glDisable(GL_LIGHTING);
    
    Texture* texture = NULL;
    if (m_texture != InvalidResource)
    {
        texture = GetTextureManager()->find(m_texture);
    }
    
    if (texture != NULL)
    {
        glEnable(GL_TEXTURE_2D);
        texture->bind();
    }
    else
    {
        glDisable(GL_TEXTURE_2D);
    }
    
    // Use premultiplied alpha
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    glDepthMask(GL_FALSE);    
    
    double emissionInterval = 1.0 / m_rate;
    double dserial = std::fmod(t * m_rate, (double) (1 << 31));
    int serial = (int) (dserial);
    double age = (dserial - serial) * emissionInterval;
    float invLifetime = (float) (1.0 / m_lifetime);
    
    double maxAge = m_lifetime;
    if (startBounded)
    {
        maxAge = std::min((double) m_lifetime, t);
    }
    
    if (endBounded && tsec > m_endTime)
    {
        int skipParticles = (int) ((tsec - m_endTime) * m_rate);
        serial -= skipParticles;
        age += skipParticles * emissionInterval;
    }
    
    Vec4f startColor(m_startColor.red(), m_startColor.green(), m_startColor.blue(), m_startColor.alpha());
    Vec4f endColor(m_endColor.red(), m_endColor.green(), m_endColor.blue(), m_endColor.alpha());
    
    unsigned int particleCount = 0;
    
    while (age < maxAge)
    {
        // When the particle buffer is full, render the particles and flush it
        if (particleCount == particleBufferCapacity)
        {
            glDrawArrays(GL_QUADS, 0, particleCount * 4);            
            particleCount = 0;
        }
        
        float alpha = (float) age * invLifetime;
        float beta = 1.0f - alpha;
        float size = alpha * m_endSize + beta * m_startSize;
        
        // Scramble the random number generator seed so that we don't end up with
        // artifacts from using regularly incrementing values.
        //
        // TODO: consider whether the generator could be seeded just once before
        // the first particle is drawn. This would entail further restrictions,
        // such as no 'branching' (variable number of calls to LCG::generate()) in
        // particle state calculation.
        LCGRandomGenerator gen(uint64(serial) * uint64(0x128ef719) ^ scrambleMask);

        // Calculate the color of the particle
        // TODO: switch to using a lookup table for color and opacity
        unsigned char color[4];
        color[Color::Red]   = (unsigned char) ((alpha * endColor.x + beta * startColor.x) * 255.99f);
        color[Color::Green] = (unsigned char) ((alpha * endColor.y + beta * startColor.y) * 255.99f);
        color[Color::Blue]  = (unsigned char) ((alpha * endColor.z + beta * startColor.z) * 255.99f);
        color[Color::Alpha] = (unsigned char) ((alpha * endColor.w + beta * startColor.w) * 255.99f);
        
        Vector3f v = m_velocityGenerator->generate(gen);
        Vector3f center = m_positionGenerator->generate(gen) + v * (float) age;
        if (m_nonZeroAcceleration)
            center += m_acceleration * (float) (age * age);
        
        if (!m_rotationEnabled)
        {
            particleBuffer[particleCount * 4 + 0].set(center + v0 * size, Vector2f(0.0f, 1.0f), color);
            particleBuffer[particleCount * 4 + 1].set(center + v1 * size, Vector2f(1.0f, 1.0f), color);
            particleBuffer[particleCount * 4 + 2].set(center + v2 * size, Vector2f(1.0f, 0.0f), color);
            particleBuffer[particleCount * 4 + 3].set(center + v3 * size, Vector2f(0.0f, 0.0f), color);
        }
        else
        {
            float rotationRate = m_minRotationRate + m_rotationRateVariance * gen.randFloat();
            float rotation = rotationRate * (float) age;
            float c = std::cos(rotation);
            float s = std::sin(rotation);
            
            particleBuffer[particleCount * 4 + 0].set(center + (modelViewMatrix * Vector3f(-c + s, -s - c, 0.0f)) * size, Vector2f(0.0f, 1.0f), color);
            particleBuffer[particleCount * 4 + 1].set(center + (modelViewMatrix * Vector3f( c + s,  s - c, 0.0f)) * size, Vector2f(1.0f, 1.0f), color);
            particleBuffer[particleCount * 4 + 2].set(center + (modelViewMatrix * Vector3f( c - s,  s + c, 0.0f)) * size, Vector2f(1.0f, 0.0f), color);
            particleBuffer[particleCount * 4 + 3].set(center + (modelViewMatrix * Vector3f(-c - s, -s + c, 0.0f)) * size, Vector2f(0.0f, 0.0f), color);
        }
        
        ++particleCount;
        
        age += emissionInterval;
        serial--;
    }
    
    // Render any remaining particles in the buffer
    if (particleCount > 0)
    {
        glDrawArrays(GL_QUADS, 0, particleCount * 4);
    }
}


void
ParticleEmitter::createMaterial()
{
    m_material.diffuse = cmod::Material::Color(0.0f, 0.0f, 0.0f);
    m_material.emissive = cmod::Material::Color(1.0f, 1.0f, 1.0f);
    m_material.blend = m_blendMode;
    m_material.opacity = 0.99f;
    m_material.maps[0] = new CelestiaTextureResource(m_texture);
}    


#define STRUCT_OFFSET(s, memberName) ((uint32) (reinterpret_cast<char*>(&(s).memberName) - reinterpret_cast<char*>(&(s))))

ParticleSystem::ParticleSystem() :
    m_vertexDesc(NULL),
    m_vertexData(NULL),
    m_particleCapacity(0),
    m_particleCount(0)
{
    m_particleCapacity = 1000;
    m_vertexData = new ParticleVertex[m_particleCapacity * 4];
    
    // Create the vertex description; currently, it is the same for all
    // particle systems.
    ParticleVertex temp;
    Mesh::VertexAttribute attributes[3];
    attributes[0] = cmod::Mesh::VertexAttribute(Mesh::Position, Mesh::Float3, STRUCT_OFFSET(temp, position));
    attributes[1] = Mesh::VertexAttribute(Mesh::Texture0, Mesh::Float2, STRUCT_OFFSET(temp, texCoord));
    attributes[2] = Mesh::VertexAttribute(Mesh::Color0, Mesh::UByte4, STRUCT_OFFSET(temp, color));
    m_vertexDesc = new Mesh::VertexDescription(sizeof(ParticleVertex), 3, attributes);
}


ParticleSystem::~ParticleSystem()
{
    for (list<ParticleEmitter*>::const_iterator iter = m_emitterList.begin();
         iter != m_emitterList.end(); iter++)
    {
        delete (*iter);
    }    
    
    delete[] m_vertexData;
    delete m_vertexDesc;
}


void
ParticleSystem::render(RenderContext& rc, double tsec)
{
    rc.setVertexArrays(*m_vertexDesc, m_vertexData);
    
    for (list<ParticleEmitter*>::const_iterator iter = m_emitterList.begin();
         iter != m_emitterList.end(); iter++)
    {
        (*iter)->render(tsec, rc, m_vertexData, m_particleCapacity);
    }
        
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
}


bool
ParticleSystem::pick(const Ray3d& /* r */, double& /* distance */) const
{
    // Pick selection for particle systems not supported (because it's
    // not typically desirable.)
    return false;
}


bool
ParticleSystem::isOpaque() const
{
    return false;
}


void
ParticleSystem::addEmitter(ParticleEmitter* emitter)
{
    m_emitterList.push_back(emitter);
}
