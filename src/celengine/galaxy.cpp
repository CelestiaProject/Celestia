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

static const unsigned int GALAXY_POINTS  = 7000;

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
    { "S0",  Galaxy::S0 },
    { "Sa",  Galaxy::Sa },
    { "Sb",  Galaxy::Sb },
    { "Sc",  Galaxy::Sc },
    { "SBa", Galaxy::SBa },
    { "SBb", Galaxy::SBb },
    { "SBc", Galaxy::SBc },
    { "E0",  Galaxy::E0 },
    { "E1",  Galaxy::E1 },
    { "E2",  Galaxy::E2 },
    { "E3",  Galaxy::E3 },
    { "E4",  Galaxy::E4 },
    { "E5",  Galaxy::E5 },
    { "E6",  Galaxy::E6 },
    { "E7",  Galaxy::E7 },
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


const char* Galaxy::getType() const
{
    return GalaxyTypeNames[(int) type].name;
}

void Galaxy::setType(const std::string& typeStr)
{
    type = Galaxy::Irr;
    for (int i = 0; i < (int) (sizeof(GalaxyTypeNames) / sizeof(GalaxyTypeNames[0])); ++i)
    {
        if (GalaxyTypeNames[i].name == typeStr)
        {
            type = GalaxyTypeNames[i].type;
            break;
        }
    }

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
    setType(typeName);
    
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

    float distanceToDSO = offset.length() - getRadius();
    if (distanceToDSO < 0)
        distanceToDSO = 0;

    float minimumFeatureSize = pixelSize * distanceToDSO;

    //Mat4f m = (getOrientation().toMatrix4() *
    //           Mat4f::scaling(form->scale) *
    //           Mat4f::scaling(getRadius()));

    float  size  = 2 * getRadius();
    Mat3f m =
        Mat3f::scaling(form->scale)*getOrientation().toMatrix3()*Mat3f::scaling(size);

    // Note: fixed missing factor of 2 in getRadius() scaling of galaxy diameter!
    // Note: fixed correct ordering of (non-commuting) operations!

    int   pow2  = 1;

    vector<Blob>* points = form->blobs;
    unsigned int nPoints = (unsigned int) (points->size() * clamp(getDetail()));

    // corrections to avoid excessive brightening if viewed e.g. edge-on

    float brightness_corr = 1.0f;
    float cosi;

    if (type < E0 || type > E3) //all galaxies, except ~round elliptics
    {
        cosi = Vec3f(0,1,0) * getOrientation().toMatrix3()
                            * offset/offset.length();
        brightness_corr = (float) sqrt(abs(cosi));
        if (brightness_corr < 0.2f)
            brightness_corr = 0.2f;
    }

    if (type > E3) // only elliptics with higher ellipticities
    {
        cosi = Vec3f(1,0,0) * getOrientation().toMatrix3()
                            * offset/offset.length();
        brightness_corr = brightness_corr * (float) sqrt(abs((cosi)));
        if (brightness_corr < 0.65f)
            brightness_corr = 0.65f;
    }

    glBegin(GL_QUADS);
    for (unsigned int i = 0; i < nPoints; ++i)
    {
        Blob    b    = (*points)[i];
        Point3f p    = b.position * m;            
        float   br   = b.brightness;

        // Reddening for elliptical galaxies

        b.colorIndex = type < E0 ? b.colorIndex : (unsigned int) ceil(0.3f*b.colorIndex);
        Color   c    = colorTable[b.colorIndex];     // lookup static color table
          
        Point3f relPos = p + offset;
  
        if ((i & pow2) != 0)
        {
            pow2 <<= 1;
            size /= 1.57f;
            if (size < minimumFeatureSize)
                break;
        }          
            
        float screenFrac = size / relPos.distanceFromOrigin();
          
        if (screenFrac < 0.1f)
        {
            float a  = 3.5f * (0.1f - screenFrac) * brightness_corr * brightness * br;
            
            glColor4f(c.red(), c.green(), c.blue(), (4.0f*lightGain + 1.0f)*a);
              
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
    float discSizeInPixels = pixelSize * getRadius() / offset.length();
    unsigned int nRings = (unsigned int) (discSizeInPixels / 4.0f);
    unsigned int nSlices = (unsigned int) (discSizeInPixels / 4.0f);
    nRings = max(nRings, 100);
    nSlices = max(nSlices, 100);

    VertexProcessor* vproc = context.getVertexProcessor();
    if (vproc == NULL)
        return;

    int e = min(max((int) type - (int) E0, 0), 7);
    Vec3f scale = Vec3f(1.0f, 0.9f, 1.0f) * getRadius();
    Vec3f eyePos_obj = -offset * (~getOrientation()).toMatrix3();

    vproc->enable();
    vproc->use(vp::ellipticalGalaxy);

    vproc->parameter(vp::EyePosition, eyePos_obj);
    vproc->parameter(vp::Scale, scale);
    vproc->parameter(vp::InverseScale,
                     Vec3f(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z));
    vproc->parameter((vp::Parameter) 23, eyePos_obj.length() / scale.x, 0.0f, 0.0f, 0.0f);

    glRotate(getOrientation());

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


unsigned int Galaxy::getRenderMask() const
{
    return Renderer::ShowGalaxies;
}


unsigned int Galaxy::getLabelMask() const
{
    return Renderer::GalaxyLabels;
}


void Galaxy::increaseLightGain()
{
    lightGain  += 0.05f;
    if (lightGain > 1.0f)
        lightGain  = 1.0f;
}  


void Galaxy::decreaseLightGain()
{
    lightGain  -= 0.05f;
    if (lightGain < 0.0f)
        lightGain  = 0.0f;  
}  


float Galaxy::getLightGain()
{
    return lightGain;
}  


void Galaxy::hsv2rgb( float *r, float *g, float *b, float h, float s, float v )
{
// r,g,b values are from 0 to 1
// h = [0,360], s = [0,1], v = [0,1]

   int i;
   float f, p, q, t;

   if( s == 0 ) {
       // achromatic (grey)
       *r = *g = *b = v;
   return;
   }

   h /= 60;            // sector 0 to 5
   i = (int) floorf( h );
   f = h - (float) i;            // factorial part of h
   p = v * ( 1 - s );
   q = v * ( 1 - s * f );
   t = v * ( 1 - s * ( 1 - f ) );

   switch( i ) {
   case 0:
     *r = v;
     *g = t;
     *b = p;
     break;
   case 1:
     *r = q;
     *g = v;
     *b = p;
     break;
   case 2:
     *r = p;
     *g = v;
     *b = t;
     break;
   case 3:
     *r = p;
     *g = q;
     *b = v;
     break;
   case 4:
     *r = t;
     *g = p;
     *b = v;
     break;
   default:              *r = v;
     *g = p;
     *b = q;
     break;
   }
}


GalacticForm* buildGalacticForms(const std::string& filename)
{
    unsigned int galaxySize = GALAXY_POINTS;
    Blob b;    
    vector<Blob>* galacticPoints = new vector<Blob>;
    galacticPoints->reserve(galaxySize);

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
        b.colorIndex  = (unsigned int) (b.position.distanceFromOrigin() * 255);
      
        galacticPoints->push_back(b);
    }          
    
    GalacticForm* galacticForm  = new GalacticForm();
    galacticForm->blobs         = galacticPoints;
    galacticForm->scale         = Vec3f(1.0f, 1.0f, 1.0f);
    
    return galacticForm;
};


void InitializeForms()
{
    unsigned int i = 0;

    // build color table:
    for (i = 0; i < 256; ++i)
    {
        float rr,gg,bb;
        float rho  = (float) i / 255.0f;
        // generic Hue profile as deduced from true-color imaging for spirals
        float h = 30.0f*tanh(15.68f*(0.1086f-rho));
        if ( rho > 0.1086f)
            h += 260.0f;
        //convert Hue to RGB
        Galaxy::hsv2rgb( &rr, &gg, &bb, h , 0.15f, 1.0f);
        colorTable[i]  = Color( rr,gg,bb);
    }
    // Spiral Galaxies, 7 classical Hubble types
    
    spiralForms   = new GalacticForm*[7];
	
    spiralForms[Galaxy::S0]   = buildGalacticForms("models/S0.pts");
    spiralForms[Galaxy::Sa]   = buildGalacticForms("models/Sa.pts");
    spiralForms[Galaxy::Sb]   = buildGalacticForms("models/Sb.pts");
    spiralForms[Galaxy::Sc]   = buildGalacticForms("models/Sc.pts");
    spiralForms[Galaxy::SBa]  = buildGalacticForms("models/SBa.pts");

    spiralForms[Galaxy::SBb]  = buildGalacticForms("models/SBb.pts");
    spiralForms[Galaxy::SBc]  = buildGalacticForms("models/SBc.pts");

    // Elliptical Galaxies , 8 classical Hubble types, E0..E7,
    //
    // To save space: generate spherical E0 template from S0 disk
    // via rescaling by (1.0f, 3.8f, 1.0f).

    ellipticalForms = new GalacticForm*[8];

    for (unsigned int eform  = 0; eform <= 7; ++eform)
    {
        float ell = 1.0f - (float) eform / 8.0f;
        ellipticalForms[eform] = new GalacticForm();
        ellipticalForms[eform]->blobs = spiralForms[Galaxy::S0]->blobs;
        // note the correct x,y-alignment of 'ell' scaling!!
        ellipticalForms[eform]->scale = Vec3f(ell, 3.8*ell, 1.0f);
    }

    //Irregular Galaxies
    unsigned int galaxySize = GALAXY_POINTS;
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
                b.colorIndex = (unsigned int) (r*255);
                irregularPoints->push_back(b);
                ++i;
            }
        }
    }
    irregularForm        = new GalacticForm();
    irregularForm->blobs = irregularPoints;
    irregularForm->scale = Vec3f(1.0f, 0.1f, 1.0f);

    formsInitialized = true;
}


ostream& operator<<(ostream& s, const Galaxy::GalaxyType& sc)
{
    return s << GalaxyTypeNames[static_cast<int>(sc)].name;
}
