// vertexprog.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
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
#include "vertexprog.h"

using namespace std;


unsigned int vp::diffuse = 0;
unsigned int vp::specular = 0;
unsigned int vp::diffuseHaze = 0;
unsigned int vp::diffuseBump = 0;
unsigned int vp::diffuseBumpHaze = 0;
unsigned int vp::shadowTexture = 0;
unsigned int vp::everything = 0;
unsigned int vp::diffuseTexOffset = 0;
unsigned int vp::ringIllum = 0;
unsigned int vp::ringShadow = 0;
unsigned int vp::cometTail = 0;
unsigned int vp::nightLights = 0;
unsigned int vp::glossMap = 0;


class VertexProcessorNV : public VertexProcessor
{
 public:
    VertexProcessorNV();
    virtual ~VertexProcessorNV();

    virtual void enable();
    virtual void disable();
    virtual void use(unsigned int);
    virtual void parameter(vp::Parameter, float, float, float, float);
    virtual void parameter(vp::Parameter, const float*);
};

class VertexProcessorARB : public VertexProcessor
{
 public:
    VertexProcessorARB();
    virtual ~VertexProcessorARB();

    virtual void enable();
    virtual void disable();
    virtual void use(unsigned int);
    virtual void parameter(vp::Parameter, float, float, float, float);
    virtual void parameter(vp::Parameter, const float*);
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


static bool LoadNvVertexProgram(const string& filename, unsigned int& id)
{
    cout << "Loading NV vertex program: " << filename << '\n';

    string* source = ReadTextFromFile(filename);
    if (source == NULL)
    {
        cout << "Error loading NV vertex program: " << filename << '\n';
        return false;
    }

    glx::glGenProgramsNV(1, (GLuint*) &id);
    glx::glLoadProgramNV(GL_VERTEX_PROGRAM_NV,
                         id,
                         source->length(),
                         reinterpret_cast<const GLubyte*>(source->c_str()));

    delete source;

    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        GLint errPos = 0;
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_NV, &errPos);
        cout << "Error in vertex program " << filename <<
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


static bool LoadARBVertexProgram(const string& filename, unsigned int& id)
{
    cout << "Loading ARB vertex program: " << filename << '\n';

    string* source = ReadTextFromFile(filename);
    if (source == NULL)
    {
        cout << "Error loading ARB vertex program: " << filename << '\n';
        return false;
    }

    glx::glGenProgramsARB(1, (GLuint*) &id);
    if (glGetError() != GL_NO_ERROR)
    {
        delete source;
        return false;
    }

    glx::glBindProgramARB(GL_VERTEX_PROGRAM_ARB, id);
    glx::glProgramStringARB(GL_VERTEX_PROGRAM_ARB,
                            GL_PROGRAM_FORMAT_ASCII_ARB,
                            source->length(),
                            reinterpret_cast<const GLubyte*>(source->c_str()));

    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        GLint errPos = 0;
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);

        const char* errMsg = (const char*) glGetString(GL_PROGRAM_ERROR_STRING_ARB);
        if (errMsg == NULL)
            errMsg = "Unknown error";
        
        cout << "Error in vertex program " << filename <<
            ", line " << findLineNumber(*source, errPos) << ": " << errMsg << '\n';
    }

    delete source;

    return err == GL_NO_ERROR;
}


VertexProcessor* vp::initNV()
{
    cout << "Initializing NV vertex programs . . .\n";
    if (!LoadNvVertexProgram("shaders/diffuse.vp", diffuse))
        return NULL;
    if (!LoadNvVertexProgram("shaders/specular.vp", specular))
        return NULL;
    if (!LoadNvVertexProgram("shaders/haze.vp", diffuseHaze))
        return NULL;
    if (!LoadNvVertexProgram("shaders/bumpdiffuse.vp", diffuseBump))
        return NULL;
    if (!LoadNvVertexProgram("shaders/bumphaze.vp", diffuseBumpHaze))
        return NULL;
    if (!LoadNvVertexProgram("shaders/shadowtex.vp", shadowTexture))
        return NULL;
    if (!LoadNvVertexProgram("shaders/diffuse_texoff.vp", diffuseTexOffset))
        return NULL;
    if (!LoadNvVertexProgram("shaders/rings.vp", ringIllum))
        return NULL;
    if (!LoadNvVertexProgram("shaders/ringshadow.vp", ringShadow))
        return NULL;
    if (!LoadNvVertexProgram("shaders/night.vp", nightLights))
        return NULL;
    // if (!LoadNvVertexProgram("shaders/comet.vp", cometTail))
    //    return false;
    everything = 0;
    cout << "All NV vertex programs loaded successfully.\n";

    glx::glTrackMatrixNV(GL_VERTEX_PROGRAM_NV,
                         0, GL_MODELVIEW_PROJECTION_NV, GL_IDENTITY_NV);
    glx::glTrackMatrixNV(GL_VERTEX_PROGRAM_NV,
                         4, GL_MODELVIEW_PROJECTION_NV, GL_INVERSE_TRANSPOSE_NV);

    return new VertexProcessorNV();
}


VertexProcessor* vp::initARB()
{
    cout << "Initializing ARB vertex programs . . .\n";
    if (!LoadARBVertexProgram("shaders/diffuse_arb.vp", diffuse))
        return NULL;
    if (!LoadARBVertexProgram("shaders/specular_arb.vp", specular))
        return NULL;
    if (!LoadARBVertexProgram("shaders/haze_arb.vp", diffuseHaze))
        return NULL;
    if (!LoadARBVertexProgram("shaders/bumpdiffuse_arb.vp", diffuseBump))
        return NULL;
    if (!LoadARBVertexProgram("shaders/bumphaze_arb.vp", diffuseBumpHaze))
        return NULL;
    if (!LoadARBVertexProgram("shaders/shadowtex_arb.vp", shadowTexture))
        return NULL;
    if (!LoadARBVertexProgram("shaders/diffuse_texoff_arb.vp", diffuseTexOffset))
        return NULL;
    if (!LoadARBVertexProgram("shaders/rings_arb.vp", ringIllum))
        return NULL;
    if (!LoadARBVertexProgram("shaders/ringshadow_arb.vp", ringShadow))
        return NULL;
    if (!LoadARBVertexProgram("shaders/night_arb.vp", nightLights))
        return NULL;
    if (!LoadARBVertexProgram("shaders/glossmap_arb.vp", glossMap))
        return NULL;
    cout << "All ARB vertex programs loaded successfully.\n";

    return new VertexProcessorARB();
}


void vp::disable()
{
    glDisable(GL_VERTEX_PROGRAM_NV);
}


void vp::enable()
{
    glEnable(GL_VERTEX_PROGRAM_NV);
}


void vp::use(unsigned int prog)
{
    glx::glBindProgramNV(GL_VERTEX_PROGRAM_NV, prog);
}


void vp::parameter(unsigned int param, const Vec3f& v)
{
    glx::glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, param, v.x, v.y, v.z, 0.0f);
}
                            
void vp::parameter(unsigned int param, const Point3f& p)
{
    glx::glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, param, p.x, p.y, p.z, 0.0f);
}

void vp::parameter(unsigned int param, const Color& c)
{
    glx::glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, param,
                                c.red(), c.green(), c.blue(), c.alpha());
}

void vp::parameter(unsigned int param, float x, float y, float z, float w)
{
    glx::glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, param, x, y, z, w);
}



void arbvp::parameter(unsigned int param, const Vec3f& v)
{
    glx::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, param,
                                    v.x, v.y, v.z, 0.0f);
}
                            
void arbvp::parameter(unsigned int param, const Point3f& p)
{
    glx::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, param,
                                    p.x, p.y, p.z, 0.0f);
}

void arbvp::parameter(unsigned int param, const Color& c)
{
    glx::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, param,
                                   c.red(), c.green(), c.blue(), c.alpha());
}

void arbvp::parameter(unsigned int param, float x, float y, float z, float w)
{
    glx::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, param, x, y, z, w);
}

void arbvp::parameter(unsigned int param, const float* fv)
{
    glx::glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, param, fv);
}


VertexProcessor::VertexProcessor()
{
}

VertexProcessor::~VertexProcessor()
{
}

void VertexProcessor::parameter(vp::Parameter param, const Vec3f& v)
{
    parameter(param, v.x, v.y, v.z, 0.0f);
}
                            
void VertexProcessor::parameter(vp::Parameter param, const Point3f& p)
{
    parameter(param, p.x, p.y, p.z, 0.0f);
}

void VertexProcessor::parameter(vp::Parameter param, const Color& c)
{
    parameter(param, c.red(), c.green(), c.blue(), c.alpha());
}



// VertexProcessorNV implementation

static unsigned int parameterMappings[] = 
{
    16, // SunDirection,
    15, // EyePosition,
    20, // DiffuseColor
    34, // SpecularColor
    40, // SpecularExponent
    32, // AmbientColor
    33, // HazeColor
    41, // TextureTranslation
    90, // Constant0 - relevant for NV_vertex_program only
     0, // Unused
    41, // TexGen_S,
    42, // TexGen_Y
};

VertexProcessorNV::VertexProcessorNV()
{
}

VertexProcessorNV::~VertexProcessorNV()
{
}

void VertexProcessorNV::enable()
{
    glEnable(GL_VERTEX_PROGRAM_NV);
}

void VertexProcessorNV::disable()
{
    glDisable(GL_VERTEX_PROGRAM_NV);
}

void VertexProcessorNV::use(unsigned int prog)
{
    glx::glBindProgramNV(GL_VERTEX_PROGRAM_NV, prog);
}

void VertexProcessorNV::parameter(vp::Parameter param,
                                  float x, float y, float z, float w)
{
    glx::glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, 
                                parameterMappings[param], x, y, z, w);
}

void VertexProcessorNV::parameter(vp::Parameter param, const float* fv)
{
    glx::glProgramParameter4fvNV(GL_VERTEX_PROGRAM_NV,
                                 parameterMappings[param], fv);
}



// VertexProcessorARB implementation

VertexProcessorARB::VertexProcessorARB()
{
}

VertexProcessorARB::~VertexProcessorARB()
{
}

void VertexProcessorARB::enable()
{
    glEnable(GL_VERTEX_PROGRAM_ARB);
}

void VertexProcessorARB::disable()
{
    glDisable(GL_VERTEX_PROGRAM_ARB);
}

void VertexProcessorARB::use(unsigned int prog)
{
    glx::glBindProgramARB(GL_VERTEX_PROGRAM_ARB, prog);
}

void VertexProcessorARB::parameter(vp::Parameter param,
                                   float x, float y, float z, float w)
{
    glx::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, param, x, y, z, w);
}

void VertexProcessorARB::parameter(vp::Parameter param, const float* fv)
{
    glx::glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, param, fv);
}
