// fragmentprog.cpp
//
// Copyright (C) 2003 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include <fstream>
#include <string>
#include "gl.h"
#include "glext.h"
#include "fragmentprog.h"

using namespace std;


class FragmentProcessorNV : public FragmentProcessor
{
 public:
    FragmentProcessorNV();
    virtual ~FragmentProcessorNV();

    virtual void enable();
    virtual void disable();
    virtual void use(unsigned int);
    virtual void parameter(fp::Parameter, float, float, float, float);
    virtual void parameter(fp::Parameter, const float*);
};

class FragmentProcessorARB : public FragmentProcessor
{
 public:
    FragmentProcessorARB();
    virtual ~FragmentProcessorARB();

    virtual void enable();
    virtual void disable();
    virtual void use(unsigned int);
    virtual void parameter(fp::Parameter, float, float, float, float);
    virtual void parameter(fp::Parameter, const float*);
};



static string* ReadTextFromFile(const string& filename)
{
    ifstream textFile(filename.c_str(), ios::in);
    if (!textFile.good())
        return NULL;

    string* s = new string();

    char c;
    while (textFile.get(c))
        *s += c;

    return s;
}


static bool LoadNvFragmentProgram(const string& filename, unsigned int& id)
{
    cout << "Loading NV fragment program: " << filename << '\n';

    string* source = ReadTextFromFile(filename);
    if (source == NULL)
    {
        cout << "Error loading NV fragment program: " << filename << '\n';
        return false;
    }

    glx::glGenProgramsNV(1, (GLuint*) &id);
    glx::glLoadProgramNV(GL_FRAGMENT_PROGRAM_NV,
                         id,
                         source->length(),
                         reinterpret_cast<const GLubyte*>(source->c_str()));

    delete source;

    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        GLint errPos = 0;
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_NV, &errPos);
        cout << "Error in fragment program " << filename <<
            " @ " << errPos << '\n';
        return false;
    }

    return true;
}


static int findLineNumber(const string& s, int index)
{
    if (index >= s.length())
        return -1;

    int lineno = 1;
    for (int i = 0; i < index; i++)
    {
        if (s[i] == '\n')
            lineno++;
    }

    return lineno;
}


FragmentProcessor* fp::initNV()
{
    cout << "Initializing NV fragment programs . . .\n";
#if 0
    if (!LoadNvFragmentProgram("shaders/diffuse.vp", diffuse))
        return NULL;
    if (!LoadNvFragmentProgram("shaders/specular.vp", specular))
        return NULL;
    if (!LoadNvFragmentProgram("shaders/haze.vp", diffuseHaze))
        return NULL;
    if (!LoadNvFragmentProgram("shaders/bumpdiffuse.vp", diffuseBump))
        return NULL;
    if (!LoadNvFragmentProgram("shaders/bumphaze.vp", diffuseBumpHaze))
        return NULL;
    if (!LoadNvFragmentProgram("shaders/shadowtex.vp", shadowTexture))
        return NULL;
    if (!LoadNvFragmentProgram("shaders/diffuse_texoff.vp", diffuseTexOffset))
        return NULL;
    if (!LoadNvFragmentProgram("shaders/rings.vp", ringIllum))
        return NULL;
    if (!LoadNvFragmentProgram("shaders/ringshadow.vp", ringShadow))
        return NULL;
    if (!LoadNvFragmentProgram("shaders/night.vp", nightLights))
        return NULL;
#endif 0
    everything = 0;
    cout << "All NV fragment programs loaded successfully.\n";

    return new FragmentProcessorNV();
}


FragmentProcessor* fp::initARB()
{
    cout << "Initializing ARB fragment programs . . .\n";

    return new FragmentProcessorARB();
}


FragmentProcessor::FragmentProcessor()
{
}

FragmentProcessor::~FragmentProcessor()
{
}

void FragmentProcessor::parameter(fp::Parameter param, const Vec3f& v)
{
    parameter(param, v.x, v.y, v.z, 0.0f);
}
                            
void FragmentProcessor::parameter(fp::Parameter param, const Point3f& p)
{
    parameter(param, p.x, p.y, p.z, 0.0f);
}

void FragmentProcessor::parameter(fp::Parameter param, const Color& c)
{
    parameter(param, c.red(), c.green(), c.blue(), c.alpha());
}



// FragmentProcessorNV implementation

static unsigned int parameterMappingsNV[] = 
{
    0, // SunDirection,
    1, // EyePosition,
    2, // DiffuseColor
    3, // SpecularColor
    4, // SpecularExponent
    5, // AmbientColor
    6, // HazeColor
    7, // TextureTranslation
    8, // Unused
    9, // TexGen_S,
    10, // TexGen_T
};

FragmentProcessorNV::FragmentProcessorNV()
{
}

FragmentProcessorNV::~FragmentProcessorNV()
{
}

void FragmentProcessorNV::enable()
{
    glEnable(GL_FRAGMENT_PROGRAM_NV);
}

void FragmentProcessorNV::disable()
{
    glDisable(GL_FRAGMENT_PROGRAM_NV);
}

void FragmentProcessorNV::use(unsigned int prog)
{
    glx::glBindProgramNV(GL_FRAGMENT_PROGRAM_NV, prog);
}

void FragmentProcessorNV::parameter(fp::Parameter param,
                                    float x, float y, float z, float w)
{
    glx::glProgramParameter4fNV(GL_FRAGMENT_PROGRAM_NV, 
                                parameterMappingsNV[param], x, y, z, w);
}

void FragmentProcessorNV::parameter(fp::Parameter param, const float* fv)
{
    glx::glProgramParameter4fvNV(GL_FRAGMENT_PROGRAM_NV,
                                 parameterMappingsNV[param], fv);
}


// FragmentProcessorARB implementation

FragmentProcessorARB::FragmentProcessorARB()
{
}

FragmentProcessorARB::~FragmentProcessorARB()
{
}

void FragmentProcessorARB::enable()
{
    //glEnable(GL_FRAGMENT_PROGRAM_ARB);
}

void FragmentProcessorARB::disable()
{
    //glDisable(GL_FRAGMENT_PROGRAM_ARB);
}

void FragmentProcessorARB::use(unsigned int prog)
{
    //glx::glBindProgramARB(GL_VERTEX_PROGRAM_ARB, prog);
}

void FragmentProcessorARB::parameter(fp::Parameter param,
                                   float x, float y, float z, float w)
{
    //glx::glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, param, x, y, z, w);
}

void FragmentProcessorARB::parameter(fp::Parameter param, const float* fv)
{
    //glx::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, param, fv);
}
