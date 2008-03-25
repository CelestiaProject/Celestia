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
#include <celutil/util.h>
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
unsigned int vp::multiShadow = 0;
unsigned int vp::everything = 0;
unsigned int vp::diffuseTexOffset = 0;
unsigned int vp::ringIllum = 0;
unsigned int vp::ringShadow = 0;
unsigned int vp::cometTail = 0;
unsigned int vp::nightLights = 0;
unsigned int vp::glossMap = 0;
unsigned int vp::perFragmentSpecular = 0;
unsigned int vp::perFragmentSpecularAlpha = 0;
unsigned int vp::diffuse_2light = 0;
unsigned int vp::diffuseHaze_2light = 0;
unsigned int vp::diffuseTexOffset_2light = 0;
unsigned int vp::specular_2light = 0;
unsigned int vp::nightLights_2light = 0;
unsigned int vp::ellipticalGalaxy = 0;
unsigned int vp::starDisc = 0;
#ifdef HDR_COMPRESS
unsigned int vp::diffuseBumpHDR = 0;
unsigned int vp::diffuseBumpHazeHDR = 0;
unsigned int vp::nightLightsHDR = 0;
unsigned int vp::nightLights_2lightHDR = 0;
#endif


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

    virtual void enableAttribArray(unsigned int);
    virtual void disableAttribArray(unsigned int);
    virtual void attribArray(unsigned int index,
                             int size,
                             GLenum type,
                             unsigned int stride,
                             const void* pointer);
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

    virtual void enableAttribArray(unsigned int);
    virtual void disableAttribArray(unsigned int);
    virtual void attribArray(unsigned int index,
                             int size,
                             GLenum type,
                             unsigned int stride,
                             const void* pointer);
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
    cout << _("Loading NV vertex program: ") << filename << '\n';

    string* source = ReadTextFromFile(filename);
    if (source == NULL)
    {
        cout << _("Error loading NV vertex program: ") << filename << '\n';
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
        cout << _("Error in vertex program ") << filename <<
            " @ " << errPos << '\n';
        return false;
    }

    return true;
}


static int findLineNumber(const string& s, int index)
{
    if (index >= (int)s.length())
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
    cout << _("Loading ARB vertex program: ") << filename << '\n';

    string* source = ReadTextFromFile(filename);
    if (source == NULL)
    {
        cout << _("Error loading ARB vertex program: ") << filename << '\n';
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
        
        cout << _("Error in vertex program ") << filename <<
            _(", line ") << findLineNumber(*source, errPos) << ": " << errMsg << '\n';
    }

    delete source;

    return err == GL_NO_ERROR;
}


// ARB path is preferred; NV vertex program path will eventually go away
VertexProcessor* vp::initNV()
{
    cout << _("Initializing NV vertex programs . . .\n");
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

    // Two light shaders have only been written for the ARB vertex program path
    // Fall back to the the one light versions.
    diffuse_2light = diffuse;
    diffuseHaze_2light = diffuseHaze;
    diffuseTexOffset_2light = diffuseTexOffset;
    specular_2light = specular;

    everything = 0;
    cout << _("All NV vertex programs loaded successfully.\n");

    glx::glTrackMatrixNV(GL_VERTEX_PROGRAM_NV,
                         0, GL_MODELVIEW_PROJECTION_NV, GL_IDENTITY_NV);
    glx::glTrackMatrixNV(GL_VERTEX_PROGRAM_NV,
                         4, GL_MODELVIEW_PROJECTION_NV, GL_INVERSE_TRANSPOSE_NV);

    return new VertexProcessorNV();
}


VertexProcessor* vp::initARB()
{
    cout << _("Initializing ARB vertex programs . . .\n");
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
    if (!LoadARBVertexProgram("shaders/diffuse2_arb.vp", diffuse_2light))
        return NULL;
    if (!LoadARBVertexProgram("shaders/haze2_arb.vp", diffuseHaze_2light))
        return NULL;
    if (!LoadARBVertexProgram("shaders/diffuse_texoff2_arb.vp", diffuseTexOffset_2light))
        return NULL;
    if (!LoadARBVertexProgram("shaders/specular2_arb.vp", specular_2light))
        return NULL;
    if (!LoadARBVertexProgram("shaders/night2_arb.vp", nightLights_2light))
        return NULL;
    if (!LoadARBVertexProgram("shaders/star_arb.vp", starDisc))
        return NULL;
#ifdef HDR_COMPRESS
    if (!LoadARBVertexProgram("shaders/bumpdiffuse_arb_hdr.vp", diffuseBumpHDR))
        return NULL;
    if (!LoadARBVertexProgram("shaders/bumphaze_arb_hdr.vp", diffuseBumpHazeHDR))
        return NULL;
    if (!LoadARBVertexProgram("shaders/night_arb_hdr.vp", nightLightsHDR))
        return NULL;
    if (!LoadARBVertexProgram("shaders/night2_arb_hdr.vp", nightLights_2lightHDR))
        return NULL;
#endif

    // Load vertex programs that are only required with fragment programs
    if (ExtensionSupported("GL_NV_fragment_program") ||
        ExtensionSupported("GL_ARB_fragment_program"))
    {
        if (!LoadARBVertexProgram("shaders/multishadow_arb.vp", multiShadow))
            return NULL;
        if (!LoadARBVertexProgram("shaders/texphong_arb.vp", perFragmentSpecular))
            return NULL;
        if (!LoadARBVertexProgram("shaders/texphong_alpha_arb.vp", perFragmentSpecularAlpha))
            return NULL;
    }

    if (!LoadARBVertexProgram("shaders/ell_galaxy_arb.vp", ellipticalGalaxy))
        return NULL;

    cout << _("All ARB vertex programs loaded successfully.\n");

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
    16, // LightDirection0
    15, // EyePosition
    20, // DiffuseColor0
    34, // SpecularColor0
    40, // SpecularExponent
    32, // AmbientColor
    33, // HazeColor
    41, // TextureTranslation
    90, // Constant0 - relevant for NV_vertex_program only
     0, // Unused
    41, // TexGen_S
    42, // TexGen_T
     0, // TexGen_S2
     0, // TexGen_T2
     0, // TexGen_S3
     0, // TexGen_T3
     0, // TexGen_S4
     0, // TexGen_T4
    50, // LightDirection1
    51, // DiffuseColor1,
    52, // SpecularColor1
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

void VertexProcessorNV::enableAttribArray(unsigned int index)
{
    glEnableClientState(GL_VERTEX_ATTRIB_ARRAY0_NV + index);
}

void VertexProcessorNV::disableAttribArray(unsigned int index)
{
    glDisableClientState(GL_VERTEX_ATTRIB_ARRAY0_NV + index);
}

void VertexProcessorNV::attribArray(unsigned int index,
                                     int size,
                                     GLenum type,
                                     unsigned int stride,
                                     const void* ptr)
{
    glx::glVertexAttribPointerNV(index, size, type, stride, ptr);
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

void VertexProcessorARB::enableAttribArray(unsigned int index)
{
    glx::glEnableVertexAttribArrayARB(index);
}

void VertexProcessorARB::disableAttribArray(unsigned int index)
{
    glx::glDisableVertexAttribArrayARB(index);
}

void VertexProcessorARB::attribArray(unsigned int index,
                                     int size,
                                     GLenum type,
                                     unsigned int stride,
                                     const void* ptr)
{
    glx::glVertexAttribPointerARB(index, size, type, GL_FALSE, stride, ptr);
}

