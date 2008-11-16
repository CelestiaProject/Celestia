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
#include <celutil/util.h>
#include "gl.h"
#include "glext.h"
#include "fragmentprog.h"

using namespace std;


unsigned int fp::sphereShadowOnRings = 0;
unsigned int fp::eclipseShadow1      = 0;
unsigned int fp::eclipseShadow2      = 0;
unsigned int fp::texDiffuse          = 0;
unsigned int fp::texDiffuseBump      = 0;
unsigned int fp::texSpecular         = 0;
unsigned int fp::texSpecularAlpha    = 0;


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

#if 0
static int findLineNumber(const string& s, unsigned int index)
{
    if (index >= s.length())
        return -1;

    int lineno = 1;
    for (unsigned int i = 0; i < index; i++)
    {
        if (s[i] == '\n')
            lineno++;
    }

    return lineno;
}
#endif


static bool LoadNvFragmentProgram(const string& filename, unsigned int& id)
{
    cout << _("Loading NV fragment program: ") << filename << '\n';

    string* source = ReadTextFromFile(filename);
    if (source == NULL)
    {
        cout << _("Error loading NV fragment program: ") << filename << '\n';
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
        cout << _("Error in fragment program ") << filename << ' ' <<
            glGetString(GL_PROGRAM_ERROR_STRING_NV) << '\n';
        return false;
    }

    return true;
}


FragmentProcessor* fp::initNV()
{
    cout << _("Initializing NV fragment programs . . .\n");
    if (!LoadNvFragmentProgram("shaders/shadow_on_rings_nv.fp", sphereShadowOnRings))
        return NULL;
    if (!LoadNvFragmentProgram("shaders/eclipse1_nv.fp", eclipseShadow1))
        return NULL;
    if (!LoadNvFragmentProgram("shaders/eclipse2_nv.fp", eclipseShadow2))
        return NULL;
    if (!LoadNvFragmentProgram("shaders/diffuse_nv.fp", texDiffuse))
        return NULL;
    if (!LoadNvFragmentProgram("shaders/bumpdiffuse_nv.fp", texDiffuseBump))
        return NULL;
    if (!LoadNvFragmentProgram("shaders/texphong_nv.fp", texSpecular))
        return NULL;
    if (!LoadNvFragmentProgram("shaders/texphong_alpha_nv.fp", texSpecularAlpha))
        return NULL;

    cout << _("All NV fragment programs loaded successfully.\n");

    return new FragmentProcessorNV();
}


FragmentProcessor* fp::initARB()
{
    cout << _("Initializing ARB fragment programs . . .\n");

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
    glx::glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_NV,
                                      param, x, y, z, w);
}

void FragmentProcessorNV::parameter(fp::Parameter param, const float* fv)
{
    glx::glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_NV, param, fv);
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

void FragmentProcessorARB::use(unsigned int /*prog*/)
{
    //glx::glBindProgramARB(GL_VERTEX_PROGRAM_ARB, prog);
}

void FragmentProcessorARB::parameter(fp::Parameter /*param*/,
                                     float /*x*/, float /*y*/,
                                     float /*z*/, float /*w*/)
{
    //glx::glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, param, x, y, z, w);
}

void FragmentProcessorARB::parameter(fp::Parameter /*param*/, const float* /*fv*/)
{
    //glx::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, param, fv);
}
