// lodspheremesh.cpp
// 
// Copyright (C) 2000, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cmath>
#include <iostream>
#include "gl.h"
#include "glext.h"
#include "mathlib.h"
#include "vecmath.h"
#include "lodspheremesh.h"

using namespace std;


static bool trigArraysInitialized = false;
static int maxDivisions = 2048;
static int thetaDivisions = maxDivisions;
static int phiDivisions = maxDivisions / 2;
static int minStep = 16;
static float* sinPhi = NULL;
static float* cosPhi = NULL;
static float* sinTheta = NULL;
static float* cosTheta = NULL;


static void InitTrigArrays()
{
    sinTheta = new float[thetaDivisions + 1];
    cosTheta = new float[thetaDivisions + 1];
    sinPhi = new float[phiDivisions + 1];
    cosPhi = new float[phiDivisions + 1];

    int i;
    for (i = 0; i <= thetaDivisions; i++)
    {
        double theta = (double) i / (double) thetaDivisions * 2.0 * PI;
        sinTheta[i] = (float) sin(theta);
        cosTheta[i] = (float) cos(theta);
    }

    for (i = 0; i < phiDivisions; i++)
    {
        double phi = ((double) i / (double) phiDivisions - 0.5) * PI;
        sinPhi[i] = (float) sin(phi);
        cosPhi[i] = (float) cos(phi);
    }

    trigArraysInitialized = true;
}


LODSphereMesh::LODSphereMesh() :
    vertices(NULL),
    normals(NULL),
    texCoords(NULL),
    tangents(NULL)
{
    if (!trigArraysInitialized)
        InitTrigArrays();

    int maxThetaSteps = thetaDivisions / minStep;
    int maxPhiSteps = phiDivisions / minStep;
    int maxVertices = (maxPhiSteps + 1) * (maxThetaSteps + 1);
    
    vertices = new float[maxVertices * 3];
    normals = new float[maxVertices * 3];
    texCoords = new float[maxVertices * 2];
    tangents = new float[maxVertices * 3];

    indices = new unsigned short[maxPhiSteps * 2 * (maxThetaSteps + 1)];
}

LODSphereMesh::~LODSphereMesh()
{
    if (vertices != NULL)
        delete[] vertices;
    if (texCoords != NULL)
        delete[] texCoords;
    if (normals != NULL)
        delete[] normals;
    if (tangents != NULL)
        delete[] tangents;
}


void LODSphereMesh::render()
{
    render(Normals | TexCoords0);
}


void LODSphereMesh::render(unsigned int attributes)
{
    int lod = 64;
    int step = maxDivisions / lod;
    int phi0 = 0;
    int thetaExtent = maxDivisions;
    int phiExtent = thetaExtent / 2;

    // Set up the mesh vertices 
    int nRings = phiExtent / step;
    int nSlices = thetaExtent / step;

    int n2 = 0;
    for (int i = 0; i < nRings - 1; i++)
    {
        for (int j = 0; j <= nSlices; j++)
        {
            indices[n2 + 0] = i * (nSlices + 1) + j;
            indices[n2 + 1] = (i + 1) * (nSlices + 1) + j;
            n2 += 2;
        }
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, vertices);

    if (normals != NULL && ((attributes & Normals) != 0))
    {
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, 0, normals);
    }
    else
    {
        glDisableClientState(GL_NORMAL_ARRAY);
    }

    if (texCoords != NULL && ((attributes & TexCoords0) != 0))
    {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
    }
    else
    {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    glDisableClientState(GL_COLOR_ARRAY);

    // Use nVidia's vertex program extension . . .  right now, we
    // just assume that we only send down tangents if we're using this
    // extension.  Need to come up with a better solution . . .
    if (tangents != NULL && ((attributes & Tangents) != 0))
    {
        glEnableClientState(GL_VERTEX_ATTRIB_ARRAY6_NV);
        glVertexAttribPointerNV(6, 3, GL_FLOAT, 0, tangents);
    }

    // Now, render the sphere section by section.
    renderSection(0, 0, thetaExtent, step, attributes);

    if (tangents != NULL && ((attributes & Tangents) != 0))
    {
        glDisableClientState(GL_VERTEX_ATTRIB_ARRAY6_NV);
    }
}


void LODSphereMesh::renderSection(int phi0, int theta0,
                                  int extent,
                                  int step,
                                  unsigned int attributes)
{
    // assert(step >= minStep);
    // assert(phi0 + extent <= maxDivisions);
    // assert(theta0 + extent / 2 < maxDivisions);
    // assert(isPow2(extent));
    int count = 0;
    int thetaExtent = extent;
    int phiExtent = extent / 2;
    int theta1 = theta0 + thetaExtent;
    int phi1 = phi0 + phiExtent;
    float du = (float) 1.0f / thetaDivisions;
    float dv = (float) 1.0f / phiDivisions;
    int n3 = 0;
    int n2 = 0;

    for (int phi = phi0; phi <= phi1; phi += step)
    {
        float cphi = cosPhi[phi];
        float sphi = sinPhi[phi];

        for (int theta = theta0; theta <= theta1; theta += step)
        {
            float ctheta = cosTheta[theta];
            float stheta = sinTheta[theta];

            float x = cphi * ctheta;
            float y = sphi;
            float z = cphi * stheta;
            vertices[n3]      = x;
            vertices[n3 + 1]  = y;
            vertices[n3 + 2]  = z;
            normals[n3]       = x;
            normals[n3 + 1]   = y;
            normals[n3 + 2]   = z;
            texCoords[n2]     = 1.0f - theta * du;
            texCoords[n2 + 1] = 1.0f - phi * dv;

            // Compute the tangent--required for bump mapping
            float tx = sphi * stheta;
            float ty = -cphi;
            float tz = sphi * ctheta;
            tangents[n3]      = tx;
            tangents[n3 + 1]  = ty;
            tangents[n3 + 2]  = tz;

            n2 += 2;
            n3 += 3;
        }
    }

    int nRings = phiExtent / step;
    int nSlices = thetaExtent / step;
    for (int i = 0; i < nRings; i++)
    {
        glDrawElements(GL_QUAD_STRIP,
                       (nSlices + 1) * 2,
                       GL_UNSIGNED_SHORT,
                       indices + (nSlices + 1) * 2 * i);
    }
}
