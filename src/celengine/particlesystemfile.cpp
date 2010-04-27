// particlesystem.cpp
//
// Particle system file loader.
//
// Copyright (C) 2008, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "particlesystemfile.h"
#include "particlesystem.h"
#include "texmanager.h"
#include <sstream>

using namespace Eigen;
using namespace std;


/* !!! IMPORTANT !!!
 * The particle system code is still under development; the complete
 * set of particle system features has not been decided and the cpart
 * format is not even close time final. There are most certainly bugs.
 * DO NOT enable this code and invest a lot of time in creating your
 * own particle system files until development is further along.
 */


ParticleSystemLoader::ParticleSystemLoader(istream& in) :
    m_tokenizer(&in),
    m_parser(&m_tokenizer)
{
}


ParticleSystemLoader::~ParticleSystemLoader()
{
}


ParticleSystem*
ParticleSystemLoader::load()
{
    ParticleSystem* particleSystem = new ParticleSystem();
    
    while (m_tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        string objType;
        
        if (m_tokenizer.getTokenType() != Tokenizer::TokenName)
        {
            raiseError("Error parsing particle system");
            delete particleSystem;
            return NULL;
        }
        
        objType = m_tokenizer.getNameValue();
        if (objType != "Emitter")
        {
            ostringstream stream;
            stream << "Unexpected object '" << objType << "' in particle system file";
            raiseError(stream.str());

            delete particleSystem;
            return NULL;
        }

        Value* objParamsValue = m_parser.readValue();
        if (objParamsValue == NULL || objParamsValue->getType() != Value::HashType)
        {
            raiseError("Error parsing particle system");

            delete particleSystem;
            return NULL;
        }
        
        Hash* objParams = objParamsValue->getHash();
        if (objType == "Emitter")
        {
            ParticleEmitter* emitter = parseEmitter(objParams);
            if (emitter == NULL)
            {
                delete particleSystem;
                return NULL;
            }
            
            particleSystem->addEmitter(emitter);
        }
    }
    
    return particleSystem;
}


VectorGenerator*
ParticleSystemLoader::parseGenerator(Hash* params)
{
    Vector3f constantValue(0.0f, 0.0f, 0.0f);
    if (params->getVector("Constant", constantValue))
    {
        return new ConstantGenerator(constantValue);
    }
    
    Value* generatorValue = NULL;
    generatorValue = params->getValue("Box");
    if (generatorValue != NULL)
    {
        if (generatorValue->getType() != Value::HashType)
        {
            raiseError("Error in Box syntax");
            return NULL;
        }
        
        Hash* boxParams = generatorValue->getHash();
        
        Vector3f center(0.0f, 0.0f, 0.0f);
        Vector3f size(0.0f, 0.0f, 0.0f);
        boxParams->getVector("Center", center);
        boxParams->getVector("Size", size);
        
        return new BoxGenerator(center, size);
    }
    
    generatorValue = params->getValue("Line");
    if (generatorValue != NULL)
    {
        if (generatorValue->getType() != Value::HashType)
        {
            raiseError("Error in Line syntax");
            return NULL;
        }
        
        Hash* lineParams = generatorValue->getHash();
        
        Vector3f p0(0.0f, 0.0f, 0.0f);
        Vector3f p1(0.0f, 0.0f, 0.0f);
        lineParams->getVector("Point1", p0);
        lineParams->getVector("Point2", p1);
        
        return new LineGenerator(p0, p1);
    }
    
    generatorValue = params->getValue("EllipsoidSurface");
    if (generatorValue != NULL)
    {
        if (generatorValue->getType() != Value::HashType)
        {
            raiseError("Error in EllipsoidSurface syntax");
            return NULL;
        }
        
        Hash* ellipsoidSurfaceParams = generatorValue->getHash();
        
        Vector3f center(0.0f, 0.0f, 0.0f);
        Vector3f size(2.0f, 2.0f, 2.0f);
        ellipsoidSurfaceParams->getVector("Center", center);
        ellipsoidSurfaceParams->getVector("Size", size);
        
        return new EllipsoidSurfaceGenerator(center, size * 0.5f);        
    }
    
    generatorValue = params->getValue("Cone");
    if (generatorValue != NULL)
    {
        if (generatorValue->getType() != Value::HashType)
        {
            raiseError("Error in Cone syntax");
            return NULL;
        }
        
        Hash* coneParams = generatorValue->getHash();
        
        double minAngle = 0.0;
        double maxAngle = 0.0;
        double minSpeed = 0.0;
        double maxSpeed = 1.0;
        coneParams->getNumber("MinAngle", minAngle);
        coneParams->getNumber("MaxAngle", maxAngle);
        coneParams->getNumber("MinSpeed", minSpeed);
        coneParams->getNumber("MaxSpeed", maxSpeed);
        
        return new ConeGenerator((float) degToRad(minAngle), (float) degToRad(maxAngle), (float) minSpeed, (float) maxSpeed);                
    }

    generatorValue = params->getValue("GaussianDisc");
    if (generatorValue != NULL)
    {
        if (generatorValue->getType() != Value::HashType)
        {
            raiseError("Error in GaussianDisc syntax");
            return NULL;
        }
        
        Hash* gaussianDiscParams = generatorValue->getHash();
        
        double sigma = 1.0;
        gaussianDiscParams->getNumber("Sigma", sigma);

        return new GaussianDiscGenerator((float) sigma);
    }
    
    raiseError("Missing generator for emitter");
    
    return NULL;
}


ParticleEmitter*
ParticleSystemLoader::parseEmitter(Hash* params)
{
    string textureFileName;
    ResourceHandle textureHandle = InvalidResource;
    if (params->getString("Texture", textureFileName))
    {
        textureHandle = GetTextureManager()->getHandle(TextureInfo(textureFileName, getTexturePath(), TextureInfo::BorderClamp));
    }
    
    double rate = 1.0;
    double lifetime = 1.0;
    params->getNumber("Rate", rate);
    params->getNumber("Lifetime", lifetime);
    
    double startSize = 1.0;
    double endSize = 1.0;
    params->getNumber("StartSize", startSize);
    params->getNumber("EndSize", endSize);
    
    Color startColor(Color::White);
    Color endColor(Color::Black);
    float startOpacity = 0.0f;
    float endOpacity = 0.0f;
    
    params->getColor("StartColor", startColor);
    params->getNumber("StartOpacity", startOpacity);
    params->getColor("EndColor", endColor);
    params->getNumber("EndOpacity", endOpacity);
    
    VectorGenerator* initialPositionGenerator = NULL;
    Value* positionValue = params->getValue("InitialPosition");
    if (positionValue == NULL)
    {
        initialPositionGenerator = new ConstantGenerator(Vector3f::Zero());
    }
    else
    {
        if (positionValue->getType() != Value::HashType)
        {
            raiseError("Error in InitialPosition syntax");
        }
        else
        {
            initialPositionGenerator = parseGenerator(positionValue->getHash());
        }
    }
    
    if (initialPositionGenerator == NULL)
    {
        return NULL;
    }
    
    VectorGenerator* initialVelocityGenerator = NULL;
    Value* velocityValue = params->getValue("InitialVelocity");
    if (velocityValue == NULL)
    {
        initialVelocityGenerator = new ConstantGenerator(Vector3f::Zero());
    }
    else
    {
        if (velocityValue->getType() != Value::HashType)
        {
            raiseError("Error in InitialVelocity syntax");
        }
        else
        {
            initialVelocityGenerator = parseGenerator(velocityValue->getHash());
        }
    }
    
    if (initialVelocityGenerator == NULL)
    {
        delete initialPositionGenerator;
        return NULL;
    }
    
    Vector3f acceleration;
    params->getVector("Acceleration", acceleration);
    
    double startTime = -numeric_limits<double>::infinity();
    double endTime   =  numeric_limits<double>::infinity();
    params->getNumber("Beginning", startTime);
    params->getNumber("Ending", endTime);
    
    double minRotationRate = 0.0;
    double maxRotationRate = 0.0;
    params->getNumber("MinRotationRate", minRotationRate);
    params->getNumber("MaxRotationRate", maxRotationRate);
    
    ParticleEmitter* emitter = new ParticleEmitter();
    emitter->m_texture = textureHandle;
    emitter->m_rate = (float) rate;
    emitter->m_lifetime = (float) lifetime;
    emitter->m_startColor = Color(startColor, startOpacity);
    emitter->m_endColor = Color(endColor, endOpacity);
    emitter->m_startSize = (float) startSize;
    emitter->m_endSize = (float) endSize;
    emitter->m_positionGenerator = initialPositionGenerator;
    emitter->m_velocityGenerator = initialVelocityGenerator;
    emitter->createMaterial();
    
    emitter->setAcceleration(acceleration);
    
    emitter->setLifespan(startTime, endTime);
    emitter->setRotationRateRange((float) degToRad(minRotationRate), (float) degToRad(maxRotationRate));
    
    return emitter;
}


void
ParticleSystemLoader::raiseError(const string& errorMessage)
{
    m_errorMessage = errorMessage;
}


const string&
ParticleSystemLoader::getErrorMessage() const
{
    return m_errorMessage;
}


void
ParticleSystemLoader::setTexturePath(const string& texPath)
{
    m_texPath = texPath;
}


const string&
ParticleSystemLoader::getTexturePath() const
{
    return m_texPath;
}


ParticleSystem* 
LoadParticleSystem(istream& in, const string& texPath)
{
    ParticleSystemLoader* loader = new ParticleSystemLoader(in);
    if (loader == NULL)
        return NULL;

    loader->setTexturePath(texPath);

    ParticleSystem* particleSystem = loader->load();
    if (particleSystem == NULL)
        cerr << "Error in particle system file: " << loader->getErrorMessage() << '\n';

    delete loader;

    return particleSystem;
}
