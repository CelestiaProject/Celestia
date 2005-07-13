// galaxy.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <fstream>
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
#include "render.h"
#include "texture.h"

using namespace std;

static Color colorTable[256];

static const unsigned int GALAXY_POINTS  = 5000;

static bool formsInitialized = false;

static GalacticForm** spiralForms     = NULL;
static GalacticForm** ellipticalForms = NULL;
static GalacticForm*  irregularForm   = NULL;

static Texture* galaxyTex = NULL;

static void InitializeForms();

float Galaxy::lightGain  = 0.0f;


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
    pixel[0] = 255;//65;
    pixel[1] = 255;//64;
    pixel[2] = 255;//65;
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
        form = spiralForms[type - S0];
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
        //form = NULL;
        break;
    case Irr:
        form = irregularForm;
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
    for (int i = 0; i < (int) (sizeof(GalaxyTypeNames) / sizeof(GalaxyTypeNames[0])); ++i)
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


void Galaxy::render(const GLContext& context,
                    const Vec3f& offset,
                    const Quatf& viewerOrientation,
                    float brightness,
                    float pixelSize)
{
    if (form == NULL)
        renderGalaxyEllipsoid(context, offset, viewerOrientation, brightness, pixelSize);
    else
        renderGalaxyPointSprites(context, offset, viewerOrientation, brightness, pixelSize);
}


void Galaxy::renderGalaxyPointSprites(const GLContext& context,
                                      const Vec3f& offset,
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
    float size  = getRadius() * 1.25f;
    int   pow2  = 1;

    vector<Blob>* points = form->blobs;
    unsigned int nPoints = (unsigned int) (points->size() * clamp(getDetail()));

    glBegin(GL_QUADS);
    for (unsigned int i = 0; i < nPoints; ++i)
    {
        Blob    b    = (*points)[i];
        Point3f p    = b.position * m;            
        float   br   = b.brightness;
        Color   c    = colorTable[b.colorIndex];     // lookup static color table
          
        Point3f relPos = p + offset;
  
        if ((i & pow2) != 0)
        {
            pow2 <<= 1;
            size /= 1.5f;
            if (size < minimumFeatureSize)
                break;
        }          
            
        float screenFrac = size / relPos.distanceFromOrigin();
          
        if (screenFrac < 0.05f)
        {
            float a  = 20 * (0.05f - screenFrac) * brightness * br;
            
            glColor4f(c.red(), c.green(), c.blue(), (0.975f*a)*lightGain + 0.025f*a);
              
            glTexCoord2f(0, 0);          glVertex(p + (v0 * size));
            glTexCoord2f(1, 0);          glVertex(p + (v1 * size));
            glTexCoord2f(1, 1);          glVertex(p + (v2 * size));
            glTexCoord2f(0, 1);          glVertex(p + (v3 * size));
        }
    }
    glEnd();
}


void Galaxy::renderGalaxyEllipsoid(const GLContext& context,
                                   const Vec3f& offset,
                                   const Quatf& viewerOrientation,
                                   float brightness,
                                   float pixelSize)
{
    unsigned int nRings = 20;
    unsigned int nSlices = 20;

    VertexProcessor* vproc = context.getVertexProcessor();
    if (vproc == NULL)
        return;

    int e = min(max((int) type - (int) E0, 0), 7);
    Vec3f scale = Vec3f(1.0f, 0.2f, 1.0f) * getRadius();
    Vec3f eyePos = -offset * (viewerOrientation).toMatrix3();

    vproc->enable();
    vproc->use(vp::ellipticalGalaxy);

    vproc->parameter(vp::EyePosition, eyePos);
    vproc->parameter(vp::Scale, scale);
    vproc->parameter(vp::InverseScale,
                     Vec3f(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z));

    glDisable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 0.3f);
    for (unsigned int i = 0; i < nRings; i++)
    {
        float phi0 = (float) PI * ((float) i / (float) nRings - 0.5f);
        float phi1 = (float) PI * ((float) (i + 1) / (float) nRings - 0.5f);

        glBegin(GL_QUAD_STRIP);
        for (unsigned int j = 0; j <= nSlices; j++)
        {
            float theta = (float) (PI * 2) * (float) j / (float) nSlices;
            float sinTheta = (float) sin(theta);
            float cosTheta = (float) cos(theta);
            
            glVertex3f((float) cos(phi0) * cosTheta * scale.x,
                       (float) sin(phi0)            * scale.y,
                       (float) cos(phi0) * sinTheta * scale.z);
            glVertex3f((float) cos(phi1) * cosTheta * scale.x,
                       (float) sin(phi1)            * scale.y,
                       (float) cos(phi1) * sinTheta * scale.z);
        }
        glEnd();
    }
    glEnable(GL_TEXTURE_2D);

    vproc->disable();
}


unsigned int Galaxy::getRenderMask()
{
    return Renderer::ShowGalaxies;
}


unsigned int Galaxy::getLabelMask()
{
    return Renderer::GalaxyLabels;
}


void Galaxy::increaseLightGain()
{
    lightGain  += 0.0125f;
    if (lightGain > 1.0f)
        lightGain  = 1.0f;
}  


void Galaxy::decreaseLightGain()
{
    lightGain  -= 0.0125f;
    if (lightGain < 0.0f)
        lightGain  = 0.0f;  
}  


float Galaxy::getLightGain()
{
    return lightGain;
}  


GalacticForm* buildSpiralForms(const std::string& filename)
{
    Blob b;    
    vector<Blob>* spiralPoints = new vector<Blob>;
    //spiralPoints->reserve(galaxySize);                //TODO!!

    ifstream inFile(filename.c_str());    
    while (inFile.good())
    {
        float x, y, z, br;
        inFile >> x;
        inFile >> y;
        inFile >> z;
        inFile >> br;
      
        b.position    = Point3f(x, y, z);
        b.brightness  = br;
        b.colorIndex  = (unsigned int) (b.position.distanceFromOrigin() * 256);
      
        spiralPoints->push_back(b);
    }          
    
    GalacticForm* spiralForm  = new GalacticForm();
    spiralForm->blobs         = spiralPoints;
    spiralForm->scale         = Vec3f(1.0f, 1.0f, 1.0f);
    
    return spiralForm;
};


void InitializeForms()
{
    unsigned int i = 0;

    // build color table:
    for (i = 0; i < 256; ++i)
    {
      float f  = (float) i / 256.0f * 0.9f;
      
      colorTable[i]  = Color( 0.9f - 1.0f*square(f), 
                              0.9f - 1.0f*f, 
                              0.9f - 1.0f*cube(f) 
                            );
    }
    
    
    spiralForms   = new GalacticForm*[7];
	
    spiralForms[Galaxy::S0]   = buildSpiralForms("Sa.pts");  
    
    spiralForms[Galaxy::Sa]   = buildSpiralForms("Sa.pts");     
    spiralForms[Galaxy::Sb]   = buildSpiralForms("Sb.pts");  
    spiralForms[Galaxy::Sc]   = buildSpiralForms("Sc.pts");              
    
    spiralForms[Galaxy::SBa]  = buildSpiralForms("SBa.pts");                     
    spiralForms[Galaxy::SBb]  = buildSpiralForms("SBa.pts");    
    spiralForms[Galaxy::SBc]  = buildSpiralForms("SBc.pts");
    
    
    unsigned int galaxySize = 5000;
    Blob b;    
    Point3f p;    
    
    
    vector<Blob>* irregularPoints = new vector<Blob>;
    irregularPoints->reserve(galaxySize);
    
    i = 0;
    while (i < galaxySize)
    {
        p        = Point3f(Mathf::sfrand(), Mathf::sfrand(), Mathf::sfrand());
        float r  = p.distanceFromOrigin();
        if (r < 1)
        {
            float prob = (1 - r) * (fractalsum(Point3f(p.x + 5, p.y + 5, p.z + 5), 8) + 1) * 0.5f;
            if (Mathf::frand() < prob)
            {
                b.position   = p;
                b.brightness = 1.0f;  
                b.colorIndex = (unsigned int) (r*256);
                irregularPoints->push_back(b);
                ++i;
            }
        }
    }
    irregularForm        = new GalacticForm();
    irregularForm->blobs = irregularPoints;
    irregularForm->scale = Vec3f(1, 1, 1);

    
    
    vector<Blob>* ellipticalPoints = new vector<Blob>;
    ellipticalPoints->reserve(galaxySize);
    
    i = 0;
    while (i < galaxySize)
    {
        p        = Point3f(Mathf::sfrand(), Mathf::sfrand(), Mathf::sfrand());
        float r  = p.distanceFromOrigin();
        if (r < 1)
        {
            if (Mathf::frand() < cube(1 - r))
            {
                b.position   = p;
                b.brightness = 1.0f;
                b.colorIndex = (unsigned int) (r*256);
                ellipticalPoints->push_back(b);
                ++i;
            }
        }
    }
    ellipticalForms = new GalacticForm*[8];
    for (unsigned int eform  = 0; eform <= 7; ++eform)
    {
        ellipticalForms[eform]        = new GalacticForm();
        ellipticalForms[eform]->blobs = ellipticalPoints;
        ellipticalForms[eform]->scale = Vec3f(1.0f, 1.0f - (float) eform / 8.0f, 1.0f);
    }

    formsInitialized = true;
}


ostream& operator<<(ostream& s, const Galaxy::GalaxyType& sc)
{
    return s << GalaxyTypeNames[static_cast<int>(sc)].name;
}
