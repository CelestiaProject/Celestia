// galaxy.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel, Fridger Schrempp, and Toti
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "render.h"
#include "astro.h"
#include "galaxy.h"
#include "vecgl.h"
#include "texture.h"
#include <celmath/mathlib.h>
#include <celmath/perlin.h>
#include <celmath/intersect.h>
#include <celutil/gettext.h>
#include <celutil/debug.h>
#include <celcompat/filesystem.h>
#include <fstream>
#include <algorithm>
#include <random>
#include <cassert>

using namespace Eigen;
using namespace std;
using namespace celmath;
using namespace celestia;

static int width = 128, height = 128;
static const unsigned int GALAXY_POINTS  = 3500;

static bool formsInitialized = false;

static GalacticForm** spiralForms     = nullptr;
static GalacticForm** ellipticalForms = nullptr;
static GalacticForm*  irregularForm   = nullptr;

static Texture* galaxyTex = nullptr;
static Texture* colorTex  = nullptr;

static void InitializeForms();
static GalacticForm* buildGalacticForms(const fs::path& filename);

float Galaxy::lightGain  = 0.0f;

bool operator< (const Blob& b1, const Blob& b2)
{
    return (b1.position.squaredNorm() < b2.position.squaredNorm());
}

typedef vector<Blob, aligned_allocator<Blob> > BlobVector;
class GalacticForm
{
public:
    BlobVector* blobs;
    Vector3f scale;
};

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


static void GalaxyTextureEval(float u, float v, float /*w*/, unsigned char *pixel)
{
    float r = 0.9f - (float) sqrt(u * u + v * v );
    if (r < 0)
        r = 0;

    auto pixVal = (int) (r * 255.99f);
#ifdef HDR_COMPRESS
    pixel[0] = 127;
    pixel[1] = 127;
    pixel[2] = 127;
#else
    pixel[0] = 255;//65;
    pixel[1] = 255;//64;
    pixel[2] = 255;//65;
#endif
    pixel[3] = pixVal;
}

static void ColorTextureEval(float u, float /*v*/, float /*w*/, unsigned char *pixel)
{
    unsigned int i = (u*0.5f + 0.5f)*255.99f; // [-1, 1] -> [0, 255]

    // generic Hue profile as deduced from true-color imaging for spirals
    // Hue in degrees
    float hue = 25 * tanh(0.0615f * (27 - i));
    if (i >= 28) hue += 220;
    //convert Hue to RGB
    float r, g, b;
    DeepSkyObject::hsv2rgb(&r, &g, &b, hue, 0.20f, 1.0f);
    pixel[0] = (unsigned char) (r * 255.99f);
    pixel[1] = (unsigned char) (g * 255.99f);
    pixel[2] = (unsigned char) (b * 255.99f);
}

float Galaxy::getDetail() const
{
    return detail;
}


void Galaxy::setDetail(float d)
{
    detail = d;
}


void Galaxy::setCustomTmpName(const string& tmpNameStr)
{
    if (customTmpName == nullptr)
        customTmpName = new string(tmpNameStr);
    else
        *customTmpName = tmpNameStr;
}


string Galaxy::getCustomTmpName() const
{
    if (customTmpName == nullptr)
        return "";
    else
        return *customTmpName;
}


const char* Galaxy::getType() const
{
    return GalaxyTypeNames[(int) type].name;
}


void Galaxy::setType(const string& typeStr)
{
    type = Galaxy::Irr;
    auto iter = std::find_if(begin(GalaxyTypeNames), end(GalaxyTypeNames),
                             [typeStr](GalaxyTypeName& g) { return g.name == typeStr; });
    if (iter != end(GalaxyTypeNames))
        type = iter->type;

    if (!formsInitialized)
        InitializeForms();

    if (customTmpName != nullptr)
    {
        form = buildGalacticForms(fs::path("models") / *customTmpName);
    }
    else
    {
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
            //form = nullptr;
            break;
        case Irr:
            form = irregularForm;
            break;
        }
    }
}


string Galaxy::getDescription() const
{
    return fmt::sprintf(_("Galaxy (Hubble type: %s)"), getType());
}


GalacticForm* Galaxy::getForm() const
{
    return form;
}

const char* Galaxy::getObjTypeName() const
{
    return "galaxy";
}


// TODO: This value is just a guess.
// To be optimal, it should actually be computed:
static const float RADIUS_CORRECTION     = 0.025f;
static const float MAX_SPIRAL_THICKNESS  = 0.06f;

bool Galaxy::pick(const Ray3d& ray,
                  double& distanceToPicker,
                  double& cosAngleToBoundCenter) const
{
    if (form == nullptr)
        return false;

    if (!isVisible())
        return false;

    // The ellipsoid should be slightly larger to compensate for the fact
    // that blobs are considered points when galaxies are built, but have size
    // when they are drawn.
    float yscale = (type < E0 )? MAX_SPIRAL_THICKNESS: form->scale.y() + RADIUS_CORRECTION;
    Vector3d ellipsoidAxes(getRadius()*(form->scale.x() + RADIUS_CORRECTION),
                           getRadius()* yscale,
                           getRadius()*(form->scale.z() + RADIUS_CORRECTION));

    Matrix3d rotation = getOrientation().cast<double>().toRotationMatrix();
    return testIntersection(Ray3d(ray.origin - getPosition(), ray.direction).transform(rotation),
                            Ellipsoidd(ellipsoidAxes),
                            distanceToPicker,
                            cosAngleToBoundCenter);
}


bool Galaxy::load(AssociativeArray* params, const fs::path& resPath)
{
    double detail = 1.0;
    params->getNumber("Detail", detail);
    setDetail((float) detail);

    string customTmpName;
    if(params->getString("CustomTemplate", customTmpName))
        setCustomTmpName(customTmpName);

    string typeName;
    params->getString("Type", typeName);
    setType(typeName);

    return DeepSkyObject::load(params, resPath);
}


void Galaxy::render(const Vector3f& offset,
                    const Quaternionf& viewerOrientation,
                    float brightness,
                    float pixelSize,
                    const Matrices& m,
                    Renderer* renderer)
{
    if (form == nullptr)
    {
        //renderGalaxyEllipsoid(offset, viewerOrientation, brightness, pixelSize);
    }
    else
    {
        renderGalaxyPointSprites(offset, viewerOrientation, brightness, pixelSize, m, renderer);
    }
}

struct GalaxyVertex
{
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    Vector4f position;
    Matrix<GLushort, 4, 1> texCoord; // texCoord.x = x, texCoord.y = y, texCoord.z = color index, texCoord.w = alpha
};

static void draw(const GalaxyVertex *v, size_t count, const GLushort *indices)
{
    glVertexAttribPointer(CelestiaGLProgram::VertexCoordAttributeIndex,
                          4, GL_FLOAT, GL_FALSE,
                          sizeof(GalaxyVertex), v[0].position.data());
    glVertexAttribPointer(CelestiaGLProgram::TextureCoord0AttributeIndex,
                          4, GL_UNSIGNED_SHORT, GL_FALSE,
                          sizeof(GalaxyVertex), v[0].texCoord.data());
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, indices);
}

static GalaxyVertex *g_vertices = nullptr;
static GLushort *g_indices = nullptr;
constexpr const size_t maxPoints = 8192; // 256k buffer

void Galaxy::renderGalaxyPointSprites(const Vector3f& offset,
                                      const Quaternionf& viewerOrientation,
                                      float brightness,
                                      float pixelSize,
                                      const Matrices& ms,
                                      Renderer* renderer)
{
    if (form == nullptr)
        return;

    /* We'll first see if the galaxy's apparent size is big enough to
       be noticeable on screen; if it's not we'll break right here,
       avoiding all the overhead of the matrix transformations and
       GL state changes: */
    float distanceToDSO = offset.norm() - getRadius();
    if (distanceToDSO < 0)
        distanceToDSO = 0;

    float minimumFeatureSize = pixelSize * distanceToDSO;
    float size  = 2 * getRadius();

    if (size < minimumFeatureSize)
        return;

    auto *prog = renderer->getShaderManager().getShader("galaxy");
    if (prog == nullptr)
        return;

    if (galaxyTex == nullptr)
    {
        galaxyTex = CreateProceduralTexture(width, height, PixelFormat::RGBA,
                                            GalaxyTextureEval);
    }
    assert(galaxyTex != nullptr);
    glActiveTexture(GL_TEXTURE0);
    galaxyTex->bind();

    if (colorTex == nullptr)
    {
        colorTex = CreateProceduralTexture(256, 1, PixelFormat::RGBA,
                                           ColorTextureEval,
                                           Texture::EdgeClamp,
                                           Texture::NoMipMaps);
    }
    assert(colorTex != nullptr);
    glActiveTexture(GL_TEXTURE1);
    colorTex->bind();

    Matrix3f viewMat = viewerOrientation.conjugate().toRotationMatrix();
    Vector4f v0(Vector4f::Zero());
    Vector4f v1(Vector4f::Zero());
    Vector4f v2(Vector4f::Zero());
    Vector4f v3(Vector4f::Zero());
    v0.head(3) = viewMat * Vector3f(-1, -1, 0) * size;
    v1.head(3) = viewMat * Vector3f( 1, -1, 0) * size;
    v2.head(3) = viewMat * Vector3f( 1,  1, 0) * size;
    v3.head(3) = viewMat * Vector3f(-1,  1, 0) * size;

    Quaternionf orientation = getOrientation().conjugate();
    Matrix3f mScale = form->scale.asDiagonal() * size;
    Matrix3f mLinear = orientation.toRotationMatrix() * mScale;

    Matrix4f m = Matrix4f::Identity();
    m.topLeftCorner(3,3) = mLinear;
    m.block<3,1>(0, 3) = offset;

    int pow2 = 1;

    BlobVector* points = form->blobs;
    unsigned int nPoints = (unsigned int) (points->size() * clamp(getDetail()));
    // corrections to avoid excessive brightening if viewed e.g. edge-on

    float brightness_corr = 1.0f;
    float cosi;

    if (type < E0 || type > E3) //all galaxies, except ~round elliptics
    {
        cosi = (orientation * Vector3f::UnitY()).dot(offset) / offset.norm();
        brightness_corr = std::sqrt(std::abs(cosi));
        if (brightness_corr < 0.2f)
            brightness_corr = 0.2f;
    }
    if (type > E3) // only elliptics with higher ellipticities
    {
        cosi = (orientation * Vector3f::UnitX()).dot(offset) / offset.norm();
        brightness_corr = brightness_corr * std::abs(cosi);
        if (brightness_corr < 0.45f)
            brightness_corr = 0.45f;
    }


    Matrix4f mv = vecgl::translate(*ms.modelview, Vector3f(-offset));

    const float btot = ((type > SBc) && (type < Irr)) ? 2.5f : 5.0f;
    const float spriteScaleFactor = 1.0f / 1.55f;

    glEnableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glEnableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex);

    if (g_vertices == nullptr)
        g_vertices = new GalaxyVertex[maxPoints];
    if (g_indices == nullptr)
        g_indices = new GLushort[(maxPoints / 4 + 1) * 6];

    GLushort j = 0;

    prog->use();
    prog->setMVPMatrices(*ms.projection, mv);
    prog->samplerParam("galaxyTex") = 0;
    prog->samplerParam("colorTex") = 1;

    size_t vertex = 0, index = 0;
    for (unsigned int i = 0; i < nPoints; ++i)
    {
        if ((i & pow2) != 0)
        {
            pow2 <<= 1;
            size *= spriteScaleFactor;
            v0 *= spriteScaleFactor;
            v1 *= spriteScaleFactor;
            v2 *= spriteScaleFactor;
            v3 *= spriteScaleFactor;
            if (size < minimumFeatureSize)
                break;
        }

        const Blob& b  = (*points)[i];
        Vector4f    p  = m * b.position;
        float       br = b.brightness / 255.0f;

        float screenFrac = size / p.norm();
        if (screenFrac < 0.1f)
        {
            float a = (4.0f * lightGain + 1.0f) * btot * (0.1f - screenFrac) * brightness_corr * brightness * br;
            GLushort alpha = (GLushort) (min(1.0f, a) * 65535.99f);
            GLushort color = (GLushort) b.colorIndex;
            g_vertices[vertex++] = { p + v0, { 0, 0, color, alpha } };
            g_vertices[vertex++] = { p + v1, { 1, 0, color, alpha } };
            g_vertices[vertex++] = { p + v2, { 1, 1, color, alpha } };
            g_vertices[vertex++] = { p + v3, { 0, 1, color, alpha } };

            g_indices[index++] = j;
            g_indices[index++] = j + 1;
            g_indices[index++] = j + 2;
            g_indices[index++] = j;
            g_indices[index++] = j + 2;
            g_indices[index++] = j + 3;
            j += 4;

            if (vertex + 4 > maxPoints)
            {
                draw(g_vertices, index, g_indices);
                index = 0;
                vertex = 0;
                j = 0;
            }
        }
    }

    if (index > 0)
        draw(g_vertices, index, g_indices);

    glDisableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glDisableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex);
    glActiveTexture(GL_TEXTURE0);
}


#if 0
void Galaxy::renderGalaxyEllipsoid(const Vec3f& offset,
                                   const Quatf&,
                                   float,
                                   float pixelSize)
{
    float discSizeInPixels = pixelSize * getRadius() / offset.length();
    unsigned int nRings = (unsigned int) (discSizeInPixels / 4.0f);
    unsigned int nSlices = (unsigned int) (discSizeInPixels / 4.0f);
    nRings = max(nRings, 100u);
    nSlices = max(nSlices, 100u);

    VertexProcessor* vproc = context.getVertexProcessor();
    if (vproc == nullptr)
        return;

    //int e = min(max((int) type - (int) E0, 0), 7);
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
#endif


uint64_t Galaxy::getRenderMask() const
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


void Galaxy::setLightGain(float lg)
{
    lightGain = clamp(lg);
}


GalacticForm* buildGalacticForms(const fs::path& filename)
{
    Blob b;
    BlobVector* galacticPoints = new BlobVector;

    // Load templates in standard .png format
    int width, height, rgb, j = 0, kmin = 9;
    unsigned char value;
    float h = 0.75f;
    Image* img = LoadImageFromFile(filename);
    if (img == nullptr)
    {
        cout<<"\nThe galaxy template *** "<<filename<<" *** could not be loaded!\n\n";
        delete galacticPoints;
        return nullptr;
    }
    width  = img->getWidth();
    height = img->getHeight();
    rgb    = img->getComponents();

    for (int i = 0; i < width * height; i++)
    {
        value = img->getPixels()[rgb * i];
        if (value > 10)
        {
            float x, y, z, r2, yy, prob;
            z  = floor(i /(float) width);
            x  = (i - width * z - 0.5f * (width - 1)) / (float) width;
            z  = (0.5f * (height - 1) - z) / (float) height;
            x  += sfrand<float>() * 0.008f;
            z  += sfrand<float>() * 0.008f;
            r2 = x * x + z * z;

            if (filename != "models/E0.png")
            {
                float y0 = 0.5f * MAX_SPIRAL_THICKNESS * sqrt((float)value/256.0f) * exp(- 5.0f * r2);
                float B, yr;
                B = (r2 > 0.35f)? 1.0f: 0.75f; // the darkness of the "dust lane", 0 < B < 1
                float p0 = 1.0f - B * exp(-h * h); // the uniform reference probability, envelopping prob*p0.
                do
                {
                    // generate "thickness" y of spirals with emulation of a dust lane
                    // in galctic plane (y=0)

                    yr =  sfrand<float>() * h;
                    prob = (1.0f - B * exp(-yr * yr))/p0;

                } while (frand<float>() > prob);
                b.brightness  = value * prob;
                y = y0 * yr / h;
            }
            else
            {
                // generate spherically symmetric distribution from E0.png
                do
                {
                    yy = sfrand<float>();
                    float ry2 = 1.0f - yy * yy;
                    prob = ry2 > 0? sqrt(ry2): 0.0f;
                } while (frand<float>() > prob);
                y = yy * sqrt(0.25f - r2) ;
                b.brightness  = value;
                kmin = 12;
            }

            b.position    = Vector4f(x, y, z, 1.0f);
            unsigned int rr =  (unsigned int) (b.position.head(3).norm() * 511);
            b.colorIndex  = rr < 256? rr: 255;
            galacticPoints->push_back(b);
            j++;
        }
    }

    delete img;
    galacticPoints->reserve(j);

    // sort to start with the galaxy center region (x^2 + y^2 + z^2 ~ 0), such that
    // the biggest (brightest) sprites will be localized there!

    sort(galacticPoints->begin(), galacticPoints->end());

    // reshuffle the galaxy points randomly...except the first kmin+1 in the center!
    // the higher that number the stronger the central "glow"

    std::random_device rng;
    std::mt19937 urng(rng());
    shuffle(galacticPoints->begin() + kmin, galacticPoints->end(), urng);

    auto* galacticForm  = new GalacticForm();
    galacticForm->blobs = galacticPoints;
    galacticForm->scale = Vector3f::Ones();

    return galacticForm;
}


void InitializeForms()
{
    // Spiral Galaxies, 7 classical Hubble types

    spiralForms   = new GalacticForm*[7];

    spiralForms[Galaxy::S0]   = buildGalacticForms("models/S0.png");
    spiralForms[Galaxy::Sa]   = buildGalacticForms("models/Sa.png");
    spiralForms[Galaxy::Sb]   = buildGalacticForms("models/Sb.png");
    spiralForms[Galaxy::Sc]   = buildGalacticForms("models/Sc.png");
    spiralForms[Galaxy::SBa]  = buildGalacticForms("models/SBa.png");
    spiralForms[Galaxy::SBb]  = buildGalacticForms("models/SBb.png");
    spiralForms[Galaxy::SBc]  = buildGalacticForms("models/SBc.png");

    // Elliptical Galaxies , 8 classical Hubble types, E0..E7,
    //
    // To save space: generate spherical E0 template from S0 disk
    // via rescaling by (1.0f, 3.8f, 1.0f).

    ellipticalForms = new GalacticForm*[8];
    for (unsigned int eform  = 0; eform <= 7; ++eform)
    {
        float ell = 1.0f - (float) eform / 8.0f;

        // note the correct x,y-alignment of 'ell' scaling!!
        // build all elliptical templates from rescaling E0

        ellipticalForms[eform] = buildGalacticForms("models/E0.png");
        if (*ellipticalForms)
            ellipticalForms[eform]->scale = Vector3f(ell, ell, 1.0f);

        // account for reddening of ellipticals rel.to spirals
        if (*ellipticalForms)
        {
            unsigned int nPoints = (unsigned int) (ellipticalForms[eform]->blobs->size());
            for (unsigned int i = 0; i < nPoints; ++i)
            {
                (*ellipticalForms[eform]->blobs)[i].colorIndex = (unsigned int) ceil(0.76f * (*ellipticalForms[eform]->blobs)[i].colorIndex);
            }
        }
    }
    //Irregular Galaxies
    unsigned int galaxySize = GALAXY_POINTS, ip = 0;
    Blob b;
    Vector3f p;

    BlobVector* irregularPoints = new BlobVector;
    irregularPoints->reserve(galaxySize);

    while (ip < galaxySize)
    {
        p        = Vector3f(sfrand<float>(), sfrand<float>(), sfrand<float>());
        float r  = p.norm();
        if (r < 1)
        {
            float prob = (1 - r) * (fractalsum(Vector3f(p.x() + 5, p.y() + 5, p.z() + 5), 8) + 1) * 0.5f;
            if (frand<float>() < prob)
            {
                b.position   = Vector4f(p.x(), p.y(), p.z(), 1.0f);
                b.brightness = 64u;
                auto rr      =  (unsigned int) (r * 511);
                b.colorIndex = rr < 256 ? rr : 255;
                irregularPoints->push_back(b);
                ++ip;
            }
        }
    }
    irregularForm        = new GalacticForm();
    irregularForm->blobs = irregularPoints;
    irregularForm->scale = Vector3f::Constant(0.5f);

    formsInitialized = true;
}


ostream& operator<<(ostream& s, const Galaxy::GalaxyType& sc)
{
    return s << GalaxyTypeNames[static_cast<int>(sc)].name;
}
