// galaxy.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <stdio.h>
#include <cassert>
#include "celestia.h"
#include <celmath/mathlib.h>
#include <celmath/perlin.h>
#include "astro.h"
#include "galaxy.h"
#include <celutil/util.h>
#include <celutil/debug.h>
#include "gl.h"
#include "vecgl.h"
#include "texture.h"

using namespace std;

static bool formsInitialized = false;
static vector<Point3f>* spiralPoints = NULL;
static vector<Point3f>* ellipticalPoints = NULL;
static vector<Point3f>* irregularPoints = NULL;
static GalacticForm* spiralForm = NULL;
static GalacticForm* irregularForm = NULL;
static GalacticForm** ellipticalForms = NULL;
static void InitializeForms();

static Texture* galaxyTex = NULL;

struct GalaxyTypeName 
{
    const char* name;
    Galaxy::GalaxyType type;
};

static GalaxyTypeName GalaxyTypeNames[] = 
{
    { "S0", Galaxy::S0 },
    { "Sa", Galaxy::Sa },
    { "Sb", Galaxy::Sb },
    { "Sc", Galaxy::Sc },
    { "SBa", Galaxy::SBa },
    { "SBb", Galaxy::SBb },
    { "SBc", Galaxy::SBc },
    { "E0", Galaxy::E0 },
    { "E1", Galaxy::E1 },
    { "E2", Galaxy::E2 },
    { "E3", Galaxy::E3 },
    { "E4", Galaxy::E4 },
    { "E5", Galaxy::E5 },
    { "E6", Galaxy::E6 },
    { "E7", Galaxy::E7 },
    { "Irr", Galaxy::Irr },
};


static void GalaxyTextureEval(float u, float v, float w,
                              unsigned char *pixel)
{
    float r = 0.9f - (float) sqrt(u * u + v * v);
    if (r < 0)
        r = 0;

    int pixVal = (int) (r * 255.99f);
    pixel[0] = 65;
    pixel[1] = 64;
    pixel[2] = 65;
    pixel[3] = pixVal;
}


Galaxy::Galaxy() :
    detail(1.0f),
    form(NULL)
{
}


float Galaxy::getDetail() const
{
    return detail;
}

void Galaxy::setDetail(float d)
{
    detail = d;
}


Galaxy::GalaxyType Galaxy::getType() const
{
    return type;
}

void Galaxy::setType(Galaxy::GalaxyType _type)
{
    type = _type;

    if (!formsInitialized)
        InitializeForms();
    switch (type)
    {
    case S0:
    case Sa:
    case Sb:
    case Sc:
    case SBa:
    case SBb:
    case SBc:
        form = spiralForm;
        break;
    case Irr:
        form = irregularForm;
        break;
    case E0:
    case E1:
    case E2:
    case E3:
    case E4:
    case E5:
    case E6:
    case E7:
        form = ellipticalForms[type - E0];
        break;
    }
}


GalacticForm* Galaxy::getForm() const
{
    return form;
}


bool Galaxy::load(AssociativeArray* params, const string& resPath)
{
    double detail = 1.0;
    params->getNumber("Detail", detail);
    setDetail((float) detail);

    string typeName;
    params->getString("Type", typeName);
    Galaxy::GalaxyType type = Galaxy::Irr;
    for (int i = 0; i < (int) (sizeof(GalaxyTypeNames) / sizeof(GalaxyTypeNames[0])); i++)
    {
        if (GalaxyTypeNames[i].name == typeName)
        {
            type = GalaxyTypeNames[i].type;
            break;
        }
    }
    setType(type);
    
    return DeepSkyObject::load(params, resPath);
}


void Galaxy::render(const Vec3f& offset,
                    const Quatf& viewerOrientation,
                    float brightness,
                    float pixelSize)
{
    if (form == NULL)
        return;

    if (galaxyTex == NULL)
    {
        galaxyTex = CreateProceduralTexture(128, 128, GL_RGBA,
                                            GalaxyTextureEval);
    }
    assert(galaxyTex != NULL);

    glEnable(GL_TEXTURE_2D);
    galaxyTex->bind();

    Mat3f viewMat = viewerOrientation.toMatrix3();
    Vec3f v0 = Vec3f(-1, -1, 0) * viewMat;
    Vec3f v1 = Vec3f( 1, -1, 0) * viewMat;
    Vec3f v2 = Vec3f( 1,  1, 0) * viewMat;
    Vec3f v3 = Vec3f(-1,  1, 0) * viewMat;

    float distanceToObject = offset.length() - getRadius();
    if (distanceToObject < 0)
        distanceToObject = 0;
    float minimumFeatureSize = pixelSize * 0.5f * distanceToObject;

    Mat4f m = (getOrientation().toMatrix4() *
               Mat4f::scaling(form->scale) *
               Mat4f::scaling(getRadius()));
    float size = getRadius();
    int pow2 = 1;

    vector<Point3f>* points = form->points;
    int nPoints = (int) (points->size() * clamp(getDetail()));

    glBegin(GL_QUADS);
    for (int i = 0; i < nPoints; i++)
    {
        Point3f p = (*points)[i] * m;
        Point3f relPos = p + offset;

        if ((i & pow2) != 0)
        {
            pow2 <<= 1;
            size /= 1.5f;
            if (size < minimumFeatureSize)
                break;
        }

        {
            float distance = relPos.distanceFromOrigin();
            float screenFrac = size / distance;

            if (screenFrac < 0.05f)
            {
                float a = 8 * (0.05f - screenFrac) * brightness;
                glColor4f(1, 1, 1, a);
                glTexCoord2f(0, 0);
                glVertex(p + (v0 * size));
                glTexCoord2f(1, 0);
                glVertex(p + (v1 * size));
                glTexCoord2f(1, 1);
                glVertex(p + (v2 * size));
                glTexCoord2f(0, 1);
                glVertex(p + (v3 * size));
            }
        }
    }
    glEnd();
}


void InitializeForms()
{
    int galaxySize = 5000;
    int i;

    // Initialize the spiral form--this is a big hack
    spiralPoints = new vector<Point3f>();
    spiralPoints->reserve(galaxySize);
    for (i = 0; i < galaxySize; i++)
    {
        float r = Mathf::frand();
        float theta = Mathf::sfrand() * PI;

        if (r > 0.2f)
        {
            theta = (Mathf::sfrand() + Mathf::sfrand() + Mathf::sfrand()) * (float) PI / 2.0f / 3.0f;
            if (Mathf::sfrand() < 0)
                theta += (float) PI;
        }
        theta += (float) log(r + 1) * 3 * PI;
        float x = r * (float) cos(theta);
        float z = r * (float) sin(theta);
        float y = Mathf::sfrand() * 0.1f * 1.0f / (1 + 2 * r);
        spiralPoints->insert(spiralPoints->end(), Point3f(x, y, z));
    }
    spiralForm = new GalacticForm();
    spiralForm->points = spiralPoints;
    spiralForm->scale = Vec3f(1, 1, 1);

    irregularPoints = new vector<Point3f>;
    irregularPoints->reserve(galaxySize);
    i = 0;
    while (i < galaxySize)
    {
        Point3f p(Mathf::sfrand(), Mathf::sfrand(), Mathf::sfrand());
        float r = p.distanceFromOrigin();
        if (r < 1)
        {
            float prob = (1 - r) * (fractalsum(Point3f(p.x + 5, p.y + 5, p.z + 5), 8) + 1) * 0.5f;
            if (Mathf::frand() < prob)
            {
                irregularPoints->insert(irregularPoints->end(), p);
                i++;
            }
        }
    }
    irregularForm = new GalacticForm();
    irregularForm->points = irregularPoints;
    irregularForm->scale = Vec3f(1, 1, 1);

    ellipticalPoints = new vector<Point3f>();
    ellipticalPoints->reserve(galaxySize);
    i = 0;
    while (i < galaxySize)
    {
        Point3f p(Mathf::sfrand(), Mathf::sfrand(), Mathf::sfrand());
        float r = p.distanceFromOrigin();
        if (r < 1)
        {
            if (Mathf::frand() < cube(1 - r))
            {
                ellipticalPoints->insert(ellipticalPoints->end(), p);
                i++;
            }
        }
    }

    ellipticalForms = new GalacticForm*[8];
    for (int eform = 0; eform <= 7; eform++)
    {
        ellipticalForms[eform] = new GalacticForm();
        ellipticalForms[eform]->points = ellipticalPoints;
        ellipticalForms[eform]->scale = Vec3f(1.0f, 1.0f - (float) eform / 8.0f, 1.0f);
    }

    formsInitialized = true;
}


ostream& operator<<(ostream& s, const Galaxy::GalaxyType& sc)
{
    return s << GalaxyTypeNames[static_cast<int>(sc)].name;
}
