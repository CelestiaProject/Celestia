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


static bool LoadVertexProgram(const string& filename, unsigned int& id)
{
    cout << "Loading vertex program: " << filename << '\n';

    string* source = ReadTextFromFile(filename);
    if (source == NULL)
    {
        cout << "Error loading vertex program: " << filename << '\n';
        return false;
    }

    EXTglGenProgramsNV(1, (GLuint*) &id);
    EXTglLoadProgramNV(GL_VERTEX_PROGRAM_NV,
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


bool vp::init()
{
    cout << "Initializing vertex programs . . .\n";
    if (!LoadVertexProgram("shaders/diffuse.vp", diffuse))
        return false;
    if (!LoadVertexProgram("shaders/specular.vp", specular))
        return false;
    if (!LoadVertexProgram("shaders/haze.vp", diffuseHaze))
        return false;
    if (!LoadVertexProgram("shaders/bumpdiffuse.vp", diffuseBump))
        return false;
    if (!LoadVertexProgram("shaders/bumphaze.vp", diffuseBumpHaze))
        return false;
    if (!LoadVertexProgram("shaders/shadowtex.vp", shadowTexture))
        return false;
    if (!LoadVertexProgram("shaders/diffuse_texoff.vp", diffuseTexOffset))
        return false;
    if (!LoadVertexProgram("shaders/rings.vp", ringIllum))
        return false;
    if (!LoadVertexProgram("shaders/ringshadow.vp", ringShadow))
        return false;
    everything = 0;

    EXTglTrackMatrixNV(GL_VERTEX_PROGRAM_NV,
                    0, GL_MODELVIEW_PROJECTION_NV, GL_IDENTITY_NV);
    EXTglTrackMatrixNV(GL_VERTEX_PROGRAM_NV,
                    4, GL_MODELVIEW_PROJECTION_NV, GL_INVERSE_TRANSPOSE_NV);

    return true;
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
    EXTglBindProgramNV(GL_VERTEX_PROGRAM_NV, prog);
}


void vp::parameter(unsigned int param, const Vec3f& v)
{
    EXTglProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, param, v.x, v.y, v.z, 0.0f);
}
                            

void vp::parameter(unsigned int param, const Point3f& p)
{
    EXTglProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, param, p.x, p.y, p.z, 0.0f);
}


void vp::parameter(unsigned int param, const Color& c)
{
    EXTglProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, param,
                           c.red(), c.green(), c.blue(), c.alpha());
}


void vp::parameter(unsigned int param, float x, float y, float z, float w)
{
    EXTglProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, param, x, y, z, w);
}
