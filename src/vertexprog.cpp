// vertexprog.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include "gl.h"
#include "glext.h"
#include "vertexprog.h"

using namespace std;


// c[0]..c[3] contains the concatenation of the modelview and projection matrices.
// c[4]..c[7] contains the inverse transpose of the modelview
// c[15] contains the eye position in object space
// c[16] contains the light direction in object space
// c[17] contains H, the normalized sum of the eye and light direction
// c[20] contains the light color
// c[32] contains the ambient light color
// c[33] contains the haze color
// c[40] contains (0, 1, 0, specPower)
// v[OPOS] contains the per-vertex position
// v[NRML] contains the per-vertex normal
// v[TEX0] contains the per-vertex texture coordinate 0
// o[HPOS] output register for homogeneous position
// o[TEX0] output register for texture coordinate 0
// o[COL0] output register for primary color
// R0...R11 temporary registers

static char* simpleVPSource =
      "!!VP1.0\n"
      "DP4   R1.x, c[0], v[OPOS];"
      "DP4   R1.y, c[1], v[OPOS];"
      "DP4   R1.z, c[2], v[OPOS];"
      "DP4   R1.w, c[3], v[OPOS];"

      // Compute the diffuse light component
      "DP3   R2, v[NRML], c[16];"
      // Clamp the diffuse component to zero
      "MAX   R2.x, R2, c[40].xxxx;"

      "ADD   R4, c[15], -v[OPOS];"
      "DP3   R0.w, R4, R4;"
      "RSQ   R0.w, R0.w;"
      "MUL   R4.xyz, R4, R0.w;"
      "DP3   R2.y, v[NRML], R4;"
      "ADD   R2.y, c[40].y, -R2.y;"
      "MUL   R2.y, R2.x, R2.y;"

      "MOV   R6.x, R2.x;"
      "DP3   R6.y, c[17], v[NRML];"
      "MAX   R6.y, R6, c[40].x;"
      "MOV   R6.w, c[40].w;"

      // Output the texture
      "MOV   o[TEX0], v[TEX0];"
      // Output the color
      "MOV   R0, c[32];"
      "MAD   o[COL0], c[20], R2.xxxx, R0;"
      // "MUL   o[COL1], R2.y, c[33];"
      "LIT   R0, R6;"
      "MUL   o[COL1], c[20], R0.zzzz;"
      // Output the vertex
      "MOV   o[HPOS], R1;"

      "END";

#if 0
static char* simpleVPSource =
      "!!VP1.0\n"
      "DP4   R1.x, c[0], v[OPOS];"
      "DP4   R1.y, c[1], v[OPOS];"
      "DP4   R1.z, c[2], v[OPOS];"
      "DP4   R1.w, c[3], v[OPOS];"

      // Compute the diffuse light component
      "DP3   R2, v[NRML], c[16];"
      // Clamp the diffuse component to zero
      "MAX   R2.x, R2, c[40].xxxx;"

      "ADD   R4, c[15], -v[OPOS];"
      "DP3   R0.w, R4, R4;"
      "RSQ   R0.w, R0.w;"
      "MUL   R4.xyz, R4, R0.w;"
      "DP3   R2.y, v[NRML], R4;"
      "ADD   R2.y, c[40].y, -R2.y;"
      "MUL   R2.y, R2.x, R2.y;"

      // Output the texture
      "MOV   o[TEX0], v[TEX0];"
      // Output the color
      "MOV   R0, c[32];"
      "MAD   o[COL0], c[20], R2.xxxx, R0;"
      "MUL   o[COL1], R2.y, c[33];"
      // Output the vertex
      "MOV   o[HPOS], R1;"

      "END";
#endif

#if 0
static char* simpleVPSource =
      "!!VP1.0\n"
      "DP4   R1.x, c[0], v[OPOS];"
      "DP4   R1.y, c[1], v[OPOS];"
      "DP4   R1.z, c[2], v[OPOS];"
      "DP4   R1.w, c[3], v[OPOS];"

      // Compute the diffuse light component
      "DP3   R2, v[NRML], c[16];"
      // Clamp the diffuse component to zero
      "MAX   R2, R2, c[33];"

      // Output the texture
      "MOV   o[TEX0], v[TEX0];"
      // Output the color
      "MOV   R0, c[32];"
      "MAD   o[COL0], c[20], R2, R0;"
      // Output the vertex
      "MOV   o[HPOS], R1;"

      "END";
#endif
#if 0
      // Normalize the transformed normal
      "DP3   R0.w, R1, R1;"
      "RSQ   R0.w, R0.w;"
      "MUL   R1.xyz, R1, R0.w;"
#endif


char *simpleSVPSource = 
    "!!VSP1.0 # Transforms a directional light into object space.\n"
    "DP3   R1.x, c[12], v[0];"
    "DP3   R1.y, c[13], v[0];"
    "DP3   R1.z, c[14], v[0];"
    "MOV   c[4], -R1;"            // Store object space light direction

   // Fetch camera space view vector.
   // Transform into object space.
   "MOV   R2, c[8];"
   "DP3   R3.x, c[12], R2;"
   "DP3   R3.y, c[13], R2;"
   "DP3   R3.z, c[14], R2;"
   "MOV   c[8], -R3;"            // Store object space eye direction

   // Compute H vector in object space.
   "ADD   R4, -R1, -R3;"
   "DP3   R0, R4, R4;"
   "RSQ   R2.x, R0.x;"
   "MUL   R5, R4, R2.x;"
   "MOV   c[5], R5;"            // Store object space H direction

   "END";


unsigned int simpleVP = 0;
unsigned int simpleSVP = 0;


void InitVertexPrograms()
{
    cout << "Initializing vertex programs . . .\n";
    glGenProgramsNV(1, &simpleVP);
    glLoadProgramNV(GL_VERTEX_PROGRAM_NV,
                    simpleVP,
                    strlen(simpleVPSource),
                    (const GLubyte*) simpleVPSource);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        GLint errPos = 0;
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_NV, &errPos);
        cout << "Error in vertex program @ " << errPos << '\n';
    }

    glTrackMatrixNV(GL_VERTEX_PROGRAM_NV,
                    0, GL_MODELVIEW_PROJECTION_NV, GL_IDENTITY_NV);
    glTrackMatrixNV(GL_VERTEX_PROGRAM_NV,
                    4, GL_MODELVIEW_PROJECTION_NV, GL_INVERSE_TRANSPOSE_NV);
}


void DisableVertexPrograms()
{
    glDisable(GL_VERTEX_PROGRAM_NV);
}


void EnableVertexPrograms()
{
    glEnable(GL_VERTEX_PROGRAM_NV);
}


void UseVertexProgram(unsigned int prog)
{
    glBindProgramNV(GL_VERTEX_PROGRAM_NV, prog);
}


void VertexProgramParameter(unsigned int param, const Vec3f& v)
{
    glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, param, v.x, v.y, v.z, 0.0f);
}
                            

void VertexProgramParameter(unsigned int param, const Point3f& p)
{
    glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, param, p.x, p.y, p.z, 0.0f);
}


void VertexProgramParameter(unsigned int param, const Color& c)
{
    glProgramParameter4fNV(GL_VERTEX_PROGRAM_NV, param,
                           c.red(), c.green(), c.blue(), c.alpha());
}
